

VM_NAME := fowltalk.so
VM_PATH := bin/$(VM_NAME)
VM_SRCS := $(wildcard src/vm/*.c)
VM_OBJS := $(VM_SRCS:%.c=%.o)

CLI_NAME := fowltalk
CLI_PATH := bin/$(CLI_NAME)
CLI_SRCS := $(wildcard src/cli/*.c)
CLI_OBJS := $(CLI_SRCS:%.c=%.o)
CLI_CFLAGS += -Ilinenoise/

LINENOISE_OBJ := linenoise/linenoise.o

DEBUG ?= 0
ifeq ($(DEBUG), 1)
	CFLAGS += -DDEBUG -g -O0 -fsanitize=address
	CFLAGS += -DFT_DEBUG_LEXER=0 -DFT_DEBUG_PARSER=1 -DFT_DEBUG_COMPILER=1
	CFLAGS += -DFT_DEBUG_BYTECODE_BUILDER=1 -DFT_DEBUG_VM=1 -DFT_DEBUG_VTABLE=1
	CFLAGS += -DFT_DEBUG_FOWLTALK=1
else
	CFLAGS += -DNDEBUG -O1 -Wno-switch
endif

CFLAGS += -Iinclude/

OS := $(shell uname -s)
ifeq ($(OS),Linux)
	CFLAGS += -std=gnu17
else
	CFLAGS += -std=c17
endif

all: $(CLI_PATH)
.PHONY: clean

src/vm/%.o: src/vm/%.c
	cc $(CFLAGS) -c -o $@ $< -fPIC

$(VM_PATH): $(VM_OBJS) bindir
	cc $(CFLAGS) -shared -o $@ $(VM_OBJS) $(LDFLAGS)

src/cli/%.o: src/cli/%.c $(LINENOISE_OBJ)
	cc $(CFLAGS) $(CLI_CFLAGS) -c -o $@ $<

$(CLI_PATH): $(CLI_OBJS) $(LINENOISE_OBJ) $(VM_PATH)
	cc $(CFLAGS) -o $@ $^ $(LDFLAGS)

linenoise/linenoise.c:
	git clone https://github.com/antirez/linenoise.git --depth 1

$(LINENOISE_OBJ): linenoise/linenoise.c
	cc $(CFLAGS) -c -o $@ $< $(LDFLAGS)

bindir:
	mkdir -p bin

clean:
	rm -f $(VM_OBJS) $(VM_PATH) $(CLI_OBJS) $(CLI_PATH) $(LINENOISE_OBJ)

