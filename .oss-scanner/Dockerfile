# OSS Scanner build for Hyphen (https://github.com/hunspell/hyphen).
#
# Two builds of the same checkout:
#
#   /src/build-asan   clang, AddressSanitizer and UndefinedBehaviorSanitizer, -O1 -g. The library and the
#                     example program, with "make check" run under the sanitizers.
#   /src/fuzz         fuzz_hyphen, the libFuzzer target in fuzz/fuzz_hyphen.c, built with the same
#                     sanitizers by fuzz/build.sh. It runs the files named on its command line and exits.
#   /src              a plain -O2 -g build with frame pointers, installed to /usr/local, with "make check"
#                     run.
FROM ubuntu:24.04
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
      ca-certificates git make autoconf automake libtool perl \
      clang libclang-rt-18-dev llvm gcc gdb valgrind \
    && rm -rf /var/lib/apt/lists/*

# scanner contract: the checkout lives inside the image, at /src
COPY . /src
WORKDIR /src

RUN autoreconf -vfi

# Sanitizer build, out of tree so the plain build below can configure in place.
RUN mkdir /src/build-asan && cd /src/build-asan \
    && CC=clang \
       CFLAGS="-O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined -fno-sanitize-recover=undefined" \
       LDFLAGS="-fsanitize=address,undefined" \
       ../configure --disable-shared --enable-static \
    && make -j"$(nproc)" \
    && make -j"$(nproc)" check

RUN sh fuzz/build.sh

# Plain build, as distributions build it.
RUN CFLAGS="-O2 -g -fno-omit-frame-pointer" ./configure \
    && make -j"$(nproc)" && make install && ldconfig \
    && make -j"$(nproc)" check
