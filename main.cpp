#include <uthreads.h>
#include <iostream>
#include <gtest/gtest.h>
#include <random>
#include <algorithm>
#include <ctime>
#include <regex>

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *                        IMPORTANT
 * Each test, that is, whatever begins with TEST(XXX,XXX),
 * must be ran separately, that is, by clicking on the green button
 * and doing 'Run XXX.XXX'
 *
 * If you run all tests in a file/directory, e.g, by right clicking this .cpp
 * file and doing 'Run all in ...', THE TESTS WILL FAIL
 *
 * The reason is, each test expects a clean slate, that is, it expects that
 * the threads library wasn't initialized yet (and thus will begin with 1 quantum
 * once initialized).
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

TEST(DO_NOT_RUN_ALL_TESTS_AT_ONCE, READ_THIS)
{
FAIL() << "Do not run all tests at once via CLion, use the provided script" << std::endl;
}

// conversions to microseconds:
static const int MILLISECOND = 1000;
static const int SECOND = MILLISECOND * 1000;

/** Given a function, expects it to return -1 and that a thread library
 *  error message is printed to stderr
 */
template <class Function>
void expect_thread_library_error(Function function) {
    testing::internal::CaptureStderr();
    int retCode = function();
    std::string errOut = testing::internal::GetCapturedStderr();
    EXPECT_EQ(retCode, -1) << "failed thread library call should return -1";
    std::regex expectedPattern("thread library error: .*\\n");
    if (!std::regex_match(errOut, expectedPattern)) {
        ADD_FAILURE() << "no appropriate STDERR message was printed for failed thread library call";
    }
}

/**
 * This function initializes the library with given priorities.
 * If it fails, either you didn't implement these correctly,
 * or you ran multiple tests at the same time - see above note.
 */
void initializeWithPriorities(int *lengths, int priorityCount)
{
    ASSERT_EQ( uthread_init(lengths, priorityCount), 0);
    // First total number of quantums is always 1(the main thread)
    ASSERT_EQ ( uthread_get_total_quantums(), 1);

    if (STACK_SIZE < 8192)
    {
        std::cout << "(NOT AN ERROR) Your STACK_SIZE is " << STACK_SIZE << ", you might want to consider increasing it to at least 8192\n"
                  << "               If you have no trouble with the current stack size, ignore this message. " << std::endl;
    }
}



/**
 * Causes the currently running thread to sleep for a given number
 * of thread-quantums, that is, by counting the thread's own quantums.
 * Unlike "block", this works for the main thread too.
 *
 * @param threadQuants Positive number of quantums to sleep for
 */
void threadQuantumSleep(int threadQuants)
{
    assert (threadQuants > 0);
    int myId = uthread_get_tid();
    int start = uthread_get_quantums(myId);
    int end = start + threadQuants;
    /* Note, from the thread's standpoint, it is almost impossible for two consecutive calls to
     * 'uthread_get_quantum' to yield a difference larger than 1, therefore, at some point, uthread_get_quantums(myId)
     * must obtain 'end'.
     *
     * Theoretically, it's possible that the thread will be preempted before the condition check occurs, if this happens,
     * the above won't hold, and you'll get an infinite loop. But this is unlikely, as the following operation should
     * take much less than a microsecond
     */
    while (uthread_get_quantums(myId) != end)
    {
    }
}


/**
 * Testing basic thred library functionality and expected errors
 */
