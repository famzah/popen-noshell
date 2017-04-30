#!/bin/bash
set -u

nasm -f elf64 wrapper.asm && ld -s -o wrapper-asm wrapper.o ; rm -f wrapper.o
for CC in gcc musl-gcc ; do
	"$CC" -O2         -Wall -Wl,--strip-all -Wl,--gc-sections wrapper.c -o "wrapper-${CC}-std"
	"$CC" -O2 -static -Wall -Wl,--strip-all -Wl,--gc-sections wrapper.c -o "wrapper-${CC}-static"
done
