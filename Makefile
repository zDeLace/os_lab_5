CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2

SRCS = src/parent.c src/child.c src/utils.c
OBJS = $(SRCS:.c=.o)
TARGET = lab_os

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) -lm -lm

%.o: %.c include/common.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
