

VM_NAME := libfowltalk.so
VM_PATH := bin/$(VM_NAME)
VM_SRCS := $(wildcard src/vm/*.c)
VM_OBJS := $(VM_SRCS:%.c=%.o)
VM_LIB_NAME := fowltalk

VM_TEST_NAME := fowltalk-tests
VM_TEST_PATH := bin/$(VM_TEST_NAME)
VM_TEST_SRCS := $(wildcard tests/vm/*.c)
VM_TEST_OBJS := $(VM_TEST_SRCS:%.c=%.o)

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

TEST_CFLAGS := $(CFLAGS) -Ideps/libtest/include

CLI_LDFLAGS += -L./bin -l$(VM_LIB_NAME)
OS := $(shell uname -s)
ifeq ($(OS),Linux)
	CFLAGS += -std=gnu17
else
	CFLAGS += -std=c17
# 	CLI_LDFLAGS += $(VM_PATH)
endif

all: $(CLI_PATH)
.PHONY: clean test

src/vm/%.o: src/vm/%.c
	$(CC) $(CFLAGS) -c -o $@ $< -fPIC

$(VM_PATH): $(VM_OBJS) bin
	$(CC) $(CFLAGS) -shared -o $@ $(VM_OBJS) $(LDFLAGS)

src/cli/%.o: src/cli/%.c $(LINENOISE_OBJ)
	$(CC) $(CFLAGS) $(CLI_CFLAGS) -c -o $@ $<

$(CLI_PATH): $(CLI_OBJS) $(LINENOISE_OBJ) $(VM_PATH)
	$(CC) $(CFLAGS) -o $@ $(CLI_OBJS) $(LINENOISE_OBJ) $(LDFLAGS) $(CLI_LDFLAGS)
ifeq ($(OS),Darwin)
	install_name_tool -change $(VM_PATH) @executable_path/$(VM_NAME) $(CLI_PATH)
endif

linenoise/linenoise.c:
	git clone https://github.com/antirez/linenoise.git --depth 1

$(LINENOISE_OBJ): linenoise/linenoise.c
	$(CC) $(CFLAGS) -c -o $@ $< $(LDFLAGS)

bin:
	mkdir -p bin

test: $(VM_TEST_PATH)

tests/vm/%.o: tests/vm/%.c deps/libtest/libtest.c
	$(CC) $(TEST_CFLAGS) -c -o $@ $<

$(VM_TEST_PATH): $(VM_TEST_OBJS) $(VM_OBJS) bin deps/libtest
	$(CC) $(TEST_CFLAGS) -o $@ $(VM_TEST_OBJS) $(VM_OBJS) deps/libtest/libtest.c $(LDFLAGS)
ifeq ($(OS),Darwin)
	install_name_tool -change $(VM_PATH) @executable_path/$(VM_NAME) $(VM_TEST_PATH)
endif

clean: cleanobjs
	rm -f $(VM_PATH) $(VM_TEST_PATH) $(CLI_PATH)

cleanobjs:
	rm -f $(VM_OBJS) $(VM_TEST_OBJS) $(CLI_OBJS) $(LINENOISE_OBJ)

cleandeps:
	rm -rf deps

cleanall: clean cleandeps

deps: deps/libtest

deps/libtest:
	mkdir -p deps
	git clone --depth 1 https://github.com/fowlmouth/libtest.git deps/libtest



xxx:
	echo $(UNAME)
	echo $(shell uname -s)
	echo $(shell uname -h)
	echo OS= $(OS)


