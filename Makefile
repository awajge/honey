CC		?= cc
AR		?= ar
CFLAGS	?= -std=c99 -Wall -Wextra -Wpedantic -Iinclude -Iexternal -O2

NAME	:= honey
SRC_DIR := src
DOC_DIR := docs
BLD_DIR := build
OBJ_DIR := $(BLD_DIR)/obj

TARGET  := $(BLD_DIR)/lib$(NAME).a
SRCS 	:= $(wildcard $(SRC_DIR)/*.c)
OBJS 	:= $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

$(NAME) all: $(TARGET)

clean:
	rm -rf $(BLD_DIR) $(DOC_DIR) $(OBJ_DIR)

docs:
	mkdir -p $(DOC_DIR)
	doxygen Doxyfile

$(TARGET): $(OBJS)
	$(AR) rcs $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BLD_DIR) $(OBJ_DIR):
	mkdir -p $@

.PHONY: $(NAME) clean docs