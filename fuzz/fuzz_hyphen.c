/* libFuzzer harness for hyphen.
 *
 * Input format: [word bytes][NUL][dictionary bytes].
 * If no NUL is present, the whole buffer is treated as the dictionary
 * and the word is empty (parser-only path).
 *
 * The dictionary is loaded via hnj_hyphen_load_data() to avoid filesystem I/O.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hyphen.h"

#define MAX_INPUT     65536
#define MAX_WORD_SIZE  1024

static void run_hyphenate(HyphenDict *dict, const char *word, int word_size) {
    if (word_size <= 0) return;

    size_t hyphens_size  = (size_t)word_size + 5;
    size_t hyphword_size = (size_t)word_size * 2 + 1;

    /* hnj_hyphen_hyphenate2 */
    {
        char *hyphens  = (char *)malloc(hyphens_size);
        char *hyphword = (char *)malloc(hyphword_size);
        char **rep = NULL;
        int *pos = NULL;
        int *cut = NULL;

        if (hyphens && hyphword) {
            memset(hyphens,  0, hyphens_size);
            memset(hyphword, 0, hyphword_size);
            (void)hnj_hyphen_hyphenate2(dict, word, word_size,
                                        hyphens, hyphword,
                                        &rep, &pos, &cut);
            if (rep) {
                for (int i = 0; i < word_size; i++)
                    if (rep[i]) free(rep[i]);
                free(rep);
                free(pos);
                free(cut);
            }
        }
        free(hyphens);
        free(hyphword);
    }

    /* hnj_hyphen_hyphenate3 with non-trivial hyphenmin parameters to
       exercise the lhmin/rhmin/clhmin/crhmin paths. */
    {
        char *hyphens  = (char *)malloc(hyphens_size);
        char *hyphword = (char *)malloc(hyphword_size);
        char **rep = NULL;
        int *pos = NULL;
        int *cut = NULL;

        if (hyphens && hyphword) {
            memset(hyphens,  0, hyphens_size);
            memset(hyphword, 0, hyphword_size);
            (void)hnj_hyphen_hyphenate3(dict, word, word_size,
                                        hyphens, hyphword,
                                        &rep, &pos, &cut,
                                        2, 2, 2, 2);
            if (rep) {
                for (int i = 0; i < word_size; i++)
                    if (rep[i]) free(rep[i]);
                free(rep);
                free(pos);
                free(cut);
            }
        }
        free(hyphens);
        free(hyphword);
    }

    /* legacy hnj_hyphen_hyphenate */
    {
        char *hyphens = (char *)malloc(hyphens_size);
        if (hyphens) {
            memset(hyphens, 0, hyphens_size);
            (void)hnj_hyphen_hyphenate(dict, word, word_size, hyphens);
        }
        free(hyphens);
    }
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size == 0 || size > MAX_INPUT) return 0;

    const uint8_t *word_buf = NULL;
    int word_size = 0;
    const uint8_t *dict_buf = data;
    size_t dict_size = size;

    const uint8_t *nul = memchr(data, 0, size);
    if (nul) {
        size_t prefix = (size_t)(nul - data);
        if (prefix > MAX_WORD_SIZE) prefix = MAX_WORD_SIZE;
        word_buf  = data;
        word_size = (int)prefix;
        dict_buf  = nul + 1;
        dict_size = size - (size_t)(nul - data) - 1;
    }

    if (dict_size == 0) return 0;

    HyphenDict *dict = hnj_hyphen_load_data((const char *)dict_buf, dict_size);
    if (!dict) return 0;

    if (word_size > 0) {
        char *word = (char *)malloc((size_t)word_size + 1);
        if (word) {
            memcpy(word, word_buf, (size_t)word_size);
            word[word_size] = 0;
            run_hyphenate(dict, word, word_size);
            free(word);
        }
    }

    hnj_hyphen_free(dict);
    return 0;
}
