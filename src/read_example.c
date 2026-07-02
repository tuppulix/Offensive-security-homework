/*
 * read_example.c - minimal Arbitrary-Write-via-fread PoC.
 *
 * Companion to "The attacks -> Arbitrary Write (Memory Corruption via fread)".
 *
 * The program hands the attacker a write primitive over the _IO_FILE struct
 * itself: read(0, fp, sizeof(FILE)) lets stdin overwrite the structure,
 * including the buffer pointers and (on x86_64 glibc) up to the vtable field.
 * A forged structure with:
 *      _fileno      = 0                 (read from stdin)
 *      _IO_buf_base = &win_var          (target address)
 *      _IO_buf_end  = &win_var + size
 * and coherent flags (_IO_NO_READS clear, _IO_read_ptr == _IO_read_end)
 * makes the subsequent fread() write attacker bytes into win_var.
 *
 * Compile:  gcc read_example.c -o read_example.bin
 * Run:      ./read_example.bin < payload.bin
 *
 * NOTE: this is didactic. On a real glibc you must craft the flags/pointer
 * state so that fread takes the _IO_file_underflow refill branch; see the
 * report's AAW subsection for the exact preconditions.
 */
#include <stdio.h>
#include <unistd.h>

int win_var = 0;
void win(void) { puts("You Win!"); }

int main(void) {
    printf("win_var @ %p\n", (void *)&win_var);

    FILE *fp = fopen("./secret_file", "r");

    /* Arbitrary write primitive: attacker overwrites the _IO_FILE struct.
     * sizeof(FILE) ensures the vtable field is reached on x86_64 glibc. */
    read(0, fp, sizeof(FILE));

    /* fread dispatches through the corrupted struct */
    char buf[256];
    fread(buf, 1, 10, fp);

    if (win_var) win();
}
