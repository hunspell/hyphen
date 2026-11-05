# Threat model

## What this project does and where untrusted input enters

Hyphen is a small C library that finds hyphenation points in words, using TeX-style hyphenation
patterns converted to a .dic file. Office suites and desktop publishing programs embed it.

The main untrusted input is the word being hyphenated: text taken from a document and passed to
hnj_hyphen_hyphenate, hnj_hyphen_hyphenate2 or hnj_hyphen_hyphenate3. A word can be any byte
sequence in the dictionary encoding, 8-bit or UTF-8, of any length; the hyphenate functions fail
for an empty word and for one over INT_MAX / 8 bytes.

Hyphenation dictionaries (hyph_*.dic) are installed data, not untrusted input. They come from a
distribution package or from an extension the user chose to install, and an office suite
extension can carry code that runs as the user, so whoever can place a dictionary on a machine
can already run programs there. The library is still hardened against malformed dictionaries and
the fuzzer feeds it malformed dictionaries, because a crash on a broken dictionary is a bug we
fix. It is not a security boundary: a bug that needs a crafted dictionary is fixed on master as
an ordinary bug, credited in the commit, with no advisory and no CVE. A dictionary pattern can
carry a non-standard replacement, such as "ff=f", so the replacement and its position and cut
values come from the dictionary too, and a word's handling of them is in scope.

## Components that matter most and least

- Matters most: hyphen.c, the library, and hnjalloc.c.
- Not security: example.c, a test driver that reads a word list, and substrings.c and
  substrings.pl, which build a .dic file from TeX patterns at packaging time. Bugs there are
  welcome as bugs.
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

- An out of bounds write, use after free or double free reached from a word with a well-formed
  dictionary: critical.
- An out of bounds read reached the same way: high.
- A null dereference, assertion failure or other clean crash reached the same way: medium.
- Anything reached only with a crafted dictionary: a bug, not a security finding, whatever the
  sanitizer prints. We still want it.
- A crash that needs an allocation to fail first, slow hyphenation, high memory use and
  timeouts: out of scope.
- Wrong or missing hyphenation points: not security issues.

## Anything to leave alone

- The caller allocates the output buffers, at the sizes that hyphen.h gives: hyphens holds
  word_size + 5 bytes, and hyphenated_word word_size * 2. A bug that needs a caller to pass
  smaller buffers than that is a bug in the caller.
- hnj_hyphen_hyphenate is the old interface, kept for compatibility. Bugs there are in scope but
  the newer functions matter more.

## How reports and patches should look

One report per root cause, with the input that triggers it and the sanitizer output; several
crashes with one cause are one report. A finding against code that is in no release is fixed on
master without an advisory. A variant of an issue already reported, or of a fix under review,
earns no separate credit. A patch should be the smallest change that fixes the bug, on master, in
the style of the surrounding code.
