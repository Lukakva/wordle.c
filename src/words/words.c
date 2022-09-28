#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "words.h"

// Reallocate 128 items per realloc() call
#define WORDS_PER_ALLOC 128

/**
 * Since we're representing words as bitfields,
 * the largest possible value a 5 digit word can achieve for a bitfield
 * is 'VWXYZ' which would set all of the last 5 bits to 1
 * therefore that's the largest possible index into our anagrams array.
 * So in total, 26 bits, left 5 set to 1.
 */
#define MAX_WORDS (0b11111 << 21)

/**
 * A bitmask of all vowels together (including Y).
 */
#define VOWELS_MASK 0b00000001000100000100000100010001

static bool anagrams[MAX_WORDS];

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

    // Two words with the same numeric representation are anagrams
    if (anagrams[word_num] == true) {
        return false;
    }

    anagrams[word_num] = true;
    return true;
}

void load_words(word_t **all_words, int *word_count) {
    FILE *file = fopen("words.txt", "r");
    if (file == NULL) {
        perror("Error opening file: ");
        exit(EXIT_FAILURE);
    }

    int nread;
    char *line = NULL;
    size_t lineSize = 0;

    word_t *words = (word_t *) malloc(WORDS_PER_ALLOC * sizeof(word_t));
    if (words == NULL) {
        perror(NULL);
        exit(EXIT_FAILURE);
    }

    memset(words, 0, WORDS_PER_ALLOC * sizeof(word_t));
    memset(anagrams, 0, MAX_WORDS * sizeof(bool));

    int i = 0;
    // How many pointers we have allocated, not bytes, pointers
    // basically, how many strings can this array hold so far
    int allocated = WORDS_PER_ALLOC;
    while ((nread = getline(&line, &lineSize, file)) != -1) {
        if (line[nread - 1] == '\n') {
            line[nread - 1] = '\0';
        }

        
        uint32_t numeric = numeric_representation(line);
        if (!should_keep_word(numeric)) {
            continue;
        }

        // We're about to include this word as well
        if (i >= allocated) {
            // allocate another chunk of words
            words = realloc(words, (allocated + WORDS_PER_ALLOC) * sizeof(word_t));
            if (words == NULL) {
                perror(NULL);
                exit(EXIT_FAILURE);
            }

            // Initialize the new memory
            memset(words + allocated, 0, WORDS_PER_ALLOC * sizeof(word_t));
            allocated += WORDS_PER_ALLOC;
        }
        
        words[i].str = strdup(line);
        words[i].numeric = numeric;
        words[i].allowed_words = NULL;
        words[i].allowed_words_n = 0;

        if (words[i].str == NULL) {
            perror("strdup");
            exit(EXIT_FAILURE);
        }

        i++;
    }

    free(line);

    words[i].str = NULL; // Mark the end of the words
    words[i].numeric = 0;
    words[i].allowed_words = NULL;
    words[i].allowed_words_n = 0;
    
    *all_words = words;
    *word_count = i;

    /**
     * Section below does the following:
     * For every word W, creates an array that stores indexes of
     * every other words that have 0 character overlap with W.
     * This way, when we're trying out all the combinations where
     * W is present, we can efficiently try out words that definitely
     * work with W.
     */
    int total = i;
    for (int i = 0; i < total; i++) {
        uint16_t *possibles = (uint16_t *) malloc((total - i - 1) * sizeof(uint16_t));
        if (possibles == NULL) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }

        int n = 0;
        for (int j = i + 1; j < total; j++) {
            // If there is a bitwise overlap, they share a character
            if (words[i].numeric & words[j].numeric) {
                continue;
            }

            // Store the index of the compatible word
            possibles[n++] = (uint16_t) j;
        }

        words[i].allowed_words = possibles;
        words[i].allowed_words_n = n;
    }
}
