LIB_DIR = lib

include ../define.mk

C_SRC = $(wildcard ./*.c)
ASM_SRC = $(wildcard ./*.asm)
C_OBJS = $(patsubst %.c, %.o, $(C_SRC))
BUILD_C_OBJS = $(patsubst %.o, ../build/lib/%.o, $(C_OBJS))
ASM_OBJS = $(patsubst %.asm, %.o, $(ASM_SRC))
BUILD_ASM_OBJS = $(patsubst %.o, ../build/lib/%.o, $(ASM_OBJS))
LIB_OBJS := $(BUILD_C_OBJS) $(BUILD_ASM_OBJS)

$(BUILD_C_OBJS): $(C_SRC) 
	$(CC) $(C_ARGS) -m$(ELF_F) $^ -c
	mkdir -p ../build/$(LIB_DIR)
	mv $(C_OBJS) ../build/$(LIB_DIR)

$(BUILD_ASM_OBJS): $(ASM_SRC)
	$(foreach asmf, $^, nasm -f elf$(ELF_F) $(asmf);)
	mkdir -p ../build/$(LIB_DIR)
	mv $(ASM_OBJS) ../build/$(LIB_DIR)



all: $(LIB_OBJS)

x86: ELF_F = 32
x86: all
