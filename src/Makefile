UTILS_PATH ?= ../utils

CC = gcc
CPPFLAGS = -I$(UTILS_PATH)
CFLAGS = -fPIC -Wall -Wextra -g
LDFLAGS = -shared

SRCS = osmem.c osmem_utils.c $(UTILS_PATH)/printf.c
OBJS = $(SRCS:.c=.o)
TARGET = libosmem.so

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) ${LDFLAGS} -o $@ $^

clean:
	-rm -f ../src.zip
	-rm -f $(TARGET)
	-rm -f $(OBJS)
