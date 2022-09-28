#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <unistd.h>
#include "words/words.h"
#include "threads/threads.h"

static int MAX_THREADS = 32;
static int WORDS_PER_THREAD = 64;

// Count of all words
static int word_count = 0;
static word_t *all_words = NULL;

void* thread(void *arg) {
    thread_arg_t *data = (thread_arg_t *) arg;

    for (int i = data->start; i < data->end; i++) {
        int a = i;

        word_t a_word = all_words[a];
        uint32_t A = a_word.numeric;

        for (int j = 0; j < a_word.allowed_words_n; j++) {
            int b = a_word.allowed_words[j];
            word_t b_word = all_words[b];
            uint32_t B = b_word.numeric;

            for (int k = 0; k < b_word.allowed_words_n; k++) {
                int c = b_word.allowed_words[k];

                word_t c_word = all_words[c];
                uint32_t C = c_word.numeric;
                if ((A & C) != 0) {
                    continue;
                }

                uint32_t AB = A | B;
                for (int l = 0; l < c_word.allowed_words_n; l++) {
                    int d = c_word.allowed_words[l];

                    word_t d_word = all_words[d];
                    uint32_t D = d_word.numeric;
                    if ((AB & D) != 0) {
                        continue;
                    }

                    uint32_t ABC = AB | C;
                    for (int m = 0; m < d_word.allowed_words_n; m++) {
                        int e = d_word.allowed_words[m];

                        word_t e_word = all_words[e];
                        uint32_t E = e_word.numeric;
                        if ((ABC & E) != 0) {
                            continue;
                        }

                        printf(
                            "thread #%d [%d-%d]\t: %s %s %s %s %s\n",
                            data->id,
                            data->start,
                            data->end,
                            a_word.str,
                            b_word.str,
                            c_word.str,
                            d_word.str,
                            e_word.str
                        );
                    }
                }
            }
        }
    }

    mutex_lock();
    if (thread_manager.thread_count == thread_manager.max_threads) {
        pthread_cond_signal(&thread_manager.can_create_thread);
    }

    thread_manager.thread_count--;
    data->running = false;

    if (thread_manager.thread_count == 0) {
        pthread_cond_signal(&thread_manager.all_threads_finished);
    }
    mutex_unlock();

    return NULL;
}

void parse_options(int argc, char *argv[]) {
    char ch;
    while ((ch = getopt(argc, argv, "t:w:")) != -1) {
        if (ch == 't') {
            MAX_THREADS = atoi(optarg);
        }

        if (ch == 'w') {
            WORDS_PER_THREAD = atoi(optarg);
        }
    }
}

int main(int argc, char *argv[]) {
    parse_options(argc, argv);
    load_words(&all_words, &word_count);
    thread_manager_init(MAX_THREADS);

    printf("max_threads=%d, words_per_thread=%d\n", MAX_THREADS, WORDS_PER_THREAD);

    /**
     * The whole search space is divded into chunks now, since we're using
     * WORDS_PER_THREAD. So next_chunk_id tells us what the next launched thread
     * should investigate
     */
    int next_chunk_index = 0;

    /**
     * Indicates wether or not we're interested in waiting for threads to finish
     * If we are interested, it means we have more jobs to do, and we want to create
     * new threads
     */
    bool there_are_combinations_left = true;
    while (there_are_combinations_left) {
        // Waits for threads to become available, which on the first iteration will be
        // immediately
        mutex_wait_for_thread_availability();

        /**
         * Critical section. Creating new threads and updating the thread count.
         * Create as many threads as possible on this iteration.
         * 
         * On the first iteration, max_threads will be created, on every next iteration
         * we'll see. Probably only 1 thread at a time, but perhaps more.
         */
        for (int i = thread_manager.thread_count; i < thread_manager.max_threads; i++) {
            thread_arg_t arg = {
                .id = next_chunk_index,
                // .running should be here but create_thread does it for us
                .start = next_chunk_index * WORDS_PER_THREAD,
                .end = (next_chunk_index + 1) * WORDS_PER_THREAD
            };

            // The thread we were about to create would not have anything to check
            // We're done creating threads and checking combinations!
            if (arg.start >= word_count) {
                there_are_combinations_left = false;
                break;
            }

            if (arg.end >= word_count) {
                arg.end = word_count;
            }

            create_thread(arg, thread);
            next_chunk_index++;
        }

        mutex_unlock();
    }

    mutex_wait_for_all_threads_to_finish();
    mutex_unlock();

    return 0;
}
