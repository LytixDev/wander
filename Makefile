# Nicolai Brand (lytix.dev) 2022-2023
# See LICENSE for license info

OBJDIR = .obj
SRC = src
DIRS := $(shell find $(SRC) -type d -not -wholename "src/client")
SRCS := $(shell find $(SRC) -type f -name "*.c" -not -wholename "src/client/*")
OBJS := $(SRCS:%.c=$(OBJDIR)/%.o)

CFLAGS = -Iinclude -Wall -Wextra -Wshadow -std=c11
CFLAGS += -DLOGGING
LDFLAGS = -pthread -lm

.PHONY: format clean tags bear $(OBJDIR)
TARGET = ulsr
TARGET_CLIENT = client

all: $(TARGET)

$(OBJDIR)/%.o: %.c Makefile | $(OBJDIR)
	@echo [CC] $@
	@$(CC) -c $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	@echo [LD] $@
	@$(CC) $(LDFLAGS) -o $@ $^

debug: CFLAGS += -g -DDEBUG
debug: $(TARGET)

clean:
	rm -rf $(OBJDIR) $(TARGET) $(TARGET_CLIENT)

tags:
	@ctags -R

bear:
	@bear -- make

format:
	python format.py

$(TARGET_CLIENT):
	$(CC) src/client/client.c src/ulsr/packet.c $(CFLAGS) -o $(TARGET_CLIENT)

$(OBJDIR):
	$(foreach dir, $(DIRS), $(shell mkdir -p $(OBJDIR)/$(dir)))
