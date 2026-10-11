# Threat model

## What this project does and where untrusted input enters

Hyphen is a small C library that finds hyphenation points in words, using TeX-style hyphenation
patterns converted to a .dic file. Office suites and desktop publishing programs embed it.

The main untrusted input is the word being hyphenated: text taken from a document and passed to
hnj_hyphen_hyphenate, hnj_hyphen_hyphenate2 or hnj_hyphen_hyphenate3. A word can be any byte
sequence in the dictionary encoding, 8-bit or UTF-8, of any length.

Hyphenation dictionaries (hyph_*.dic) are a weaker boundary. They are normally installed from a
distribution package or an extension, so a malicious dictionary needs the user to install it.
Memory safety bugs in dictionary loading (hnj_hyphen_load, hnj_hyphen_load_file and
hnj_hyphen_load_data) are still in scope, at a lower severity than the same bug reached from a
word. A dictionary pattern can carry a non-standard replacement, such as "ff=f", so the
replacement and its position and cut values come from the dictionary too.

## Components that matter most and least

- Matters most: hyphen.c, the library, and hnjalloc.c.
- Lower: example.c, a demonstration program that reads a word list, and substrings.c and
  substrings.pl, which build a .dic file from TeX patterns at packaging time.
- Out of scope: the en_US pattern sources (ushyphmax.tex, tbhyphext.tex) and the documentation.

## How to exercise it

- /src/fuzz/fuzz_hyphen is the libFuzzer target, built with AddressSanitizer and
  UndefinedBehaviorSanitizer. Its input is a word, a NUL byte, then a dictionary. Without a NUL
  byte the whole input is a dictionary and only loading is tested. It runs the files named on its
  command line, or fuzzes when given a corpus directory.
- /src/build-asan/example is the example program built with the same sanitizers:
  "example [-d | -dd] <dictionary> <file of words>".
- /src/tests holds small pattern files with words and expected results. "make check" in /src and
  in /src/build-asan runs them.
- /usr/local/share/hyphen/hyph_en_US.dic is a full real-world dictionary.

## How we rate severity

- An out of bounds write, use after free or double free reached from a word: critical.
- An out of bounds read reached from a word: high.
- The same bugs reached only through a crafted dictionary: one level lower.
- A null dereference, assertion failure or other clean crash: medium from a word, low from a
  dictionary.
- Slow hyphenation, high memory use and timeouts: out of scope.
- Wrong or missing hyphenation points: not security issues.

## Anything to leave alone

- The caller allocates the output buffers, at the sizes that hyphen.h gives: hyphens holds
  word_size + 5 bytes, and hyphenated_word about word_size * 2. A bug that needs a caller to pass
  smaller buffers than that is a bug in the caller.
- hnj_hyphen_hyphenate is the old interface, kept for compatibility. Bugs there are in scope but
  the newer functions matter more.

## How reports and patches should look

A reproducer file and the sanitizer output are the most useful parts. A patch should be the
smallest change that fixes the bug, on master, in the style of the surrounding code.
