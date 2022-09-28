#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "words.h"

// Reallocate 128 items per realloc() call
#define WORDS_PER_ALLOC 128
#define MAX_WORDS ((1 << 27) - 1)

static bool anagrams[MAX_WORDS];

static uint8_t number_of_bits(uint32_t n) {
    uint8_t result = 0;

    while (n > 0) {
        result += n & 1;
        n >>= 1;
    }

    return result;
}

static uint32_t numeric_representation(char *word) {
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

static uint8_t number_of_vowels(uint32_t word_num) {
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

static bool should_keep_word(uint32_t word_num) {
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

void load_words(word_t **all_words, int *word_count) {
    FILE *file = fopen("words.txt", "r");
    if (file == NULL)
    {
        perror("Error opening file: ");
        exit(EXIT_FAILURE);
    }

    int nread;
    char *line = NULL;
    size_t lineSize = 0;

    word_t *words = (word_t *) malloc(WORDS_PER_ALLOC * sizeof(word_t));
    if (words == NULL)
    {
        perror(NULL);
        exit(EXIT_FAILURE);
    }

    memset(words, 0, WORDS_PER_ALLOC * sizeof(word_t));
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

        i++;
    }

    free(line);
    words[i].str = NULL; // Mark the end of the words
    words[i].numeric = 0;
    words[i].allowed_words = NULL;
    words[i].allowed_words_n = 0;
    
    *all_words = words;
    *word_count = i;

    int total = i;
    for (int i = 0; i < total; i++) {
        uint16_t *possibles = (uint16_t *) malloc((total - i - 1) * sizeof(uint16_t));
        if (possibles == NULL) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }

        int n = 0;
        for (int j = i + 1; j < total; j++) {
            if (words[i].numeric & words[j].numeric) {
                continue;
            }

            possibles[n++] = (uint16_t) j;
        }

        words[i].allowed_words = possibles;
        words[i].allowed_words_n = n;
    }
}
