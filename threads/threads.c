#include <stdio.h>
#include <string.h>
#include "threads.h"

thread_manager_t thread_manager = {0};

void mutex_lock() {
    if (pthread_mutex_lock(&thread_manager.mutex) == -1) {
        perror("pthread_mutex_lock");
        exit(EXIT_FAILURE);
    }
}

void mutex_unlock() {
    if (pthread_mutex_unlock(&thread_manager.mutex) == -1) {
        perror("pthread_mutex_unlock");
        exit(EXIT_FAILURE);
    }
}

void thread_manager_init(int max_threads) {
    thread_manager.thread_count = 0;
    thread_manager.max_threads = max_threads;

    if (pthread_mutex_init(&thread_manager.mutex, NULL) == -1) {
        perror("pthread_mutex_init");
        exit(EXIT_FAILURE);
    }

    if (pthread_cond_init(&thread_manager.can_create_thread, NULL)) {
        perror("pthread_cond_init");
        exit(EXIT_FAILURE);
    }

    if (pthread_cond_init(&thread_manager.all_threads_finished, NULL)) {
        perror("pthread_cond_init");
        exit(EXIT_FAILURE);
    }
}

/**
 * Creates a thread. MUST BE CALLED after acquiring a mutex lock
 * since this function operates on the shared `thread_manager` variable.
 * 
 * @param arg - Argument to be passed to the thread. Will be copied to the heap.
 * @param thread - A function pointer.
 * @return pthread_t
 */
pthread_t create_thread(thread_arg_t arg, void *thread(void *)) {
    // Copy of the thread argument on the heap
    thread_arg_t *arg_copy = (thread_arg_t *) malloc(sizeof(thread_arg_t));
    if (arg_copy == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    memcpy(arg_copy, &arg, sizeof(thread_arg_t));
    thread_manager.thread_count++;

    pthread_t tid;
    if (pthread_create(&tid, NULL, thread, arg_copy) == -1) {
        perror(NULL);
        exit(EXIT_FAILURE);
    }

    return tid;
}

/**
 * Awaits until another thread can be created.
 * This method acquires but doesn't release a mutex lock.
 */
void mutex_wait_for_thread_availability() {
    mutex_lock();
    while (thread_manager.thread_count == thread_manager.max_threads) {
        pthread_cond_wait(&thread_manager.can_create_thread, &thread_manager.mutex);
    }
}

/**
 * Waits for all threads to finish.
 */
void mutex_wait_for_all_threads_to_finish() {
    mutex_lock();
    while (thread_manager.thread_count != 0) {
        pthread_cond_wait(&thread_manager.all_threads_finished, &thread_manager.mutex);
    }
}
