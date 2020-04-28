#include <setjmp.h>
#include <signal.h>
#include <sys/time.h>
#include <deque>
#include <iostream>
#include <algorithm>

#include "uthreads.h"

#define JMP_RET_VAL 1

#ifdef __x86_64__
/* code for 64 bit Intel arch */

typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%fs:0x30,%0\n"
                 "rol    $0x11,%0\n"
    : "=g" (ret)
    : "0" (addr));
    return ret;
}

#else
/* code for 32 bit Intel arch */

typedef unsigned int address_t;
#define JB_SP 4
#define JB_PC 5

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%gs:0x18,%0\n"
		"rol    $0x9,%0\n"
                 : "=g" (ret)
                 : "0" (addr));
    return ret;
}

#endif

// enums for in-library usage
enum uthread_state { READY, RUNNING, BLOCKED };
enum error_type { SYS_ERR, LIB_ERR };

/* The uthread instance struct holds all the relevant data
 * structures and information to every thread */
struct uthread_instance {
    int tid;
    int priority;
    uthread_state state;
    char stack[STACK_SIZE];
    sigjmp_buf env;
    int quantum_count;
};
typedef uthread_instance* uthread_instance_ptr;


/* Pointers array to all the threads instances. Every thread is
 * stored in the array by his ID (main thread in existing_threads[0],
 * thread with ID=1 in existing_threads[1], etc.). */
uthread_instance_ptr existing_threads[MAX_THREAD_NUM] = {nullptr};

/* Queue for all the READY threads. The queue holds the threads ID */
std::deque<int> ready_queue;

// The ID of the running thread
int running_uthread_tid;

// hold the quantum_usecs for each priority
int* priorities_quantum_usecs;
int priorities_size;

// the count of the total quantums from library initialization
int total_quantum_count;

// {SIGVTALRM} set
sigset_t set_sigvtalrm;

/* Returns true if thread with the given ID as tid exist */
bool is_uthread_exist(int tid) {
    // std::cout << "is_uthread_exist\n";
    if (tid >= MAX_THREAD_NUM || tid < 0)
        return false;
    return existing_threads[tid] != nullptr;
}

void safe_exit(int code) {
    for (int i=1; i<MAX_THREAD_NUM; ++i)
        if (is_uthread_exist(i))
            delete existing_threads[i];
    exit(code);
}

/* Prints error to stderr by the given message and error type*/
void print_error(const char *msg, error_type type) {
    // std::cout << "print_error\n";
    if (type == SYS_ERR)
        std::cerr << "system error: ";
    else if (type == LIB_ERR)
        std::cerr << "thread library error: ";
    else
        std::cerr << "unknown error: ";
    std::cerr << msg << std::endl;
}

/* Returns the smallest free ID for new thread.
 * Return value: On success, return the smallest free ID.
 * On failure, return -1.*/
int get_free_tid() {
    // std::cout << "get_free_tid\n";
    for (int i=0; i < MAX_THREAD_NUM; ++i)
        if (existing_threads[i] == nullptr)
            return i;
    return -1;
}

/* Check if priority is in range, and print error if not.
 * Returns true if there is an error, and false otherwise */
bool check_priority_range_error(int priority) {
    if (priority < 0 && priority >= priorities_size) {
        print_error("given priority is out of range", LIB_ERR);
        return true;
    } else
        return false;
}

/* Check if ID exist, and print error if not.
 * Returns true if there is an error, and false otherwise */
bool check_tid_existence_error(int tid) {
    if (!is_uthread_exist(tid)) {
        print_error("thread with the given ID doesn't exist", LIB_ERR);
        return true;
    } else
        return false;
}

void safe_sigvalrm_masking(int how) {
    if (sigprocmask(how, &set_sigvtalrm, nullptr) == -1) {
        print_error("sigprocmask() error", SYS_ERR);
        safe_exit(1);
    }
}

/* insert ID from ready queue by given tid */
void push_to_ready_queue(int tid) {
    // std::cout << "push_to_ready_queue\n";
    ready_queue.push_back(tid);
}

