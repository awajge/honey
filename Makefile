CC		?= cc
AR		?= ar
CFLAGS	?= -std=c99 -Wall -Wextra -Wpedantic -Iinclude -O2

NAME	:= honey
SRC_DIR := src
DOC_DIR := docs
LIB_DIR := lib
OBJ_DIR := $(LIB_DIR)/obj

TARGET  := $(LIB_DIR)/lib$(NAME).a
SRCS 	:= $(wildcard $(SRC_DIR)/*.c)
OBJS 	:= $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

$(NAME) build all: $(TARGET)

clean:
	rm -rf $(LIB_DIR) $(DOC_DIR) $(OBJ_DIR)

docs:
	mkdir -p $(DOC_DIR)
	doxygen Doxyfile

$(TARGET): $(OBJS)
	$(AR) rcs $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(LIB_DIR) $(OBJ_DIR):
	mkdir -p $@

.PHONY: $(NAME) clean docs