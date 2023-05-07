LINKER_SCRIPT=linker.ld
OS_NAME=slavos
START32=startx32
START64=startx32

all: x32

x32:
	nasm -f elf32 $(START32).asm
	ld -T $(LINKER_SCRIPT) -melf_i386 $(START32).o -o kernel.elf
	mv kernel.elf iso/boot

run_x32:
	qemu-system-i386 -enable-kvm -cdrom $(OS_NAME).iso  -boot menu=on -drive file=../../qemu/Image.img -m 1G

x64:
	nasm -f elf64 $(START64).asm
	ld -T $(LINKER_SCRIPT) -melf_x86_64 $(START64).o -o kernel.elf
	mv kernel.elf iso/boot

run_x64:
	qemu-system-x86_64 -enable-kvm -cdrom $(OS_NAME).iso  -boot menu=on -drive file=../../qemu/Image.img -m 1G

mkiso:
	mkisofs -R \
		-b boot/grub/stage2_eltorito    \
		-no-emul-boot                   \
		-boot-load-size 4               \
		-A $(OS_NAME)					\
		-input-charset utf8             \
		-boot-info-table                \
		-o $(OS_NAME).iso           	\
		iso

clean:
	$(RM) *.o
	$(RM) *.elf
	$(RM) $(OS_NAME).iso


