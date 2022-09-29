#include <stdlib.h>

/**
 * Represents a single word.
 */
typedef struct {
    /** String representation of the word. */
    char *str;

    /** Numeric representation of the word. */
    uint32_t numeric;

    /**
     * Array containing indexes of all the words that have no character
     * overlap with this word.
     */
    uint16_t *neighbors;
    
    /** Length of said array. */
    uint16_t neighbors_n;
} word_t;

/**
 * Reads words from a file, allocates memory for them,
 * filters them so that there are no duplicate letter words,
 * and no anagrams, and generates their numeric representations
 * to simplify checking for overlaps.
 * 
 * @param all_words Pointer to an array. A pointer to the allocated array will be put here. Like getline()
 * @param word_count Pointer to a number. Number of words will be put in here.
 */
void load_words(word_t **all_words, int *word_count);
