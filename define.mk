LINKER_SCRIPT=linker.ld
OS_NAME=slavos
LD_ARGS=-T $(LINKER_SCRIPT) -Map=$(OS_NAME).map -z noexecstack
CC=gcc
C_ARGS=-c -nostdlib -fno-builtin -fno-stack-protector -Wall -Wextra
START_FILE=start
X86=i386
X86_64=x86_64
ARCH=$(X86)

