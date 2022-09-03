NAME           := kernel
SRCS           := framework/main.c $(shell find -L ./src/ -name "*.c")
INC_PATH       := include/ framework/

export AM_HOME := $(PWD)/../abstract-machine
ifeq ($(ARCH),)
export ARCH    := x86_64-qemu
endif

include $(AM_HOME)/Makefile
include ../Makefile.lab
image: git

NAME           := test

test: git
	@gcc $(shell find src/pmm.c)  \
         $(shell find test/ -name "*.c") \
        -Iframework -I../abstract-machine/klib/include -Itest -DTEST -lpthread -g\
        -o build/test

	@build/test 2

testall: test
	@build/test 0
	@build/test 1
	@build/test 2