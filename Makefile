# Nicolai Brand (lytix.dev) 2022-2023
# See LICENSE for license info

OBJDIR = .obj
SRC = src
DIRS := $(shell find $(SRC) -type d -not -wholename "src/client" -not -wholename "src/gui")
SRCS := $(shell find $(SRC) -type f -name "*.c" -not -wholename "src/client/*" -not -wholename "src/gui/*")
OBJS := $(SRCS:%.c=$(OBJDIR)/%.o)

CFLAGS = -Iinclude -Wall -Wextra -Wshadow -std=c11
CFLAGS += -DLOGGING
LDFLAGS = -pthread
LDLIBS = -lm

.PHONY: format clean tags bear $(OBJDIR)
TARGET = ulsr
TARGET_CLIENT = client
TARGET_GUI = gui
TARGET_GUI_MACOS = gui_macos

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
	rm -rf $(OBJDIR) $(TARGET) $(TARGET_CLIENT) $(TARGET_GUI) $(TARGET_GUI_MACOS)

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

gui: CFLAGS += -DGUI -I/usr/include/freetype2 -I/usr/include/libpng16 -g -DDEBUG
gui: LDLIBS += -lglfw -lGLU -lGL -lXrandr -lXxf86vm -lXi -lXinerama -lX11 -lrt -ldl -lfreetype -lGLEW

GUI_SRCS := $(shell find $(SRC)/gui -type f -name "*.c")
GUI_OBJS := $(GUI_SRCS:$(SRC)/gui/%.c=$(OBJDIR)/$(SRC)/gui/%.o)

$(TARGET_GUI): $(GUI_OBJS) $(OBJS)
	@echo [LD] $@
	@$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(OBJDIR)/$(SRC)/gui/%.o: $(SRC)/gui/%.c Makefile | $(OBJDIR)/$(SRC)/gui
	@echo [CC] $@
	@$(CC) -c $(CFLAGS) $< -o $@

$(OBJDIR)/$(SRC)/gui:
	@mkdir -p $@

$(TARGET_GUI_MACOS): CFLAGS += -DGUI -I/usr/local/include -I/usr/local/include/freetype2
$(TARGET_GUI_MACOS): LDLIBS += -L/usr/local/lib -lglfw -framework OpenGL -lfreetype -lGLEW

GUI_MACOS_SRCS := $(shell find $(SRC)/gui -type f -name "*.c")
GUI_MACOS_OBJS := $(GUI_MACOS_SRCS:$(SRC)/gui/%.c=$(OBJDIR)/$(SRC)/gui_macos/%.o)

$(TARGET_GUI_MACOS): $(GUI_MACOS_OBJS) $(OBJS)
	@echo [LD] $@
	@$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(OBJDIR)/$(SRC)/gui_macos/%.o: $(SRC)/gui/%.c Makefile | $(OBJDIR)/$(SRC)/gui_macos
	@echo [CC] $@
	@$(CC) -c $(CFLAGS) $< -o $@

$(OBJDIR)/$(SRC)/gui_macos:
	@mkdir -p $@