TEST(Test1, BasicFunctionality)
{
// first, initializing library with invalid parameters should fail
int invalidPriorities1[] = { -1337};
expect_thread_library_error([&](){
return uthread_init(invalidPriorities1, 1);
});

int priorites[] = { 100 * MILLISECOND};

expect_thread_library_error([&](){
// invalid size
return uthread_init(priorites, 0);
});

// initialize it for real now.
initializeWithPriorities(priorites, 1);

// can't block main thread
expect_thread_library_error([](){
return uthread_block(0);
});

// main thread has only started one(the current) quantum
EXPECT_EQ(uthread_get_quantums(0), 1);


static bool ran = false;
// most CPP compilers will translate this to a normal function (there's no closure)
auto t1 = []()
{
    EXPECT_EQ(uthread_get_tid(), 1);

    // every thread begins with 1 quantums
    EXPECT_EQ(uthread_get_quantums(1), 1);

    // main thread's quantums are unchanged
    EXPECT_EQ(uthread_get_quantums(0), 1);

    // this is the 2nd quantum in the program entire run
    EXPECT_EQ(uthread_get_total_quantums(), 2);
    ran = true;
    EXPECT_EQ ( uthread_terminate(1), 0);
};
EXPECT_EQ(uthread_spawn(t1, 0), 1);
// spawning a thread shouldn't cause a switch
// while theoretically it's possible that at the end of uthread_spawn
// we get a preempt-signal, with the quantum length we've specified at the test initialization, it shouldn't happen.

// (unless your uthread_spawn implementation is very slow, in which case you might want to check that out, or
//  just increase the quantum length at test initialization)

EXPECT_EQ(uthread_get_total_quantums(), 1);
EXPECT_EQ(uthread_get_quantums(0), 1);

// see implementation of this function for explanation
threadQuantumSleep(1);

EXPECT_TRUE(ran);
EXPECT_EQ(uthread_get_quantums(0), 2);
EXPECT_EQ(uthread_get_total_quantums(), 3);

// by now thread 1 was terminated, so operations on it should fail
expect_thread_library_error([](){
return uthread_get_quantums(1);
});
expect_thread_library_error([](){
return uthread_block(1);
});
expect_thread_library_error([](){
return uthread_resume(1);
});
expect_thread_library_error([](){
return uthread_terminate(1);
});


ASSERT_EXIT(uthread_terminate(0), ::testing::ExitedWithCode(0), "");
}

TEST(Test2, ThreadSchedulingWithTermination)
{
int priorities[] = { MILLISECOND };
initializeWithPriorities(priorities, 1);

static bool reached_middle = false;
static bool reached_f = false;

auto f = [](){
    EXPECT_TRUE ( reached_middle );
    reached_f = true;
    EXPECT_EQ ( uthread_terminate(1), 0);

};

auto g = [](){
    EXPECT_EQ ( uthread_resume(1), 0);
    EXPECT_EQ ( uthread_terminate(2), 0);
};


EXPECT_EQ ( uthread_spawn(f, 0), 1);
EXPECT_EQ ( uthread_spawn(g, 0), 2);
EXPECT_EQ ( uthread_block(1), 0);

threadQuantumSleep(1);
// since thread f is blocked, we expect to go to g, after which we'll go back to main thread

reached_middle = true;

// next, we'll go to f, and then back here(since g was terminataed)
threadQuantumSleep(1);

EXPECT_TRUE (reached_f);

// in total we had 5 quantums: 0->2->0->1->0
EXPECT_EQ ( uthread_get_total_quantums(), 5);

ASSERT_EXIT(uthread_terminate(0) , ::testing::ExitedWithCode(0), "");

}


/** In this test there's a total of 3 threads, including main one,
 *  and we basically track their order of execution
 */
TEST(Test3, ThreadExecutionOrder)
{
int priorities[] = { 100 * MILLISECOND };
initializeWithPriorities(priorities, 1);


// maps number of passed quantums(ran in each of the loops)
// to thread id
static std::map<int, int> quantumsToTids;


auto t = [](){
    int tid = uthread_get_tid();
    for (int i=1; i <= 4; ++i)
    {
        EXPECT_EQ( uthread_get_quantums(tid), i );
        quantumsToTids[uthread_get_total_quantums()] = tid;
        EXPECT_EQ( uthread_block(tid), 0);
    }
    EXPECT_EQ( uthread_terminate(tid), 0);
};

// every one of the 2 spawned threads will block itself
// every iteration, up to 4 iterations, then terminate itself
EXPECT_EQ(uthread_spawn(t, 0), 1);
EXPECT_EQ(uthread_spawn(t, 0), 2);

// the main thread will also "block" itself every iteration,
// but "blocking" is done by busy waiting, and it will also resume the other 2 threads
for (int i=1; i <= 4; ++i)
{
// sanity check
EXPECT_EQ(uthread_get_tid(), 0);

// the order in which we resume is only significant if
// the threads were blocked, in the first iteration,
// both threads aren't blocked, so this doesn't change
// their order in the queue
EXPECT_EQ( uthread_resume(2), 0);
EXPECT_EQ( uthread_resume(1), 0);
EXPECT_EQ(uthread_get_quantums(0), i);

quantumsToTids[uthread_get_total_quantums()] = 0;
threadQuantumSleep(1);
}

// therefore, we got the following execution:
// 0 -> 1 -> 2 -> 0 -> 2 -> 1 -> 0 -> 2 -> 1 -> 0 -> 2 -> 1 -> 0 -> exit
//[................][............][.............][...........][.........]
//      i=1              i=2         i=3              i=4        after loop

std::map<int, int> expectedQuantumsToTids {
        {1, 0},
        {2,1},
        {3,2},
        {4,0},
        {5,2},
        {6,1},
        {7,0},
        {8,2},
        {9,1},
        {10,0},
        {11,2},
        {12,1}
};
EXPECT_EQ( quantumsToTids, expectedQuantumsToTids);

ASSERT_EXIT(uthread_terminate(0), ::testing::ExitedWithCode(0), "");
}