/* Remove ID from ready queue by given tid */
void remove_from_ready_queue(int tid) {
    // std::cout << "remove_from_ready_queue\n";
    ready_queue.erase(std::remove(ready_queue.begin(), ready_queue.end(), tid),
                      ready_queue.end());
}

/* Pop and return the front of the ready queue */
int pop_from_ready_queue() {
    // std::cout << "pop_from_ready_queue\n";
    int val = ready_queue.front();
    ready_queue.pop_front();
    return val;
}

/* Return true if ready queue is empty, false otherwise */
bool is_ready_queue_empty() {
    // std::cout << "is_ready_queue_empty\n";
    return ready_queue.empty();
}

void start_quantum_timer(uthread_instance_ptr ut) {
    // std::cout << "start_quantum_timer\n";

    // Timer for the scheduler
    struct itimerval timer = {0};

    // set virtual timer values
    timer.it_value.tv_usec = priorities_quantum_usecs[ut->priority];

    // start a virtual timer
    if (setitimer(ITIMER_VIRTUAL, &timer, nullptr)) {
        print_error("setitimer() error", SYS_ERR);
        safe_exit(1);
    }

    // decrease quantum count of given thread
    existing_threads[ut->tid]->quantum_count++;
    total_quantum_count++;
}

/* If ready queue isn't empty run the next ready thread. */
void run_next_ready_thread() {
    // std::cout << "run_next_ready_thread\n";
    int next_tid;
    uthread_instance_ptr ut;


    // switch threads only if ready queue isn't empty
    if (!is_ready_queue_empty()) {
        next_tid = pop_from_ready_queue();

        ut = existing_threads[next_tid]; // thread instance

        // run next thread
        ut->state = RUNNING;
        running_uthread_tid = next_tid;

        start_quantum_timer(ut);

        // std::cout << "ut(" << ut->tid << ")\n";
        siglongjmp(ut->env, JMP_RET_VAL);
    } else {
        // continue running thread run
        ut = existing_threads[running_uthread_tid];
        start_quantum_timer(ut);
    }
}

/* Thrown by the timer. make the first thread in the ready
 * queue the running thread and push the running thread to
 * the end of the ready queue */
void uthread_timing_scheduler(int sig) {
    // std::cout << "uthread_timing_scheduler\n";
    uthread_instance_ptr ut;

    safe_sigvalrm_masking(SIG_BLOCK);

    ut = existing_threads[running_uthread_tid];
    ut->state = READY;
    push_to_ready_queue(ut->tid);

    safe_sigvalrm_masking(SIG_UNBLOCK);

    if (sigsetjmp(ut->env, 1) == 0)
        run_next_ready_thread();
}


