LINKER_SCRIPT=linker.ld
OS_NAME=slavos
LINK_ARGS=-T $(LINKER_SCRIPT) -Map=$(OS_NAME).map -z noexecstack
CC=gcc
CC_ARGS=-c -nostdlib -nostdinc -fno-builtin -fno-stack-protector
START32=startx32
START64=startx32
ISO32=iso_x86
ISO64=iso

x86: $(START32).asm kernel.c
	nasm -f elf32 $(START32).asm
	$(CC) -m32 $(CC_ARGS) kernel.c
	ld $(LINK_ARGS) -melf_i386 $(START32).o kernel.o -o kernel.elf
	mv kernel.elf $(ISO32)/boot

x86_64: $(START64).asm kernel.c
	nasm -f elf64 $(START64).asm
	$(CC) -m64 $(CC_ARGS) kernel.c
	ld $(LINK_ARGS) -melf_x86_64 $(START64).o kernel.o -o kernel.elf
	mv kernel.elf $(ISO64)/boot

run_x86: mkiso
	qemu-system-i386 -enable-kvm -cdrom $(OS_NAME).iso  -boot menu=on -drive file=../../qemu/Image.img -m 1G

run_x86_64: grubiso
	qemu-system-x86_64 -enable-kvm -cdrom $(OS_NAME).iso  -boot menu=on -drive file=../../qemu/Image.img -m 1G

grubiso: x86_64
	grub-mkrescue -o grub.iso iso

run_grubiso: grubiso
	qemu-system-x86_64 -enable-kvm -cdrom grub.iso  -boot menu=on -drive file=../../qemu/Image.img -m 1G

mkiso: x86
	mkisofs -R \
		-b boot/grub/stage2_eltorito    \
		-no-emul-boot                   \
		-boot-load-size 4               \
		-A $(OS_NAME)					\
		-input-charset utf8             \
		-boot-info-table                \
		-o $(OS_NAME).iso           	\
		$(ISO32)

clean:
	$(RM) *.o
	$(RM) *.elf
	$(RM) *.map
	$(RM) $(OS_NAME).iso


