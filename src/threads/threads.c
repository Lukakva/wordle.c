#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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
    thread_manager.work_done = 0;
    thread_manager.thread_args = (thread_arg_t *) calloc(max_threads, sizeof(thread_arg_t));
    if (thread_manager.thread_args == NULL) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }
    memset(thread_manager.thread_args, 0, max_threads * sizeof(thread_arg_t));

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

pthread_t create_thread(thread_arg_t arg, void *thread(void *)) {
    // Some thread finished, we can use this part of the heap now.
    thread_arg_t *free_thread_arg = NULL;
    for (int i = 0; i < thread_manager.max_threads; i++) {
        // Is your thread running? Well you beter catch it then :)
        if (!thread_manager.thread_args[i].running) {
            free_thread_arg = &(thread_manager.thread_args[i]);
            break;
        }
    }

    if (free_thread_arg == NULL) {
        printf("Could not find space for a thread\n");
        exit(EXIT_FAILURE);
    }

    // Before copying into the heap, mark it as running
    arg.running = true;
    memcpy(free_thread_arg, &arg, sizeof(thread_arg_t));
    thread_manager.thread_count++;

    pthread_t tid;
    if (pthread_create(&tid, NULL, thread, free_thread_arg) == -1) {
        perror(NULL);
        exit(EXIT_FAILURE);
    }

    return tid;
}

void mutex_wait_for_thread_availability() {
    mutex_lock();
    while (thread_manager.thread_count == thread_manager.max_threads) {
        pthread_cond_wait(&thread_manager.can_create_thread, &thread_manager.mutex);
    }
}

void mutex_wait_for_all_threads_to_finish() {
    mutex_lock();
    while (thread_manager.thread_count != 0) {
        pthread_cond_wait(&thread_manager.all_threads_finished, &thread_manager.mutex);
    }
}
