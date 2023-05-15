# Nicolai Brand (lytix.dev) 2022-2023
# See LICENSE for license info

OBJDIR = .obj
SRC = src
DIRS := $(shell find $(SRC) -type d)
SRCS := $(shell find $(SRC) -type f -name "*.c")
OBJS := $(SRCS:%.c=$(OBJDIR)/%.o)

CC = gcc
CFLAGS = -I include -Wall -Wpedantic -Wextra -Wshadow -std=c11

.PHONY: format clean tags bear $(OBJDIR)
TARGET = ulsr

all: $(TARGET)

$(OBJDIR)/%.o: %.c Makefile | $(OBJDIR)
	@echo [CC] $@
	@$(CC) -c $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	@echo [LD] $@
	@$(CC) -o $@ $^

debug: CFLAGS += -g -DDEBUG
debug: $(TARGET)

debug-verbose: CFLAGS += -DDEBUG_VERBOSE
debug-verbose: debug

clean:
	@rm -rf $(OBJDIR) $(TARGET)

tags:
	@ctags -R

bear:
	bear -- make

format:
	python format.py

$(OBJDIR):
	$(foreach dir, $(DIRS), $(shell mkdir -p $(OBJDIR)/$(dir)))
