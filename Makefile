include define.mk

ELF_F=$(ARCH)
OBJS = $(shell find ./build -name '*.o')
MODULES = arch drivers lib bin mem kernel
MODE=

build_modules:
	for md in $(MODULES); do \
		$(MAKE) -C $$md $(MODE) $(ARCH); \
	done

kernel.elf:
	ld $(LD_ARGS) -melf_$(ARCH) $(OBJS) -o iso/$(ARCH)/boot/kernel.elf

# release: clean
release: MODE=release

x86: ARCH = $(X86)
x86: build_modules
x86: ELF_F = 32
x86: kernel.elf

i386: x86

x86_64: ARCH = $(X86_64)
x86_64: build_modules
x86_64: ELF_F = 64
x86_64: kernel.elf

mkiso_i386: kernel.elf
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
	$(RM) -r build


