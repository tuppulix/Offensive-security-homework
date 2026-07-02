/*
 * fread_loop.c - same workload as read_loop.c, but through glibc's
 * buffered fread(). The internal _IO_FILE buffer (~one page) collapses the
 * 50,000 logical reads into far fewer real syscalls.
 *
 * Companion to the "Technical background" section of the FSOP report.
 * Compile:  gcc fread_loop.c -o fread_loop.bin
 * Measure:  time ./fread_loop.bin
 * Inspect:  strace -c ./fread_loop.bin   (expect far fewer than 50000 reads)
 */
#include <stdio.h>

int main() {
    char buf[0x1000];
    FILE *fp = fopen("/dev/urandom", "r");

    if (fp != NULL) {
        for (int i = 0; i < 50000; i++) {
            fread(buf, 1, 0x20, fp);
        }
        fclose(fp);
    }

    return 0;
}
