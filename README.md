# FSOP Overview - Replication Package

---

## 1. Contents

```
replication_package/
|-- README.md                  <- this file
|-- src/                       <- the C snippets shown in the "Technical background" section
|   |-- read_loop.c            <- 50k raw read() syscalls
|   |-- fread_loop.c           <- same workload via buffered fread()
|   `-- read_example.c         <- Arbitrary Write via fread() minimal PoC
|-- exploit/
|   `-- exploit.py             <- full FSOP exploit for `fakev` (pwntools)
`-- challenge/
    |-- fakev                  <- target ELF (included)
    `-- libc.so.6              <- Ubuntu GLIBC 2.27-3ubuntu1 (included)
```

## 2. Challenge files and provenance

The target binary and its C library are included under `challenge/`:

| File          | What it is                              | SHA-256 (first 16)  |
|---------------|-----------------------------------------|---------------------|
| `fakev`       | target ELF (No PIE, Partial RELRO, not stripped) | `0e763d53f0c2fd3f` |
| `libc.so.6`   | `Ubuntu GLIBC 2.27-3ubuntu1`            | `cd7c1a035d241227`  |
| `ld-2.27.so`  | matching loader - **not bundled**; `pwninit` fetches it automatically, or copy it from an Ubuntu 18.04 host | - |

Provenance / attribution:
- Challenge: OliCyber training #180, originally m0leCon `fakev` (Politecnico di Torino) - <https://training.olicyber.it/challenges#challenge-180>
- Reference write-up the files were sourced from: <https://ctftime.org/writeup/20702>

Verify you have the right `libc` before running anything:

```bash
strings challenge/libc.so.6 | grep "GNU C Library"
# expected: GNU C Library (Ubuntu GLIBC 2.27-3ubuntu1) stable release version 2.27.
```

If that string differs, **the offsets in `exploit.py` will not match** - see section 5.

## 3. Reproducing the "Technical background" measurements

Any Linux with `gcc` works. Build and time the two loops:

```bash
gcc src/read_loop.c    -o read_loop.bin
gcc src/fread_loop.c   -o fread_loop.bin
gcc src/read_example.c -o read_example.bin

time ./read_loop.bin       # ~50000 read() syscalls  -> confirm with: strace -c ./read_loop.bin
time ./fread_loop.bin      # buffered, far fewer syscalls
```

`read_example.bin` demonstrates the Arbitrary-Write-via-`fread` primitive:

```bash
# feed a forged _IO_FILE on stdin; see src/read_example.c for the layout
./read_example.bin < payload.bin
```

## 4. Reproducing the exploit

Requires `python3` and `pwntools` (`pip install pwntools`). For a **remote** run only
`fakev` and `libc.so.6` are needed - the script merely reads their symbols.

```bash
# remote service (default endpoint is baked into exploit.py):
python3 exploit/exploit.py
# override the endpoint if it changes:
HOST=fakev.challs.olicyber.it PORT=11004 python3 exploit/exploit.py

# local process instead (needs pwninit to link fakev against the shipped libc/ld):
cd challenge && pwninit --bin fakev --libc libc.so.6 --ld ld-2.27.so
python3 ../exploit/exploit.py LOCAL
```

**This exploit has been verified end-to-end against the live service**: it leaks the
`libc` base via the Use-After-Read, forges an `_IO_FILE` in `.bss`, shifts the vtable
onto `_IO_str_overflow`, and `fclose()` lands on `system("/bin/sh")`, giving a shell
that reads the flag:

```
ptm{pl4y1ng_w17h_5t4cks_4nd_f1l3s_f0r_fun_4nd_pr0f}
```

(The live instance appends a per-session suffix to the flag.)

## 5. Re-deriving the offsets (if your environment differs)

The offsets can be recomputed so the package stays valid against a slightly
different build. **Caveat:** the bundled `libc.so.6` is *stripped* (only `.dynsym`),
so `main_arena` and the `_IO_*_jumps` symbols cannot be resolved by name from it -
use the matching **system** libc with debug symbols (Ubuntu 18.04), or trust the
numeric offsets, which are fixed for `2.27-3ubuntu1`:

```bash
# main_arena displacement from libc base (0x3ebca0 = main_arena+96 for 2.27-3ubuntu1)
# needs a libc WITH .symtab (system libc / libc6-dbg), not the stripped bundled one:
readelf -s /usr/lib/debug/.../libc.so.6 | grep -w main_arena   # then add 96 (0x60)

# _IO_file_jumps / _IO_str_jumps and their 0x3a8 distance (same requirement):
objdump -t /lib/x86_64-linux-gnu/libc.so.6 | grep -E "_IO_file_jumps|_IO_str_jumps"
gdb -q challenge/fakev -ex 'p/x (long)&_IO_file_jumps - (long)&_IO_str_jumps' -ex quit

# static .bss buffer used by get_int (No PIE => fixed address). The relevant
# symbol is choice_string @ 0x602100; the exploit anchors at choice_string + 8:
readelf -s challenge/fakev | grep -w choice_string
```

Update the constants at the top of `exploit/exploit.py` accordingly.

## 6. Mapping to the report

| Report section                                   | Artifact here                          |
|--------------------------------------------------|----------------------------------------|
| Technical background (buffering overhead)        | `src/read_loop.c`, `src/fread_loop.c`  |
| The attacks -> Arbitrary Write via `fread`        | `src/read_example.c`                   |
| Attack examples: `fakev` (full chain)            | `exploit/exploit.py`                   |
| Reconnaissance (glibc version, checksec)         | section 2 / section 5 commands above                 |
| Reproducibility of offsets (review note #9)      | section 5 + bundled `fakev` / `libc.so.6`     |
