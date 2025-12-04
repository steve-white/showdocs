
# Detect OS
ifeq ($(OS),Windows_NT)
    DETECTED_OS := Windows
    EXE_EXT := .exe
    RM := del /Q
    LDFLAGS := -lws2_32
else
    DETECTED_OS := $(shell uname -s)
    EXE_EXT :=
    RM := rm -f
    LDFLAGS :=
endif

# Version information
VERSION ?= 1.0.0
BUILD_DATE := $(shell date "+%b_%d_%Y")
BUILD_TIME := $(shell date "+%H:%M:%S")
GIT_COMMIT := $(shell git rev-parse --short HEAD 2>/dev/null || echo "unknown")

CC = gcc
CFLAGS = -Wall -Wextra -std=c99
VERSION_FLAGS = -DVERSION=\"$(VERSION)\" -DBUILD_DATE=\"$(BUILD_DATE)\" -DBUILD_TIME=\"$(BUILD_TIME)\" -DGIT_COMMIT=\"$(GIT_COMMIT)\"
TARGET = showdocs$(EXE_EXT)

all: $(TARGET)

$(TARGET): showdocs.c
	$(CC) $(CFLAGS) $(VERSION_FLAGS) -o $(TARGET) showdocs.c $(LDFLAGS)

clean:
ifeq ($(DETECTED_OS),Windows)
	-$(RM) $(TARGET)
else
	$(RM) $(TARGET)
endif

.PHONY: all clean