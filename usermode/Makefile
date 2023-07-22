BUILD_DIR = build/

include ../define.mk

include ../build.mk

i386:
	mkdir -p ../build/usermode/usr/bin
	mkdir -p ../build/usermode/dev
	touch ../build/usermode/dev/tty
	ld -T userlink.ld -Map=userlink.map -melf_$(ARCH) build/usermode_asm.o build/main.o \
		-o ../build/usermode/usr/bin/init
	ld -T userlink.ld -Map=userlink.map -melf_$(ARCH) build/usermode_asm.o build/usershell.o \
		-o ../build/usermode/usr/bin/shell

# $(foreach user_obj, $(BUILD_OBJS), ld -T userlink.ld -Map=userlink.map \
# -melf_$(ARCH) $(user_bj) \
# -o ../build/usermode/$(basename $(user_obj));)
