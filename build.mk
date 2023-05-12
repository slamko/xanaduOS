
C_SRC = $(wildcard ./*.c)
ASM_SRC = $(wildcard ./*.asm)
C_OBJS = $(patsubst %.c, %.o, $(C_SRC))
BUILD_C_OBJS = $(patsubst %.o, $(BUILD_DIR)/%.o, $(C_OBJS))
ASM_OBJS = $(patsubst %.asm, %.o, $(ASM_SRC))
BUILD_ASM_OBJS = $(patsubst %.o, $(BUILD_DIR)/%.o, $(ASM_OBJS))
DRIVERS_OBJS := $(BUILD_C_OBJS) $(BUILD_ASM_OBJS)

$(BUILD_C_OBJS): $(C_SRC) 
	$(CC) $(C_ARGS) -m$(ELF_F) $^ -c
	mkdir -p $(BUILD_DIR)
	mv $(C_OBJS) $(BUILD_DIR)

$(BUILD_ASM_OBJS): $(ASM_SRC)
	$(foreach asmf, $^, nasm -f elf$(ELF_F) $(asmf);) 
	mkdir -p $(BUILD_DIR)
	mv $(ASM_OBJS) $(BUILD_DIR)


all: $(DRIVERS_OBJS)

x86: all
x86: ELF_F = 32

