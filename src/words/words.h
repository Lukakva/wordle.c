#include <stdint.h>
#include <stdbool.h>

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
    bool *neighbors_lookup;
    
    /** Length of said array. */
    uint16_t neighbors_n;

} word_t;

/**
 * A struct that is encapsulates the results from the `load_words` function call.
 */
typedef struct {
    /**
     * A dynamically allocated array of all the words.
     * Includes only filtered words.
     */
    word_t *all_words;

    /** Word count. */
    int word_count;

    /** Total words encountered in the file. */
    int words_encountered;

    /**
     * Hashmap collisions when filtering out the anagrams.
     */
    int collisions;
} word_results_t;

/**
 * Reads words from a file, allocates memory for them,
 * filters them so that there are no duplicate letter words,
 * and no anagrams, and generates their numeric representations
 * to simplify checking for overlaps.
 */
word_results_t load_words();

/**
 * Cleanup.
 *
 * @param all_words Array of all words.
 * @param word_count Word count.
 */
void cleanup_words(word_t *all_words, int word_count);
