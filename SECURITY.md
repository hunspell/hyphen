# Security Policy

## Supported versions

Only the latest release is maintained. Fixes go to `master`. If you are
running something older, the answer to a security report is to
upgrade.

## What is in scope

Memory safety problems reachable from ordinary use: a crash, or a read
or write outside a buffer, when hyphenating words taken from a
document, against a hyphenation dictionary installed the normal way.

## What is out of scope

1. A crafted dictionary. Dictionaries are installed data, from a
   distribution package or from an extension the user chose, and an
   extension can already run code as the user. A bug that needs a
   crafted dictionary is fixed as an ordinary bug, with no advisory
   and no CVE. If a CVE is assigned for one anyway, we will ask for it
   to be marked disputed.
2. Wrong or missing hyphenation points. Those are pattern questions and
   belong to the dictionary for the language concerned.
3. High memory use or slow hyphenation on input built to be
   pathological.
4. Bugs in the applications that embed Hyphen, including callers that
   pass smaller output buffers than hyphen.h asks for. Report those to
   the application.

One report per root cause. A variant of an issue already reported, or
of a fix under review, earns no separate credit, and a finding against
code that is in no release is fixed on master without an advisory.

## How to report

Use GitHub's private reporting: the Security tab of this repository,
then "Report a vulnerability". That keeps the report private until
there is a fix.

If that does not work for you, mail caolanm@gmail.com.

A report is most useful with the input that triggers it, the version
or commit you tested, and the sanitizer output if you have it.

## What happens next

Hyphen has one active maintainer, and there is no fixed timetable.

Most findings are fixed in a public commit once they are understood. A
finding serious enough to need handling differently is handled case by
case.

Please allow 90 days before disclosing publicly.

Reporters are credited unless they ask not to be.
