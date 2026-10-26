/* libFuzzer harness for hyphen.
 *
 * Input format: [word bytes][NUL][dictionary bytes].
 * If no NUL is present, the whole buffer is treated as the dictionary
 * and the word is empty (parser-only path).
 *
 * The dictionary is loaded via hnj_hyphen_load_data() to avoid filesystem I/O.
 *
 * The word goes to the library in a buffer of exactly word_size bytes with no
 * terminator, and every output buffer has exactly the size hyphen.h documents,
 * so AddressSanitizer reports a read or write one byte past any of them. The
 * replacement arrays the library hands back are checked against the contract a
 * caller relies on: the change starts inside the word and cuts no more than the
 * rest of it.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hyphen.h"

#define MAX_INPUT     65536
#define MAX_WORD_SIZE  1024

/* the number of characters in the first n bytes of the word, counted the way
   hnj_hyphen_norm counts them */
static int count_chars(const char *word, int n, int utf8)
{
    int i, chars = 0;
    if (!utf8) return n;
    for (i = 0; i < n; i++)
        if ((((unsigned char) word[i]) >> 6) != 2) chars++;
    return chars;
}

static void free_replacements(int word_size, char **rep, int *pos, int *cut)
{
    int i;
    if (rep) {
        for (i = 0; i < word_size; i++) free(rep[i]);
        free(rep);
    }
    free(pos);
    free(cut);
}

/* hyphens is a string inside its word_size + 5 bytes, and a caller applies
   rep[i] by stepping back pos[i] characters from the hyphenation point and
   removing cut[i] characters from there, so both stay inside the word */
static void check_result(const char *word, int word_size, int utf8, const char *hyphens,
                         char **rep, int *pos, int *cut)
{
    int n = count_chars(word, word_size, utf8);
    int i;
    if (strnlen(hyphens, (size_t) word_size + 5) >= (size_t) word_size + 5) __builtin_trap();
    if (!rep || !pos || !cut) return;
    for (i = 0; i < n; i++) {
        int start;
        if (!rep[i]) continue;
        start = i - pos[i] + 1;
        if (start < 0 || start > n) __builtin_trap();
        if (cut[i] < 0 || start + cut[i] > n) __builtin_trap();
    }
}

static void check_hyphword(const char *hyphword, int word_size)
{
    if (strnlen(hyphword, (size_t) word_size * 2) >= (size_t) word_size * 2) __builtin_trap();
}

