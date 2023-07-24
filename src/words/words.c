#include "words.h"
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Reallocate 128 items per realloc() call
#define WORDS_PER_ALLOC 128

/**
 * A bitmask of all vowels together (including Y).
 */
#define VOWELS_MASK 0b00000001000100000100000100010001

#define HASHMAP_SIZE 16 // 2^16 elements

/**
 * 5351 was randomly chosen and it worked!
 * Multiply x by 5351 and take the last 16 bits.
 * Then take the first 16 bits of x and XOR them with the result above.
 */
#define hash32(x) (uint16_t) (((x * 5351) & 0xffff) ^ ((x & 0xffff0000) >> 16))

/**
 * Used to filter out anagrams.
 */
static uint32_t hashmap[1 << HASHMAP_SIZE];
static int collisions = 0;

/**
 * Calculates number of set bits (1s) in a number.
 * Brian Kernighan's Algorithm.
 * 
 * @param n The number.
 * @return uint8_t
 */
static uint8_t number_of_bits(uint32_t n) {
    uint8_t result = 0;

    while (n) {
        n = n & (n - 1);
        result++;
    }

    return result;
}

/**
 * Creates a bitfield which holds information about
 * which characters the word uses.
 *
 * Since there are 26 characters in the English alphabet, a 32bit
 * number can hold information about what characters are used in a word.
 * a = 1st bit set to 1
 * b = 2nd bit set to 1
 * c = 3rd bit set to 1
 * ...
 * 
 * @param word a string
 * @return uint32_t
 */
static uint32_t numeric_representation(char *word) {
    uint32_t number = 0;

    char c;
    while ((c = *word) != '\0')
    {
        // c - 97 is the index of this character in the alphabet, 0-25
        // set that bit to 1
        number |= 1 << (c - 97);
        word++;
    }

    return number;
}

static bool should_keep_word(uint32_t word_num) {
    // Number of unique characters has to be 5
    if (number_of_bits(word_num) != 5) {
        return false;
    }
    
    // Number of vowels
    if (number_of_bits(word_num & VOWELS_MASK) >= 3) {
        return false;
    }

    unsigned int hashmap_key = hash32(word_num);
    // linear probing
    while (hashmap_key < (1 << HASHMAP_SIZE)) {
        // Empty space, store this number and keep the word
        if (hashmap[hashmap_key] == 0) {
            hashmap[hashmap_key] = word_num;
            return true;
        }

        // Word was found in the hashmap
        if (hashmap[hashmap_key] == word_num) {
            // This word should not be kept, since there is an anagram for it already
            return false;
        }

        // Keep searching, linear probing
        hashmap_key++;
        collisions++;
    }

    fprintf(stderr, "Failed to insert %d into a hashmap %u %u\n", word_num, hash32(word_num), hashmap_key);
    exit(EXIT_FAILURE);
}

int compare_words(const void *a, const void *b) {
    word_t *word_a = (word_t *) a;
    word_t *word_b = (word_t *) b;

    return (int) word_a->numeric - (int) word_b->numeric;
}

word_results_t load_words() {
    FILE *file = fopen("words.txt", "r");
    if (file == NULL) {
        perror("Error opening file: ");
        exit(EXIT_FAILURE);
    }

    word_t *words = (word_t *) calloc(WORDS_PER_ALLOC, sizeof(word_t));
    if (words == NULL) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    memset(words, 0, WORDS_PER_ALLOC * sizeof(word_t));
    memset(hashmap, 0, sizeof(hashmap));

    int i = 0;
    // How many pointers we have allocated, not bytes, pointers
    // basically, how many strings can this array hold so far
    int allocated = WORDS_PER_ALLOC;
    // Total words encountered in a file, even unused ones
    int total_words = 0;

    char line[6];
    while (!feof(file)) {
        size_t nread = fread(line, sizeof(char), sizeof(line), file);
        if (line[nread - 1] == '\n') {
            line[nread - 1] = '\0';
        }
        
        total_words++;
        uint32_t numeric = numeric_representation(line);
        if (!should_keep_word(numeric)) {
            continue;
        }

        // We're about to include this word as well
        if (i >= allocated) {
            // allocate another chunk of words
            words = realloc(words, (allocated + WORDS_PER_ALLOC) * sizeof(word_t));
            if (words == NULL) {
                perror("realloc");
                exit(EXIT_FAILURE);
            }

            // Initialize the new memory
            memset(words + allocated, 0, WORDS_PER_ALLOC * sizeof(word_t));
            allocated += WORDS_PER_ALLOC;
        }
        
        words[i].str = strdup(line);
        words[i].numeric = numeric;
        words[i].neighbors = NULL;
        words[i].neighbors_n = 0;

        if (words[i].str == NULL) {
            perror("strdup");
            exit(EXIT_FAILURE);
        }

        i++;
    }

    int total = i;
    /**
     * Sort based on the numeric representation of the numbers.
     */
    qsort(words, total, sizeof(word_t), compare_words);

    /**
     * Section below does the following:
     * For every word W, creates an array that stores indexes of
     * every other words that have 0 character overlap with W.
     * This way, when we're trying out all the combinations where
     * W is present, we can efficiently try out words that definitely
     * work with W.
     */
    for (int i = 0; i < total; i++) {
        uint16_t *neighbors = (uint16_t *) calloc(total - i - 1, sizeof(uint16_t));

        int n = 0;
        for (int j = i + 1; j < total; j++) {
            // If there is a bitwise overlap, they share a character
            if (words[i].numeric & words[j].numeric) {
                continue;
            }

            // Store the index of the compatible word
            neighbors[n++] = (uint16_t) j;
        }

        words[i].neighbors = neighbors;
        words[i].neighbors_n = n;
    }

    word_results_t results = {
        .all_words = words,
        .word_count = total,
        .words_encountered = total_words,
        .collisions = collisions
    };

    return results;
}

void cleanup_words(word_t *all_words, int word_count) {
    for (int i = 0; i < word_count; i++) {
        // Free up the strings
        free(all_words[i].str);
    }

    free(all_words);
}