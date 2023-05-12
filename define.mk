LINKER_SCRIPT=linker.ld
OS_NAME=slavos
LD_ARGS=-T $(LINKER_SCRIPT) -Map=$(OS_NAME).map -z noexecstack
CC=gcc
INCLUDE=../include
C_ARGS=-c -nostdlib -fno-builtin -fno-stack-protector -Wall -Wextra -I$(INCLUDE)
START_FILE=start
X86=i386
X86_64=x86_64
ARCH=$(X86)
ELF_F=$(ARCH)

