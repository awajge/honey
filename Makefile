CC		= cc
AR		= ar
CFLAGS	= -std=c99 -Wall -Wextra -Iinclude -O2

SRC_DIR = src
BLD_DIR = build
DOC_DIR = docs
OBJ_DIR	= $(BLD_DIR)/obj
LIB_DIR = $(BLD_DIR)/lib

TARGET 	= $(LIB_DIR)/libhoney.a
SRCS	= $(wildcard $(SRC_DIR)/*.c)
OBJS	= $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

all: $(TARGET)

clean:
	rm -rf $(BLD_DIR)
	rm -rf $(DOC_DIR)

docs:
	mkdir -p $(DOC_DIR)
	doxygen Doxyfile

$(TARGET): $(OBJS)
	mkdir -p $(LIB_DIR)
	$(AR) rcs $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: all clean docs