# Nicolai Brand (lytix.dev) 2022-2023
# See LICENSE for license info

OBJDIR = .obj
SRC = src
DIRS := $(shell find $(SRC) -type d -not -wholename "src/client")
SRCS := $(shell find $(SRC) -type f -name "*.c" -not -wholename "src/client/*")
OBJS := $(SRCS:%.c=$(OBJDIR)/%.o)

CFLAGS = -Iinclude -Wall -Wextra -Wshadow -std=c11
CFLAGS += -DLOGGING
LDFLAGS = -pthread
LDLIBS = -lm

.PHONY: format clean tags bear $(OBJDIR)
TARGET = ulsr
TARGET_CLIENT = client
TARGET_GUI = gui

all: $(TARGET)

$(OBJDIR)/%.o: %.c Makefile | $(OBJDIR)
	@echo [CC] $@
	@$(CC) -c $(CFLAGS) $< -o $@

$(TARGET): $(OBJS)
	@echo [LD] $@
	@$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

debug: CFLAGS += -g -DDEBUG
debug: $(TARGET)

clean:
	rm -rf $(OBJDIR) $(TARGET) $(TARGET_CLIENT) $(TARGET_GUI)

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

GUI_DIRS := $(shell find $(SRC) -type d -name "gui")
GUI_SRCS := $(shell find $(GUI_DIRS) -type f -name "*.c")
GUI_OBJS := $(GUI_SRCS:$(SRC)/%.c=$(OBJDIR)/%.o)

gui: CFLAGS += -DGUI -I/usr/include/freetype2 -I/usr/include/libpng16
gui: LDLIBS += -lglfw -lGLU -lGL -lXrandr -lXxf86vm -lXi -lXinerama -lX11 -lrt -ldl -lstb -lfreetype -lGLEW

gui: $(TARGET_GUI)

$(OBJDIR)/%.o: %.c Makefile | $(OBJDIR)
	@echo [CC] $@
	@$(CC) -c $(CFLAGS) $< -o $@

$(TARGET_GUI): $(OBJS)
	@echo [LD] $@
	@$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

gui_debug: CFLAGS += -g -DDEBUG
gui_debug: $(TARGET_GUI)