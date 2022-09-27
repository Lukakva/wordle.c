#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <ctype.h>
#include <assert.h>
#include <pthread.h>

// Reallocate 128 items per realloc() call
#define WORDS_PER_ALLOC 128
#define N_MAX_THREADS 16
#define WORDS_PER_THREAD 64
#define MAX_WORDS ((1 << 27) - 1)

/**
 * @brief Represents a single word.
 */
struct word {
    // String representation of the word
    char *str;
    // Numeric representation of the word
    uint32_t numeric;

    uint16_t *allowed_words;
    uint16_t allowed_words_n;
};

typedef struct word word_t;

typedef struct {
    uint16_t start;
    uint16_t end;
} thread_arg_t;

// Count of all words
static int word_count = 0;
static word_t *all_words = NULL;
static bool anagrams[MAX_WORDS];

uint8_t number_of_bits(uint32_t n) {
    uint8_t result = 0;

    while (n > 0) {
        result += n & 1;
        n >>= 1;
    }

    return result;
}

uint32_t numeric_representation(char *word) {
    uint32_t number = 0;

    char c;
    while ((c = *word) != '\0')
    {
        // index in the alphabet
        uint8_t index = 0;
        if (isupper(c))
        {
            index = c - 65;
        }
        else
        {
            index = c - 97;
        }

        number |= 1 << index;
        word++;
    }

    return number;
}

uint8_t number_of_vowels(uint32_t word_num) {
    uint8_t count = 0;
    static uint32_t vowels[] = {
        1 << 0, // A
        1 << 4, // E
        1 << 8, // I
        1 << 14, // O
        1 << 20, // U
        1 << 24, // Y
        0, // Ending
    };
    
    uint32_t *vowel = vowels;
    while (*vowel != 0) {
        count += (word_num & *vowel) != 0;
        vowel++;
    }

    return count;
}

bool should_keep_word(uint32_t word_num) {
    // Number of unique characters has to be 5
    if (number_of_bits(word_num) != 5) {
        return false;
    }
    
    // Number of vowels
    if (number_of_vowels(word_num) >= 3) {
        return false;
    }

    if (anagrams[word_num] == true) {
        return false;
    }

    anagrams[word_num] = true;
    return true;
}

void load_words() {
    FILE *file = fopen("words.txt", "r");
    if (file == NULL)
    {
        perror("Error opening file: ");
        exit(1);
    }

    int nread;
    char *line = NULL;
    size_t lineSize = 0;

    all_words = (word_t *) malloc(WORDS_PER_ALLOC * sizeof(word_t));
    if (all_words == NULL)
    {
        perror(NULL);
        exit(1);
    }

    memset(all_words, 0, WORDS_PER_ALLOC * sizeof(word_t));
    memset(anagrams, 0, MAX_WORDS * sizeof(bool));

    int i = 0;
    // How many pointers we have allocated, not bytes, pointers
    // basically, how many strings can this array hold so far
    int allocated = WORDS_PER_ALLOC;
    while ((nread = getline(&line, &lineSize, file)) != -1)
    {
        if (line[nread - 1] == '\n')
        {
            line[nread - 1] = '\0';
        }

        
        uint32_t numeric = numeric_representation(line);
        if (!should_keep_word(numeric)) {
            continue;
        }

        // We're about to include this word as well
        if (i >= allocated)
        {
            // allocate another chunk of strings
            all_words = realloc(all_words, (allocated + WORDS_PER_ALLOC) * sizeof(word_t));
            if (all_words == NULL) {
                perror(NULL);
                exit(1);
            }

            // Initialize the new memory
            memset(all_words + allocated, 0, WORDS_PER_ALLOC * sizeof(word_t));
            allocated += WORDS_PER_ALLOC;
        }
        
        all_words[i].str = strdup(line);
        all_words[i].numeric = numeric;
        all_words[i].allowed_words = NULL;
        all_words[i].allowed_words_n = 0;

        i++;
    }

    free(line);
    all_words[i].str = NULL; // Mark the end of the words
    all_words[i].numeric = 0;
    all_words[i].allowed_words = NULL;
    all_words[i].allowed_words_n = 0;

    word_count = i;
}

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
                            "%s %s %s %s %s\n",
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

    return NULL;
}

pthread_t create_thread(int start, int end) {
    thread_arg_t *arg = (thread_arg_t *) malloc(sizeof(thread_arg_t));
    if (arg == NULL) {
        perror(NULL);
        exit(1);
    }

    arg->start = start;
    arg->end = end;

    pthread_t tid;
    if (pthread_create(&tid, NULL, &thread, arg) == -1) {
        perror(NULL);
        exit(1);
    }

    return tid;
}

int main(void) {
    load_words();

    for (int i = 0; i < word_count; i++) {
        uint16_t *possibles = (uint16_t *) malloc((word_count - i - 1) * sizeof(uint16_t));
        if (possibles == NULL) {
            perror(NULL);
            return 1;
        }

        int n = 0;
        for (int j = i + 1; j < word_count; j++) {
            if (all_words[i].numeric & all_words[j].numeric) {
                continue;
            }

            possibles[n++] = (uint16_t) j;
        }

        all_words[i].allowed_words = possibles;
        all_words[i].allowed_words_n = n;
    }

    pthread_t tids[N_THREADS];
    int batch = word_count / N_THREADS;

    for (int i = 0; i < N_THREADS; i++) {
        int start = i * batch;
        int end = start + batch;
        if (end >= word_count) {
            end = word_count;
        }

        tids[i] = create_thread(start, end);
    };

    for (int i = 0; i < N_THREADS; i++) {
        if (pthread_join(tids[i], NULL) == -1) {
            perror(NULL);
            exit(1);
        }
    }

    return 0;
}