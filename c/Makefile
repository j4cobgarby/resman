CC=clang
CFLAGS=-Wall -Wextra -Wpedantic -Iinclude
LD=clang
LDFLAGS=

BUILD=./build
SRCDIR_SERVER=server
SRCDIR_CLIENT=client
SRCDIR_COMMON=common

SRCS_SERVER=$(wildcard $(SRCDIR_SERVER)/*.c)
SRCS_CLIENT=$(wildcard $(SRCDIR_CLIENT)/*.c)
SRCS_COMMON=$(wildcard $(SRCDIR_COMMON)/*.c)

OBJS_SERVER=$(addprefix $(BUILD)/, $(addsuffix .o, $(SRCS_SERVER)))
OBJS_CLIENT=$(addprefix $(BUILD)/, $(addsuffix .o, $(SRCS_CLIENT)))
OBJS_COMMON=$(addprefix $(BUILD)/, $(addsuffix .o, $(SRCS_COMMON)))
OBJS_SERVER += $(OBJS_COMMON)
OBJS_CLIENT += $(OBJS_COMMON)

SERVER_EXE=resmand
CLIENT_EXE=resman

.PHONY: all
all: $(BUILD)/resmand $(BUILD)/resman

$(BUILD)/$(SERVER_EXE): $(OBJS_SERVER)
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) $^ -o $@

$(BUILD)/$(CLIENT_EXE): $(OBJS_CLIENT)
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) $^ -o $@

.PHONY: clean
clean:
	rm -rf $(BUILD)

$(BUILD)/%.c.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<
