C_SRC = $(wildcard drivers/*.c)
ASM_SRC = $(wildcard drivers/*.asm)
C_OBJS = $(patsubst %.c, %.o, $(C_SRC))
ASM_OBJS = $(patsubst %.asm, %.o, $(ASM_SRC))
DRIVERS_OBJS := $(C_OBJS) $(ASM_OBJS)

drivers_objs: $(C_SRC) 
	$(CC) $(C_ARGS) -m32 $^ -c

