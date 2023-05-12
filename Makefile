include define.mk

ELF_F=$(ARCH)

include lib/Makefile
include drivers/Makefile

kernel.elf: arch/$(ARCH)/start.asm $(DRIVERS_OBJS) $(LIB_OBJS) kernel.c
	nasm -f elf$(ELF_F) arch/$(ARCH)/start.asm
	$(CC) -m$(ELF_F) $(C_ARGS) kernel.c 
	ld $(LD_ARGS) -melf_$(ARCH) arch/$(ARCH)/start.o $(DRIVERS_OBJS) \
		kernel.o -o iso/$(ARCH)/boot/kernel.elf

x86: ARCH = $(X86)
x86: ELF_F = 32
x86: kernel.elf

x86_64: ARCH = $(X86_64)
x86_64: ELF_F = 64
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


