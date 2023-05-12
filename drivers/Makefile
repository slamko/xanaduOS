DIR = drivers

include ../define.mk

C_SRC = $(wildcard ./*.c)
ASM_SRC = $(wildcard ./*.asm)
C_OBJS = $(patsubst %.c, %.o, $(C_SRC))
BUILD_C_OBJS = $(patsubst %.o, ../build/drivers/%.o, $(C_OBJS))
ASM_OBJS = $(patsubst %.asm, %.o, $(ASM_SRC))
BUILD_ASM_OBJS = $(patsubst %.o, ../build/drivers/%.o, $(ASM_OBJS))
DRIVERS_OBJS := $(BUILD_C_OBJS) $(BUILD_ASM_OBJS)

$(BUILD_C_OBJS): $(C_SRC) 
	$(CC) $(C_ARGS) -m$(ELF_F) $^ -c
	mkdir -p ../build/$(DIR)
	mv $(C_OBJS) ../build/$(DIR)

$(BUILD_ASM_OBJS): $(ASM_SRC)
	$(foreach asmf, $^, nasm -f elf$(ELF_F) $(asmf);) 
	mkdir -p ../build/$(DIR)
	mv $(ASM_OBJS) ../build/$(DIR)


all: $(DRIVERS_OBJS)

x86: all
x86: ELF_F = 32

