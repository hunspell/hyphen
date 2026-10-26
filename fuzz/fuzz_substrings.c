/* libFuzzer harness for the substrings pattern converter.
 *
 * The input is a pattern file. It is written to a file next to the fuzzer's
 * working directory, because the converter reads files, and the converter's
 * main runs on it with the output sent to /dev/null. The converter exits on a
 * bad line, so exit is redirected to a long jump back into the harness.
 */

#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static jmp_buf fuzz_exit_jump;

static void fuzz_exit(int code)
{
    (void) code;
    longjmp(fuzz_exit_jump, 1);
}

#define exit(code) fuzz_exit(code)
#define main substrings_main
#include "../substrings.c"
#undef main
#undef exit

static char fuzz_input_path[64];

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FILE *f;
    const char *argv[3];

    if (size > 65536) return 0;
    if (!fuzz_input_path[0])
        snprintf(fuzz_input_path, sizeof(fuzz_input_path), "fuzz_substrings.%ld.pat", (long) getpid());

    f = fopen(fuzz_input_path, "wb");
    if (!f) return 0;
    fwrite(data, 1, size, f);
    fclose(f);

    argv[0] = "substrings";
    argv[1] = fuzz_input_path;
    argv[2] = "/dev/null";
    if (setjmp(fuzz_exit_jump) == 0)
        substrings_main(3, argv);
    return 0;
}
