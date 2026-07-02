/*
 * read_loop.c - baseline: 50,000 raw read() syscalls.
 *
 * Companion to the "Technical background" section of the FSOP report.
 * Compile:  gcc read_loop.c -o read_loop.bin
 * Measure:  time ./read_loop.bin
 * Inspect:  strace -c ./read_loop.bin   (expect ~50000 read() calls)
 */
#include <unistd.h>
#include <fcntl.h>

int main() {
    char buf[0x1000];
    int fd = open("/dev/urandom", O_RDONLY);
    for (int i = 0; i < 50000; i++) {
        read(fd, buf, 0x20);
    }
    return 0;
}
