# Compiler
#CC = i586-pc-msdosdjgpp-gcc
CC = i586-pc-msdosdjgpp-g++ -march=i486 -mtune=i486
CFLAGS_DBG = -O0 -g
CFLAGS_PROF = -pg -g -O3
CFLAGS_PROD = -Os
CFLAGS = -fpermissive

SRC_DIR = src
CDATA_DIR = src/data
BUILD_DIR = build

TARGET = $(BUILD_DIR)/roadv3.exe
SRCS = $(wildcard $(addsuffix /*.c,$(SRC_DIR))) \
       $(wildcard $(addsuffix /*.cpp,$(SRC_DIR))) \
       $(wildcard $(addsuffix /*.c,$(CDATA_DIR)))
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) $(CFLAGS_PROD) -o $@ $<

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR) && mkdir -p $(BUILD_DIR)/data

run: $(TARGET)
	dosbox-x -c "c:" -c "cd C:\DEV\roadv3" -c "$(BUILD_DIR)\roadv3.exe" #-c "exit"

debug: $(TARGET)
	dosbox-x -c "c:" -c "cd C:\DEV\roadv3" -c "gdb -ex run $(BUILD_DIR)\roadv3.exe" 
	
prof: $(TARGET)
	dosbox-x -c "c:" -c "cd C:\DEV\roadv3" -c "gprof $(BUILD_DIR)\roadv3.exe > gprof.out" 


clean:
	rm -rf $(BUILD_DIR)

.PHONY: all run clean

