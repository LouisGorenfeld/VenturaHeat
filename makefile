# Compiler
CC = i586-pc-msdosdjgpp-gcc

# Source and build directories
SRC_DIR = src
BUILD_DIR = build

# Executable name
TARGET = $(BUILD_DIR)/roadv3.exe

# Collect all .c files in src/
SRCS = $(wildcard $(SRC_DIR)/*.c)

# Object files go in build/
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

# Default target
all: $(TARGET)

# Rule to build executable
$(TARGET): $(OBJS)
	$(CC) -o $@ $^

# Rule to compile .c to .o into build/
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) -c -o $@ $<

# Ensure build directory exists
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Run inside DOSBox-X
run: $(TARGET)
	dosbox-x -c "c:" -c "cd C:\DEV\roadv3" -c "$(BUILD_DIR)\roadv3.exe" #-c "exit"

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all run clean

