# Compiler
#CC = i586-pc-msdosdjgpp-gcc
CC = i586-pc-msdosdjgpp-g++ -O0 -g
CFLAGS = -fpermissive

SRC_DIR = src
BUILD_DIR = build

TARGET = $(BUILD_DIR)/roadv3.exe
SRCS = $(wildcard $(addsuffix /*.c,$(SRC_DIR))) \
       $(wildcard $(addsuffix /*.cpp,$(SRC_DIR)))
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) -o $@ $<

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

run: $(TARGET)
	dosbox-x -c "c:" -c "cd C:\DEV\roadv3" -c "$(BUILD_DIR)\roadv3.exe" #-c "exit"

debug: $(TARGET)
	dosbox-x -c "c:" -c "cd C:\DEV\roadv3" -c "gdb -ex run $(BUILD_DIR)\roadv3.exe" 

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all run clean

