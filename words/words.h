#include <stdlib.h>

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

void load_words(word_t **all_words, int *word_count);