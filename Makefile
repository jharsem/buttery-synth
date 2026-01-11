# ButterySynth Makefile
# 4-voice polyphonic synthesizer for Raspberry Pi

CC = gcc
CFLAGS = -Wall -std=c99 -O2 -DPLATFORM_DRM -DGRAPHICS_API_OPENGL_ES2
INCLUDES = -I/home/jon/Documents/raylib/src -I/usr/include/libdrm
LDFLAGS = -L/home/jon/Documents/raylib/src
LDLIBS = -lraylib -lasound -ldrm -lgbm -lEGL -lGLESv2 -lpthread -lrt -lm -latomic -ldl

SRC_DIR = src
BUILD_DIR = build
TARGET = buttersynth

SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

.PHONY: all clean run

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS) $(LDLIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

run: $(TARGET)
	sudo ./$(TARGET)

# Debug build
debug: CFLAGS += -g -DDEBUG
debug: clean all
