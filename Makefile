LINKER_SCRIPT=linker.ld
OS_NAME=slavos
LD_ARGS=-T $(LINKER_SCRIPT) -Map=$(OS_NAME).map -z noexecstack
CC=gcc
C_ARGS=-c -nostdlib -fno-builtin -fno-stack-protector -Wall -Wextra
START_FILE=start
X86=x86
X86_64=x86_64

x86: arch/$(X86)/start.asm drivers/fb.c kernel.c
	nasm -f elf32 arch/$@/start.asm
	nasm -f elf32 frame_buf.asm
	$(CC) -m32 $(C_ARGS) kernel.c drivers/fb.c
	ld $(LD_ARGS) -melf_i386 arch/$@/start.o fb.o frame_buf.o kernel.o -o kernel.elf
	mv kernel.elf iso/$@/boot

x86_64: arch/$(X86_64)/start.asm kernel.c
	nasm -f elf64 arch/$@/start.asm
	$(CC) -m64 $(C_ARGS) kernel.c
	ld $(LD_ARGS) -melf_$@ arch/$@/start.o kernel.o -o kernel.elf
	mv kernel.elf iso/$@/boot

mkiso_x86: x86
	mkisofs -R \
		-b boot/grub/stage2_eltorito    \
		-no-emul-boot                   \
		-boot-load-size 4               \
		-A $(OS_NAME)					\
		-input-charset utf8             \
		-boot-info-table                \
		-o $(OS_NAME).iso           	\
		iso/x86

mkiso_x86_64: x86_64
	grub-mkrescue -o grub.iso iso/x86_64

run_x86: mkiso_x86
	qemu-system-i386 -enable-kvm -cdrom $(OS_NAME).iso \
			-boot menu=on -drive file=Image.img -m 1G

run_x86_64: mkiso_x86_64
	qemu-system-x86_64 -enable-kvm -cdrom $(OS_NAME).iso \
			-boot menu=on -drive file=Image.img -m 1G

clean:
	$(RM) *.o
	$(RM) *.elf
	$(RM) *.map
	$(RM) $(OS_NAME).iso
	$(RM) *.out