/** This test involves multiple aspects:
 * - Spawning the maximal amount of threads, all running at the same time
 * - Terminating some of them after they've all spawned and ran at least once
 * - Spawning some again, expecting them to get the lowest available IDs
 */
TEST(Test4, StressTestAndThreadCreationOrder) {
// you can increase the quantum length, but even the smallest quantum should work
int priorities[] = { 1 };
initializeWithPriorities(priorities, 1);

// this is volatile, otherwise when compiling in -O2, the compiler considers the waiting loop further below
// as an infinite loop and optimizes it as such.
static volatile int ranAtLeastOnce = 0;
static auto f = [](){
    ++ranAtLeastOnce;
    while (true) {}
};

// you can lower this if you're trying to debug, but this should pass as is
const int SPAWN_COUNT = MAX_THREAD_NUM - 1;
std::vector<int> spawnedThreads;
for (int i=1; i <= SPAWN_COUNT ; ++i)
{
EXPECT_EQ ( uthread_spawn(f, 0), i);
spawnedThreads.push_back(i);
}

// wait for all spawned threads to run at least once
while (ranAtLeastOnce != SPAWN_COUNT) {}

if (SPAWN_COUNT == MAX_THREAD_NUM - 1) {
// by now, including the 0 thread, we have MAX_THREAD_NUM threads
// therefore further thread spawns should fail

expect_thread_library_error([](){ return uthread_spawn(f, 0);});
}

// now, terminate 1/3 of the spawned threads (selected randomly)
std::random_device rd;
std::shuffle(spawnedThreads.begin(),
        spawnedThreads.end(),
        rd);
std::vector<int> threadsToRemove ( spawnedThreads.begin(),
                                   spawnedThreads.begin() + SPAWN_COUNT * 1/3);
for (const int& tid: threadsToRemove)
{
EXPECT_EQ (uthread_terminate(tid), 0);
}

// now, we'll spawn an amount of threads equal to those that were terminated
// we expect the IDs of the spawned threads to equal those that were
// terminated, but in sorted order from smallest to largest
std::sort(threadsToRemove.begin(), threadsToRemove.end());
for (const int& expectedTid: threadsToRemove)
{
EXPECT_EQ (uthread_spawn(f, 0), expectedTid);
}

ASSERT_EXIT ( uthread_terminate(0), ::testing::ExitedWithCode(0), "");
}

/**
 * Times an operation, INCLUDING time in other threads
 * @return Time in microseconds
 */
template <class Function>
uint64_t timeOperation(Function op) {
    std::clock_t start = std::clock();
    op();
    std::clock_t stop = std::clock();
    double elapsed_us = 1e6 * ((double)(stop - start))/CLOCKS_PER_SEC;
    return elapsed_us;
}

