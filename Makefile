# Nicolai Brand (lytix.dev) 2022-2023
# See LICENSE for license info

OBJDIR = .obj
SRC = src
DIRS := $(shell find $(SRC) -type d -not -wholename "src/client")
SRCS := $(shell find $(SRC) -type f -name "*.c" -not -wholename "src/client/*")
OBJS := $(SRCS:%.c=$(OBJDIR)/%.o)

CC = gcc
CFLAGS = -Iinclude -Wall -Wextra -Wshadow -std=c11 -DLOGGING
LDFLAGS = -pthread

.PHONY: format clean tags bear $(OBJDIR)
TARGET = ulsr

all: $(TARGET)

$(OBJDIR)/%.o: %.c Makefile | $(OBJDIR)
	@echo [CC] $@
	@$(CC) -c $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	@echo [LD] $@
	@$(CC) $(LDFLAGS) -o $@ $^

debug: CFLAGS += -g -DDEBUG
debug: $(TARGET)

debug-verbose: CFLAGS += -DDEBUG_VERBOSE
debug-verbose: debug

clean:
	@rm -rf $(OBJDIR) $(TARGET)
	@rm -f $(TARGET)
	@rm -f client

tags:
	@ctags -R

bear:
	bear -- make

format:
	python format.py

client:
	gcc src/client/client.c src/ulsr/packet.c $(CFLAGS) -o client

$(OBJDIR):
	$(foreach dir, $(DIRS), $(shell mkdir -p $(OBJDIR)/$(dir)))
