LINKER_SCRIPT=linker.ld
OS_NAME=slavos
LD_ARGS=-T $(LINKER_SCRIPT) -Map=$(OS_NAME).map -z noexecstack
CC=gcc
INCLUDE=-I../include -Iinclude
NUL_OPT=-O0
OPT=$(NUL_OPT)
C_ARGS=-c -nostdlib -fno-builtin -fno-stack-protector -Wall -Wno-unused-parameter -Wno-unused-function -Wextra $(INCLUDE) 
START_FILE=start
X86=i386
X86_64=x86_64
ARCH=$(X86)
ELF_F=$(ARCH)

release: C_ARGS += -O2
release: $(ARCH)

debug: C_ARGS += -O0 -g
debug: $(ARCH)

