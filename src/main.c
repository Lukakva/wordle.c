#include "words/words.h"
#include "threads/threads.h"
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/time.h>

extern thread_manager_t thread_manager;

/**
 * Default options.
 *
 * These values are determined from the `time.js` / `time.txt` output.
 * These work the best for my 8 core machine.
 */
static int VERBOSE = 1;
static int MAX_THREADS = 8;
static int WORDS_PER_THREAD = 10;

// Count of all words that we're working with
static int word_count = 0;
static word_t *all_words = NULL;

/**
 * The thread itself. Given a thread argument, which tells the thread the range
 * to search through.
 */
static void* thread(void *arg) {
    thread_arg_t *data = (thread_arg_t *) arg;
    unsigned long long int work_done = 0;

    for (int i = data->start; i < data->end; i++) {
        // Grab the first word
        word_t word_1 = all_words[i];
        uint32_t n1 = word_1.numeric;
        work_done++;

        // Only iterate through the words that we know don't overlap with the first word
        for (int j = 0; j < word_1.neighbors_n; j++) {
            // Grab the index of the second word choice
            int index_2 = word_1.neighbors[j];
            // Retrieve that word
            word_t word_2 = all_words[index_2];
            uint32_t n2 = word_2.numeric;
            work_done++;

            for (int k = 0; k < word_2.neighbors_n; k++) {
                // Grab the index for the third word
                int index_3 = word_2.neighbors[k];

                word_t word_3 = all_words[index_3];
                uint32_t n3 = word_3.numeric;
                work_done++;

                // Check if the first and the third word overlap in characters
                if ((n1 & n3) != 0) {
                    continue;
                }

                /**
                 * Represents a union of the first and the second words,
                 * Because in order to find a fourth word, we don't need
                 * to check if the fourth word overlaps with the third one
                 * since we're only going through the words that don't overlap
                 * with the third one, but we do need to check if the 4th word
                 * overlaps with either the first or the second.
                 * Basically for any word at position `n` we know
                 * it doesn't overlap with `n - 1`, but we have to check for (0..n-2).
                 */
                uint32_t n12 = n1 | n2;
                for (int l = 0; l < word_3.neighbors_n; l++) {
                    int index_4 = word_3.neighbors[l];

                    word_t word_4 = all_words[index_4];
                    uint32_t n4 = word_4.numeric;
                    work_done++;

                    if ((n12 & n4) != 0) {
                        continue;
                    }

                    uint32_t n123 = n12 | n3;
                    for (int m = 0; m < word_4.neighbors_n; m++) {
                        int index_5 = word_4.neighbors[m];
                        word_t word_5 = all_words[index_5];
                        uint32_t n5 = word_5.numeric;
                        work_done++;

                        if ((n123 & n5) != 0) {
                            continue;
                        }

                        if (VERBOSE) {
                            printf(
                                "thread #%03d   chunk[%04d-%04d]: %s %s %s %s %s\n",
                                data->id,
                                data->start,
                                data->end,
                                word_1.str,
                                word_2.str,
                                word_3.str,
                                word_4.str,
                                word_5.str
                            );
                        } else {
                            printf(
                                "%s %s %s %s %s\n",
                                word_1.str,
                                word_2.str,
                                word_3.str,
                                word_4.str,
                                word_5.str
                            );
                        }
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
    thread_manager.work_done += work_done;
    data->running = false;

    if (thread_manager.thread_count == 0) {
        pthread_cond_signal(&thread_manager.all_threads_finished);
    }
    mutex_unlock();

    return NULL;
}

static void parse_options(int argc, char *argv[]) {
    char ch;
    while ((ch = getopt(argc, argv, "t:w:hs")) != -1) {
        switch (ch) {
            case 't':
                MAX_THREADS = atoi(optarg);
                break;

            case 'w':
                WORDS_PER_THREAD = atoi(optarg);
                break;

            case 's':
                VERBOSE = 0;
                break;
            
            case 'h':
            default:
                fprintf(
                    stderr,
                    "usage: ./wordle [-t thread] [-w words_per_thread] [-s] [-h]\n\n"

                    "-h help\n\n"

                    "-s silent mode, only print the results\n\n"

                    "-t threads\n"
                    "    max number of threads running at a time\n\n"

                    "-w words_per_thread\n"
                    "    number of words each thread should check.\n"
                    "    -w 8 would tell thread to pick 8 words and try\n"
                    "    all combinations where either of these 8 words are the word #1\n"
                );
                exit(ch == '?' ? EXIT_FAILURE : EXIT_SUCCESS);
        }
    }
}

static void print_number(unsigned long long int n) {
    if (n < 1000) {
        printf("%llu", n);
        return;
    }

    print_number(n / 1000);
    printf(",%03llu", n % 1000);
}

int main(int argc, char *argv[]) {
    struct timespec start, end;
    clock_gettime(CLOCK_REALTIME, &start);

    parse_options(argc, argv);
    thread_manager_init(MAX_THREADS);
    word_results_t word_results = load_words();

    all_words = word_results.all_words;
    word_count = word_results.word_count;

    if (VERBOSE) {
        printf(
            "Loading words done...\n"
            "Encountered %d hashmap collisions while filtering out anagrams.\n"
            "Left with %d usable words out of the %d total words in the file.\n\n",
            word_results.collisions,
            word_count,
            word_results.words_encountered
        );

        printf(
            "Starting processing: max_threads = %d, words_per_thread = %d\n\n",
            MAX_THREADS,
            WORDS_PER_THREAD
        );
    }

    /**
     * The whole search space is divided into chunks now, since we're using
     * WORDS_PER_THREAD.
     * Each thread gets passed a chunk to process, size of WORDS_PER_THREAD.
     * So we want to process all chunks an we want to keep
     * re-creating threads until all chunks are processed.
     */
    int next_chunk_index = 0;
    int total_chunks = (int) ceil((double) word_count / (double) WORDS_PER_THREAD);

    while (next_chunk_index < total_chunks) {
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

            // Last chunk
            if (arg.end >= word_count) {
                arg.end = word_count;
            }

            create_thread(arg, thread);
            next_chunk_index++;

            if (next_chunk_index == total_chunks) {
                break;
            }
        }

        mutex_unlock();
    }

    mutex_wait_for_all_threads_to_finish();
    mutex_unlock();
    cleanup_words(all_words, word_count);
    thread_manager_cleanup();

    clock_gettime(CLOCK_REALTIME, &end);

    long delta = (end.tv_sec * 1E9 + end.tv_nsec) - (start.tv_sec * 1E9 + start.tv_nsec);

    if (VERBOSE) {
        printf("\nFinished after ");
        print_number(thread_manager.work_done);
        printf(" iterations in %.2f milliseconds\n", delta / 1E6F);
    }

    return 0;
}
