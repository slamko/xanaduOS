include define.mk

include drivers/Makefile

kernel.elf: arch/$(ARCH)/start.asm drivers_objs kernel.c
	nasm -f elf32 arch/$(ARCH)/start.asm
	nasm -f elf32 drivers/fb_lib.asm
	$(CC) -m32 $(C_ARGS) kernel.c 
	ld $(LD_ARGS) -melf_$(ARCH) arch/$(ARCH)/start.o fb.o drivers/fb_lib.o \
		kernel.o -o iso/$(ARCH)/boot/kernel.elf

x86: ARCH=$(X86)
x86: kernel.elf


x86_64: ARCH := $(X86_64)
x86_64: kernel.elf

mkiso_i386: x86
	mkisofs -R \
		-b boot/grub/stage2_eltorito    \
		-no-emul-boot                   \
		-boot-load-size 4               \
		-A $(OS_NAME)					\
		-input-charset utf8             \
		-boot-info-table                \
		-o $(OS_NAME).iso           	\
		iso/i386

mkiso_x86_64: x86_64
	grub-mkrescue -o grub.iso iso/x86_64

run: mkiso_$(ARCH)
	qemu-system-$(ARCH) -enable-kvm -cdrom $(OS_NAME).iso \
			-boot menu=on -drive file=Image.img -m 1G

clean:
	$(RM) *.o
	$(RM) *.elf
	$(RM) *.map
	$(RM) $(OS_NAME).iso
	$(RM) *.out


