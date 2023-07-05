BUILD_DIR = build/

include ../define.mk

include ../build.mk

i386:
	mkdir -p ../build/usermode/
	ld -T userlink.ld -Map=userlink.map -z noexecstack \
		-melf_$(ARCH) $(BUILD_OBJS) -o ../build/usermode/init
