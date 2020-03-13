#!/bin/bash

date "+%Y%m%d"
echo "=================="

echo -e "\n>> PART_1 mbr"
nasm -o build/mbr.bin boot/c05a_mbr.asm
dd if=build/mbr.bin of=hd60M.img bs=512 count=1 conv=notrunc

echo -e "\n>> PART_2 loader"
nasm -I ./include/ -o build/loader.bin boot/c05c_loader.asm
dd if=build/loader.bin of=hd60M.img bs=512 count=4 seek=2 conv=notrunc

echo -e "\n>> PART_3 lib"
nasm -f elf -o build/print.o lib/print.asm

echo -e "\n>> PART_4 device"
gcc -m32 -I lib/ -c -o build/timer.o device/c07c_timer.c

echo -e "\n>> PART_5 kernel"
nasm -f elf -o build/core_interrupt.o kernel/c07b_core_interrupt.asm
gcc -m32 -I lib/ -I kernel/ -c -fno-builtin -o build/interrupt.o kernel/c07b_interrupt.c

gcc -m32 -I lib/ -I kernel/ -I device/ -c -fno-builtin -o build/init.o kernel/c07a_init.c

gcc -m32 -I lib/ -c -fno-builtin -o build/main.o main.c
ld -m elf_i386 -Ttext 0xc0001500 -e main -o build/kernel.bin build/main.o  build/init.o build/interrupt.o build/print.o build/core_interrupt.o build/timer.o

dd if=build/kernel.bin of=hd60M.img bs=512 count=200 seek=9 conv=notrunc

if [ $? -eq 0 ]
then
    echo -e "\nsuccess!"
else
    echo -e "\nfail!"
fi
