#!/bin/bash

nasm -f elf64 tiny2.asm && ld -s -o tiny2 tiny2.o && rm tiny2.o
