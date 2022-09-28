#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>

/**
 * An argument passed to a thread.
 */
typedef struct {
    /** Thread ID. */
    uint8_t id;

    /**
     * Index of the word. Thread should start trying all combinations
     * where the first word is at this index.
     */
    uint16_t start;

    /** Index of the last word thread should try for the first position. */
    uint16_t end;
} thread_arg_t;

/**
 * Helps manage threads easier.
 */
typedef struct {
    /** How many threads are currently running. */
    uint16_t thread_count;

    /** Max number of threads. */
    uint16_t max_threads;

    /** Condition variable, signals when thread_count becomes < max_threads. */
    pthread_cond_t can_create_thread;

    /** Signals when all threads have finished. */
    pthread_cond_t all_threads_finished;

    /** Mutex. */
    pthread_mutex_t mutex;
} thread_manager_t;

/** Locks the thread_manager mutex. */
void mutex_lock();

/** Unlocks the thread_manager mutex. */
void mutex_unlock();

/** Initializes the thread_manager. */
void thread_manager_init(int max_threads);

/** Creates a thread and updates the thread count. */
pthread_t create_thread(thread_arg_t arg, void *thread(void *));

/** Waits until a new thread can be created. Acquires a mutex lock. */
void mutex_wait_for_thread_availability();

/** Waits until all threads have finished. Acquires a mutex lock. */
void mutex_wait_for_all_threads_to_finish();
