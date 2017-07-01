PROGRAM_NAME := rconfig

CC := gcc
RM := rm -f

CFLAGS := -O2 -Wall -Wextra -c
LDFLAGS :=

OBJ := rconfig.o

all: $(PROGRAM_NAME)

$(PROGRAM_NAME): $(OBJ)
	$(CC) $(OBJ) -o $@

.c.o:
	$(CC) $< -o $@ $(CFLAGS)

.PHONY: clean
clean:
	$(RM) $(OBJ) $(PROGRAM_NAME)
