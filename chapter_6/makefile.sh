#!/bin/bash

date "+%Y%m%d"
echo "========="

echo -e "\n>> PART_1 mbr"
nasm -o mbr.bin c05a_mbr.asm
dd if=mbr.bin of=hd60M.img bs=512 count=1 conv=notrunc

echo -e "\n>> PART_2 loader"
nasm -I ./include/ -o loader.bin c05c_loader.asm
dd if=loader.bin of=hd60M.img bs=512 count=4 seek=2 conv=notrunc

echo -e "\n>> PART_3 lib"
nasm -f elf -o lib/print.o lib/print.asm

echo -e "\n>> PART_4 kernel"
gcc -m32 -I lib/ -c -o main.o main.c
ld -m elf_i386 -Ttext 0xc0001500 -e main -o kernel.bin main.o lib/print.o
dd if=kernel.bin of=hd60M.img bs=512 count=200 seek=9 conv=notrunc

echo -e "\nsuccess!"
