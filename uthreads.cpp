#include <stdio.h>
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <deque>
#include <iostream>

#include "uthreads.h"

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

enum uthread_state { READY, RUNNING, BLOCKED };
enum error_type { SYS_ERR, LIB_ERR };

struct uthread_instance {
    int tid;
    int priority;
    uthread_state state;
    char stack[STACK_SIZE];
    sigjmp_buf env;
    int quantum_count;
};

typedef uthread_instance* uthread_instance_ptr;

uthread_instance_ptr existing_threads[MAX_THREAD_NUM] = {nullptr};

std::deque<int> ready_uthreads_tid;
int running_uthread_tid;

int* priorities_quantum_usecs;
int priorities_amount;

int total_quantum_count;

/* Prints error to stderr by the given message and error type*/
void print_error(const char *msg, error_type type) {
    if (type == SYS_ERR)
        std::cerr << "system error: ";
    else if (type == LIB_ERR)
        std::cerr << "thread library error: ";
    else
        std::cerr << "unknown error: ";
    std::cout << msg << std::endl;
}

/* Returns the smallest free ID for new thread.
 * Return value: On success, return the smallest free ID.
 * On failure, return -1.*/
int get_free_tid() {
    int i;
    for (i=0; i<MAX_THREAD_NUM; ++i)
        if (existing_threads[i] == nullptr)
            return i;
    return -1;
}

/* Returns true if thread with the given ID as tid exist */
bool uthread_exist(int tid) {
    if (tid == 0)
        return true;
    return existing_threads[tid] != nullptr;
}

void witch_running_thread()

void insert_to_ready_queue(int tid); // Change the state of thread to ready and
                                     // insert it to the queue

void remove_from_ready_queue(int tid);

void free_uthread_resources(int tid); // free all the uthread resources

void uthread_scheduler(); // ?

void decrease_quantum_count(int tid) {
    existing_threads[tid]->quantum_count++;
    total_quantum_count++;
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
int uthread_init(int *quantum_usecs, int size);
// init priorities_quantum_usecs & priorities_amount
// connecting signals to functions
// init data to null/0
// save data of thread 0

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
    int tid;
    address_t sp, pc;

    // check if threads amount is over the limit
    tid = get_free_tid();
    if (tid == -1) {
        print_error("threads amount over MAX_THREAD_NUM!", LIB_ERR);
        return -1;
    }

    // allocate new memory for uthread_instance
    uthread_instance_ptr ut = new (std::nothrow) uthread_instance;
    if (ut == nullptr) {
        print_error("can't allocate new memory.", SYS_ERR);
        return -1;
    }

    // initialize tid
    ut->tid = tid;

    // initialize priority
    ut->priority = priority;

    // initialize state
    ut->state = READY;

    // initialize stack
    for (int i=0; i<STACK_SIZE; ++i)
        ut->stack[i] = 0;

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
    insert_to_ready_queue(ut->tid); // inert to ready queue
}

/*
 * Description: This function changes the priority of the thread with ID tid.
 * If this is the current running thread, the effect should take place only the
 * next time the thread gets scheduled.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_change_priority(int tid, int priority) {
    if (!uthread_exist(tid)) {
        print_error("thread with the given ID doesn't exist", LIB_ERR);
        return -1;
    }
    existing_threads[tid]->priority = priority;
    return 0;
}


/*
 * Description: This function terminates the thread with ID tid and deletes
 * it from all relevant control structures. All the resources allocated by
 * the library for this thread should be released. If no thread with ID tid
 * exists it is considered an error. Terminating the main thread
 * (tid == 0) will result in the termination of the entire process using
 * exit(0) [after releasing the assigned library memory].
 * Return value: The function returns 0 if the thread was successfully
 * terminated and -1 otherwise. If a thread terminates itself or the main
 * thread is terminated, the function does not return.
*/
int uthread_terminate(int tid) {
    if (!uthread_exist(tid)) {
        print_error("thread with the given ID doesn't exist", LIB_ERR);
        return -1;
    }

    // if thread is main thread - kill all other threads
    if (tid == 0) {
        for (int i=1; i<MAX_THREAD_NUM; ++i)
            if (existing_threads[tid] != nullptr)
                uthread_terminate(i);
    }

    uthread_instance_ptr ut = existing_threads[tid]; // thread instance
    uthread_state state = ut->state; // thread state

    // check if thread is in ready queue
    if (state == READY)
        remove_from_ready_queue(tid);

    // delete allocated memory
    delete ut;
    existing_threads[tid] = nullptr;

    // exit if is main thread
    if (tid == 0)
        exit(0);

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
    if (tid == 0) {
        print_error("can't block main thread!", LIB_ERR);
        return -1;
    }
    if (!uthread_exist(tid)) {
        print_error("thread with the given ID doesn't exist", LIB_ERR);
        return -1;
    }

    uthread_instance_ptr ut = existing_threads[tid]; // uthread instance
    uthread_state old_state = ut->state; // old thread ID

    ut->state = BLOCKED;

    if (old_state == READY)
        remove_from_ready_queue(tid)

    if (old_state == RUNNING)

    return 0;
}
// errors: tid not exist or tid==0


/*
 * Description: This function resumes a blocked thread with ID tid and moves
 * it to the READY state if it's not synced. Resuming a thread in a RUNNING or READY state
 * has no effect and is not considered as an error. If no thread with
 * ID tid exists it is considered an error.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_resume(int tid);


/*
 * Description: This function returns the thread ID of the calling thread.
 * Return value: The ID of the calling thread.
*/
int uthread_get_tid() {
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
    if (!uthread_exist(tid))
        return -1;
    return existing_threads[tid]->quantum_count;
}