TEST(Test5, TimesAndPriorities)
{
int priorities[] = { 300*MILLISECOND, SECOND, 2 * SECOND};
initializeWithPriorities(priorities, 3);

// Compenstate for timing inaccuracies
const int TIME_EPSILON = 300 * MILLISECOND;

auto f = [](){
    // note that changing the thread's priority only takes affect the next
    // time it is scheduled
    EXPECT_EQ ( uthread_change_priority(1, 2), 0);
    while (true) {};
};

// note that the thread is spawned with priority 1
EXPECT_EQ ( uthread_spawn(f, 1), 1);

// it should take about 1300ms for this operation to finish:
// ~300ms to finish thread 0's quantum and switch to thread 1
// +1000ms until we switch back here
int delta = timeOperation([&]() {  threadQuantumSleep(1); });
ASSERT_NEAR(delta, 1300 * MILLISECOND, TIME_EPSILON);


// now it should take 2300ms for this operation to finish,
// ~300ms to finish thread 0 and go to thread 1
// +2000ms to finish with thread 1 (since we increased its priority) and go back here
int delta2 = timeOperation([&]() {  threadQuantumSleep(1); });
ASSERT_NEAR(delta2, 2300 * MILLISECOND, TIME_EPSILON);

// now we'll ensure that you can change another thread's priority
EXPECT_EQ ( uthread_change_priority(1, 0), 0);

// 300ms + 300ms, since we changed t1's priority before it was scheduled, the change
// takes effect immediately
int delta3 = timeOperation([&]() {  threadQuantumSleep(1); });
ASSERT_NEAR(delta3, 600 * MILLISECOND, TIME_EPSILON);

ASSERT_EXIT ( uthread_terminate(0), ::testing::ExitedWithCode(0), "");
}

class RandomThreadTesting {
    std::set<int> activeThreads;
    std::unique_ptr<int[]> priorities;
    int priorityCount;
    std::mt19937 rand;

public:
    RandomThreadTesting(std::initializer_list<int> pr)
            : activeThreads(),
              priorities(new int[pr.size()]),
              priorityCount(pr.size()),
              rand{std::random_device{}()}
    {
        std::copy(pr.begin(), pr.end(), priorities.get());
        initializeWithPriorities(priorities.get(), priorityCount);
    }

    int getRandomActiveThread()
    {
        auto it = activeThreads.begin();
        auto dist  = std::uniform_int_distribution<>(0, activeThreads.size() - 1);
        std::advance(it, dist(rand));
        assert (it != activeThreads.end());
        return *it;
    }


    /**
     * Perform random thread library operation, except those involving thread 0
     * @param func Thread function if spawning
     */
    void doOperation(void (*func)())
    {
        // 50% chance of doing a thread operation(1-5), 50% of not doing anything(6-10)
        // if there are no threads, create new one
        int option = std::uniform_int_distribution<>(1, 10)(rand);
        if (activeThreads.empty() || (option == 1 && activeThreads.size() < MAX_THREAD_NUM - 1) )
        {
            // create a thread
            int priority = std::uniform_int_distribution<>(0, priorityCount-1)(rand);
            int tid = uthread_spawn(func, priority);
            EXPECT_GT(tid, 0);
            activeThreads.emplace(tid);
        } else {
            int tid = getRandomActiveThread();
            if (option == 2) {
                // change thread priority
                int priority = std::uniform_int_distribution<>(0, priorityCount-1)(rand);
                EXPECT_EQ(uthread_change_priority(tid, priority), 0);
            } else if (option == 3)
            {
                // terminate thread
                EXPECT_EQ(uthread_terminate(tid), 0);
                activeThreads.erase(tid);
            } else if (option == 4)
            {
                // block thread
                EXPECT_EQ(uthread_block(tid), 0);
            } else if (option == 5)
            {
                // resume thread
                EXPECT_EQ(uthread_resume(tid), 0);
            } else {
                // don't perform a thread operation
            }
        } // OOP? open closed principle? ain't nobody got time for that
    }


};

/** This test performs a bunch of random thread library operations, it is used
 *  to detect crashes and other memory errors
 *
 */
TEST(Test6, RandomThreadOperations)
{
RandomThreadTesting rtt ({1});

const int ITER_COUNT = 100000;

auto f = [](){
    while (true) {
    }
};
for (int i=0; i < ITER_COUNT; ++i)
{
rtt.doOperation(f);
}
ASSERT_EXIT ( uthread_terminate(0), ::testing::ExitedWithCode(0), "");
}