/*
 * Description: This function initializes the thread library.
 * You may assume that this function is called before any other thread library
 * function, and that it is called exactly once. The input to the function is
 * an array of the length of a quantum in micro-seconds for each priority.
 * It is an error to call this function with an array containing non-positive integer.
 * size - is the size of the array.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_init(int *quantum_usecs, int size) {
    // std::cout << "uthread_init\n";
    struct sigaction sa = {0};
    uthread_instance_ptr ut;

    // check if size is valid
    if (size <= 0) {
        print_error("quantum_usecs is invalid", LIB_ERR);
        return -1;
    }

    // check if all quantum usecs contain non-positive integers
    for (int i=0; i<size; ++i) {
        if (quantum_usecs[i] <= 0) {
            print_error("quantum_usecs should contain positive integers only", LIB_ERR);
            return -1;
        }
    }

    // save quantum usecs
    priorities_quantum_usecs = quantum_usecs;
    priorities_size = size;

    // create {SIGVTALRM} set
    if (sigemptyset(&set_sigvtalrm) == -1) {
        print_error("sigemptyset() error", SYS_ERR);
        safe_exit(1);
    }
    if (sigaddset(&set_sigvtalrm, SIGVTALRM) == -1) {
        print_error("sigaddset() error", SYS_ERR);
        safe_exit(1);
    }

    // mask all signalsafe_exits
    if (sigfillset(&sa.sa_mask) == -1) {
        print_error("sigfillset() error", SYS_ERR);
        safe_exit(1);
    }

    // connect signals to terminate_running_thread
    sa.sa_handler = &uthread_timing_scheduler;
    if (sigaction(SIGVTALRM, &sa,nullptr) < 0) {
        print_error("sigaction() error", SYS_ERR);
        safe_exit(1);
    }

    // save main thread
    // allocate new memory for uthread_instance
    ut = new (std::nothrow) uthread_instance;
    if (ut == nullptr) {
        print_error("can't allocate new memory.", SYS_ERR);
        safe_exit(1);
    }

    // initialize tid
    ut->tid = 0;
    // initialize priority
    ut->priority = 0;

    // initialize state
    ut->state = RUNNING;
    running_uthread_tid = 0;

    // initialize quantum_count and total_quantum_count
    ut->quantum_count = 0;
    total_quantum_count = 0;

    // other initializations
    existing_threads[0] = ut; // add to existing threads

    // start the timer
    start_quantum_timer(ut);

    return 0;
}

/*
 * Description: This function creates a new thread, whose entry point is the
 * function f with the signature void f(void). The thread is added to the end
 * of the READY threads list. The uthread_spawn function should fail if it
 * would cause the number of concurrent threads to exceed the limit
 * (MAX_THREAD_NUM). Each thread should be allocated with a stack of size
 * STACK_SIZE bytes.
 * priority - The priority of the new thread.
 * Return value: On success, return the ID of the created thread.
 * On failure, return -1.
*/
int uthread_spawn(void (*f)(void), int priority) {
    // std::cout << "uthread_spawn\n";

    int tid;
    address_t sp, pc;
    uthread_instance_ptr ut;

    safe_sigvalrm_masking(SIG_BLOCK);

    if (check_priority_range_error(priority))
        return -1;

    // check if threads amount is over the limit
    tid = get_free_tid();
    if (tid == -1) {
        print_error("threads amount over MAX_THREAD_NUM!", LIB_ERR);
        return -1;
    }

    // allocate new memory for uthread_instance
    ut = new (std::nothrow) uthread_instance;
    if (ut == nullptr) {
        print_error("can't allocate new memory.", SYS_ERR);
        safe_exit(1);
    }

    // initialize tid
    ut->tid = tid;
    // initialize priority
    ut->priority = priority;
    // initialize state
    ut->state = READY;

    // initialize env
    sp = (address_t)ut->stack + STACK_SIZE - sizeof(address_t);
    pc = (address_t)f;
    sigsetjmp(ut->env, 1);
    (ut->env->__jmpbuf)[JB_SP] = translate_address(sp);
    (ut->env->__jmpbuf)[JB_PC] = translate_address(pc);
    sigemptyset(&ut->env->__saved_mask);

    // initialize quantum_count
    ut->quantum_count = 0;

    // other initializations
    existing_threads[ut->tid] = ut; // add to existing threads
    push_to_ready_queue(ut->tid); // inert to ready queue

    safe_sigvalrm_masking(SIG_UNBLOCK);

    return ut->tid;
}

/*
 * Description: This function changes the priority of the thread with ID tid.
 * If this is the current running thread, the effect should take place only the
 * next time the thread gets scheduled.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_change_priority(int tid, int priority) {
    // std::cout << "uthread_change_priority\n";

    safe_sigvalrm_masking(SIG_BLOCK);

    if (check_tid_existence_error(tid))
        return -1;

    if (check_priority_range_error(priority))
        return -1;

    existing_threads[tid]->priority = priority;

    safe_sigvalrm_masking(SIG_UNBLOCK);

    return 0;
}

/*
 * Description: This function terminates the thread with ID tid and deletes
 * it from all relevant control structures. All the resources allocated by
 * the library for this thread should be released. If no thread with ID tid
 * exists it is considered an error. Terminating the main thread
 * (tid == 0) will result in the termination of the entire process using
 * safe_exit(0) [after releasing the assigned library memory].
 * Return value: The function returns 0 if the thread was successfully
 * terminated and -1 otherwise. If a thread terminates itself or the main
 * thread is terminated, the function does not return.
*/
int uthread_terminate(int tid) {
    // std::cout << "uthread_terminate\n";

    safe_sigvalrm_masking(SIG_BLOCK);

    if (check_tid_existence_error(tid))
        return -1;

    uthread_instance_ptr ut = existing_threads[tid]; // thread instance
    uthread_state state = ut->state; // thread state

    // safe_exit if is main thread
    if (tid == 0)
        safe_exit(0);

    // delete allocated memory
    delete ut;
    existing_threads[tid] = nullptr;

    // check if thread is in ready queue
    if (state == READY)
        remove_from_ready_queue(tid);

    safe_sigvalrm_masking(SIG_UNBLOCK);

    // if thread state was running then run the next ready thread
    if (state == RUNNING)
        run_next_ready_thread();

    return 0;
}


