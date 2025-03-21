COLOUR_GREEN=\033[0;32m
COLOUR_RED=\033[0;31m
COLOUR_BLUE=\033[0;36m
COLOUR_END=\033[0m

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
	@echo -e "$(COLOUR_GREEN)$(LD): $^$(COLOUR_END)"
	@$(LD) $(LDFLAGS) $^ -o $@

$(BUILD)/$(CLIENT_EXE): $(OBJS_CLIENT)
	@mkdir -p $(dir $@)
	@echo -e "$(COLOUR_GREEN)$(LD): $^$(COLOUR_END)"
	@$(LD) $(LDFLAGS) $^ -o $@

.PHONY: clean
clean:
	rm -rf $(BUILD)

$(BUILD)/%.c.o: %.c
	@mkdir -p $(dir $@)
	@echo -e "$(COLOUR_BLUE)$(CC): $<$(COLOUR_END)"
	@$(CC) $(CFLAGS) -c -o $@ $<