static void run_hyphenate(HyphenDict *dict, const uint8_t *data, int word_size)
{
    size_t hyphens_size  = (size_t) word_size + 5;
    size_t hyphword_size = (size_t) word_size * 2;
    unsigned char p = word_size > 0 ? data[0] : 0;
    char *word;
    char *hyphens  = (char *) malloc(hyphens_size);
    char *hyphword;
    char **rep;
    int *pos;
    int *cut;
    int ret;

    if (!hyphens) return;

    if (word_size == 0) {
        /* an empty word fails before anything is written */
        rep = NULL; pos = NULL; cut = NULL;
        if (hnj_hyphen_hyphenate2(dict, "", 0, hyphens, NULL, &rep, &pos, &cut) != 1) __builtin_trap();
        if (hnj_hyphen_hyphenate3(dict, "", 0, hyphens, NULL, &rep, &pos, &cut, 2, 2, 2, 2) != 1) __builtin_trap();
        if (rep || pos || cut) __builtin_trap();
        if (hnj_hyphen_hyphenate(dict, "", 0, hyphens) == 0) check_result("", 0, 0, hyphens, NULL, NULL, NULL);
        free(hyphens);
        return;
    }

    /* exactly word_size bytes, no terminator */
    word = (char *) malloc((size_t) word_size);
    hyphword = (char *) malloc(hyphword_size);
    if (!word || !hyphword) {
        free(word);
        free(hyphword);
        free(hyphens);
        return;
    }
    memcpy(word, data, (size_t) word_size);

    /* hnj_hyphen_hyphenate2, the library allocates the arrays */
    rep = NULL; pos = NULL; cut = NULL;
    memset(hyphens, 0, hyphens_size);
    memset(hyphword, 0, hyphword_size);
    ret = hnj_hyphen_hyphenate2(dict, word, word_size, hyphens, hyphword, &rep, &pos, &cut);
    if (ret == 0) {
        check_result(word, word_size, dict->utf8, hyphens, rep, pos, cut);
        check_hyphword(hyphword, word_size);
    }
    free_replacements(word_size, rep, pos, cut);

    /* hnj_hyphen_hyphenate3 with hyphenmin values taken from the first byte of
       the word, and the caller's own zeroed arrays */
    rep = (char **) calloc((size_t) word_size, sizeof(char *));
    pos = (int *) calloc((size_t) word_size, sizeof(int));
    cut = (int *) calloc((size_t) word_size, sizeof(int));
    if (rep && pos && cut) {
        memset(hyphens, 0, hyphens_size);
        memset(hyphword, 0, hyphword_size);
        ret = hnj_hyphen_hyphenate3(dict, word, word_size, hyphens, hyphword, &rep, &pos, &cut,
                                    p & 7, (p >> 3) & 7, (p >> 6) & 3, (p >> 1) & 3);
        if (ret == 0) {
            check_result(word, word_size, dict->utf8, hyphens, rep, pos, cut);
            check_hyphword(hyphword, word_size);
        }
    }
    free_replacements(word_size, rep, pos, cut);

    /* hnj_hyphen_hyphenate2 with no hyphword and a mixed set: rep still NULL,
       pos and cut given by the caller */
    rep = NULL;
    pos = (int *) calloc((size_t) word_size, sizeof(int));
    cut = (int *) calloc((size_t) word_size, sizeof(int));
    if (pos && cut) {
        memset(hyphens, 0, hyphens_size);
        ret = hnj_hyphen_hyphenate2(dict, word, word_size, hyphens, NULL, &rep, &pos, &cut);
        if (ret == 0) check_result(word, word_size, dict->utf8, hyphens, rep, pos, cut);
    }
    free_replacements(word_size, rep, pos, cut);

    /* hnj_hyphen_hyphenate3 with NULL array pointers */
    memset(hyphens, 0, hyphens_size);
    memset(hyphword, 0, hyphword_size);
    ret = hnj_hyphen_hyphenate3(dict, word, word_size, hyphens, hyphword, NULL, NULL, NULL, 2, 2, 2, 2);
    if (ret == 0) {
        check_result(word, word_size, dict->utf8, hyphens, NULL, NULL, NULL);
        check_hyphword(hyphword, word_size);
    }

    /* legacy hnj_hyphen_hyphenate */
    memset(hyphens, 0, hyphens_size);
    ret = hnj_hyphen_hyphenate(dict, word, word_size, hyphens);
    if (ret == 0) check_result(word, word_size, 0, hyphens, NULL, NULL, NULL);

    free(word);
    free(hyphens);
    free(hyphword);
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size == 0 || size > MAX_INPUT) return 0;

    const uint8_t *word_buf = NULL;
    int word_size = 0;
    int have_word = 0;
    const uint8_t *dict_buf = data;
    size_t dict_size = size;

    const uint8_t *nul = memchr(data, 0, size);
    if (nul) {
        size_t prefix = (size_t)(nul - data);
        if (prefix > MAX_WORD_SIZE) prefix = MAX_WORD_SIZE;
        word_buf  = data;
        word_size = (int)prefix;
        have_word = 1;
        dict_buf  = nul + 1;
        dict_size = size - (size_t)(nul - data) - 1;
    }

    if (dict_size == 0) return 0;

    HyphenDict *dict = hnj_hyphen_load_data((const char *)dict_buf, dict_size);
    if (!dict) return 0;

    if (have_word) run_hyphenate(dict, word_buf, word_size);

    hnj_hyphen_free(dict);
    return 0;
}