/*
 * Description: This function blocks the thread with ID tid. The thread may
 * be resumed later using uthread_resume. If no thread with ID tid exists it
 * is considered as an error. In addition, it is an error to try blocking the
 * main thread (tid == 0). If a thread blocks itself, a scheduling decision
 * should be made. Blocking a thread in BLOCKED state has no
 * effect and is not considered an error.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_block(int tid) {
    // std::cout << "uthread_block\n";

    safe_sigvalrm_masking(SIG_BLOCK);

    if (tid == 0) {
        print_error("can't block main thread!", LIB_ERR);
        return -1;
    }

    if (check_tid_existence_error(tid))
        return -1;

    uthread_instance_ptr ut = existing_threads[tid]; // uthread instance
    uthread_state old_state = ut->state; // old thread ID

    // change thread state to BLOCKED
    ut->state = BLOCKED;

    // if old state is READY, pop from ready queue
    if (old_state == READY)
        remove_from_ready_queue(tid);

    safe_sigvalrm_masking(SIG_UNBLOCK);

    // if the current running thread is blocked save its environment
    if (old_state == RUNNING)
        if (sigsetjmp(ut->env, 1) == 0)
            run_next_ready_thread();

    return 0;
}


/*
 * Description: This function resumes a blocked thread with ID tid and moves
 * it to the READY state if it's not synced. Resuming a thread in a RUNNING or READY state
 * has no effect and is not considered as an error. If no thread with
 * ID tid exists it is considered an error.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_resume(int tid){
    // std::cout << "uthread_resume\n";

    safe_sigvalrm_masking(SIG_BLOCK);

    if (check_tid_existence_error(tid))
        return -1;

    uthread_instance_ptr ut = existing_threads[tid]; // uthread instance

    // change only if BLOCKED
    if (ut->state == BLOCKED) {
        ut->state = READY;
        push_to_ready_queue(tid);
    }

    safe_sigvalrm_masking(SIG_UNBLOCK);
    return 0;
}


/*
 * Description: This function returns the thread ID of the calling thread.
 * Return value: The ID of the calling thread.
*/
int uthread_get_tid() {
    // std::cout << "uthread_get_tid\n";
    return running_uthread_tid;
}


/*
 * Description: This function returns the total number of quantums since
 * the library was initialized, including the current quantum.
 * Right after the call to uthread_init, the value should be 1.
 * Each time a new quantum starts, regardless of the reason, this number
 * should be increased by 1.
 * Return value: The total number of quantums.
*/
int uthread_get_total_quantums() {
    // std::cout << "uthread_get_total_quantums\n";
    return total_quantum_count;
}


/*
 * Description: This function returns the number of quantums the thread with
 * ID tid was in RUNNING state. On the first time a thread runs, the function
 * should return 1. Every additional quantum that the thread starts should
 * increase this value by 1 (so if the thread with ID tid is in RUNNING state
 * when this function is called, include also the current quantum). If no
 * thread with ID tid exists it is considered an error.
 * Return value: On success, return the number of quantums of the thread with ID tid.
 * 			     On failure, return -1.
*/
int uthread_get_quantums(int tid) {
    // std::cout << "uthread_get_quantums\n";

    int quantums;

    safe_sigvalrm_masking(SIG_BLOCK);

    if (check_tid_existence_error(tid))
        return -1;

    quantums = existing_threads[tid]->quantum_count;

    safe_sigvalrm_masking(SIG_UNBLOCK);

    return quantums;
}