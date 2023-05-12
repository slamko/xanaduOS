LIB_DIR = lib

C_SRC = $(wildcard $(LIB_DIR)/*.c)
ASM_SRC = $(wildcard $(LIB_DIR)/*.asm)
C_OBJS = $(patsubst %.c, %.o, $(C_SRC))
BUILD_C_OBJS = $(patsubst %.o, build/%.o, $(C_OBJS))
ASM_OBJS = $(patsubst %.asm, %.o, $(ASM_SRC))
BUILD_ASM_OBJS = $(patsubst %.o, build/%.o, $(ASM_OBJS))
LIB_OBJS := $(BUILD_C_OBJS) $(BUILD_ASM_OBJS)

$(BUILD_C_OBJS): $(C_SRC) 
	$(CC) $(C_ARGS) -m$(ELF_F) $^ -c
	mkdir -p build/$(LIB_DIR)
	mv $(C_OBJS) build/$(LIB_DIR)

$(BUILD_ASM_OBJS): $(ASM_SRC)
	nasm -f $(ELF_F) $^
	mkdir -p build/$(LIB_DIR)
	mv $(ASM_OBJS) build/$(LIB_DIR)


