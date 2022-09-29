#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>

/** An argument passed to a thread. */
typedef struct {
    /** Thread ID. */
    uint8_t id;

    /** Wether or not this thread is still running. */
    bool running;

    /**
     * Index of the first word to check.
     */
    uint16_t start;

    /** Index of the last word thread should try for the first position. */
    uint16_t end;
} thread_arg_t;

/** Helps manage threads easier. */
typedef struct {
    /** How many threads are currently running. */
    uint16_t thread_count;

    /** Max number of threads. */
    uint16_t max_threads;

    /**
     * Just out of curiousity, how many iterations, comparisons
     * or work has been done in total.
     * Each thread, before terminating, logs its work done.
     */
    unsigned long long int work_done;

    /** Thread arguments */
    thread_arg_t *thread_args;

    /** Condition variable, signals when thread_count becomes < max_threads. */
    pthread_cond_t can_create_thread;

    /** Signals when all threads have finished. */
    pthread_cond_t all_threads_finished;

    /** Mutex. */
    pthread_mutex_t mutex;
} thread_manager_t;

/** The thread manager. */
thread_manager_t thread_manager;

/** Locks the thread_manager mutex. */
void mutex_lock();

/** Unlocks the thread_manager mutex. */
void mutex_unlock();

/**
 * Initializes the thread_manager. 
 * 
 * @param max_threads Maximum number of threads.
 */
void thread_manager_init(int max_threads);

/**
 * Creates a thread. MUST BE CALLED after acquiring a mutex lock
 * since this function operates on the shared `thread_manager` variable.
 * 
 * @param arg Argument to be passed to the thread. Will be copied to the heap.
 * @param thread A function pointer for the thread to execute.
 * @return pthread_t
 */
pthread_t create_thread(thread_arg_t arg, void *thread(void *));

/**
 * Awaits until another thread can be created.
 * This method acquires but doesn't release a mutex lock.
 */
void mutex_wait_for_thread_availability();

/**
 * Waits for all threads to finish.
 */
void mutex_wait_for_all_threads_to_finish();
