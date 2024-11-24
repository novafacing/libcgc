# Build tools
CC := clang
AR := llvm-ar

# Build sources
LIB_DIR 	= lib
LIB_SRC  	= $(wildcard $(LIB_DIR)/*.c)
SRC_DIR 	= src
SRC 		= $(wildcard $(SRC_DIR)/*.c)
INC     	= include

# Build output configuration
BUILD_DIR       	= build
BUILD_BIN_DIR 		= $(BUILD_DIR)/bin
BUILD_LIB_DIR    	= $(BUILD_DIR)/lib
BUILD_OBJ_DIR    	= $(BUILD_DIR)/obj
BUILD_OBJ_LIB_DIR 	= $(BUILD_OBJ_DIR)/lib
BUILD_OBJ_SRC_DIR 	= $(BUILD_OBJ_DIR)/src

# Build targets
LIB       	= libcgc
STATIC_LIB 	= $(BUILD_LIB_DIR)/$(LIB).a
SHARED_LIB 	= $(BUILD_LIB_DIR)/$(LIB).so
LIB_OBJ    	= $(patsubst $(LIB_DIR)/%.c,$(BUILD_OBJ_LIB_DIR)/%.o,$(LIB_SRC))
BIN 		= $(BUILD_BIN_DIR)/test
BIN_OBJ 	= $(patsubst $(SRC_DIR)/%.c,$(BUILD_OBJ_SRC_DIR)/%.o,$(SRC))

# Build configuration
LIB_IFLAGS  = -I$(INC)
LIB_CFLAGS  = -fPIC
LIB_LDFLAGS = -lm
LIB_WFLAGS  = -Wno-unused-value -Wno-unused-command-line-argument
IFLAGS 		= -I$(INC)
CFLAGS 		= -O0 -g
LDFLAGS 	= -lm
WFLAGS 		=

all: build

build: prep $(BIN) $(SHAREDLIB) $(STATICLIB)

prep:
	mkdir -p $(BUILD_LIB_DIR) $(BUILD_BIN_DIR) $(BUILD_OBJ_LIB_DIR) $(BUILD_OBJ_SRC_DIR)

$(SHARED_LIB): $(LIB_OBJ)
	$(CC) -o $@ -shared $^ $(LIB_CFLAGS) $(LIB_IFLAGS) $(LIB_LDFLAGS) $(LIB_WFLAGS)

$(STATIC_LIB): $(LIB_OBJ)
	$(AR) rcs $@ $^

$(BIN): $(BIN_OBJ) $(STATIC_LIB)
	$(CC) -o $@ $^ $(CFLAGS) $(IFLAGS) $(LDFLAGS) $(WFLAGS)

$(BUILD_OBJ_LIB_DIR)/%.o: $(LIB_DIR)/%.c
	$(CC) -c -o $@ $< $(LIB_CFLAGS) $(LIB_IFLAGS) $(LIB_WFLAGS)

$(BUILD_OBJ_SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) -c -o $@ $< $(CFLAGS) $(IFLAGS) $(WFLAGS)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all prep build clean