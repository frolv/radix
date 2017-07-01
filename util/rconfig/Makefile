PROGRAM_NAME := rconfig

CC := gcc
LEX := flex
YACC := bison
RM := rm -f

CFLAGS := -O2 -Wall -Wextra -c
LDFLAGS := -lfl

HEADERS := rconfig.h
OBJ := rconfig.o scanner.o parser.o

SCAN_SRC := scanner.c
SCAN_H := scanner.h
PARSE_SRC := parser.c
PARSE_H := parser.h

all: parser $(PROGRAM_NAME)

$(PROGRAM_NAME): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

%.o: %.c $(HEADERS)
	$(CC) $< -o $@ $(CFLAGS)

.PHONY: parser
parser: $(PARSE_SRC) $(PARSE_H) $(SCAN_SRC)

$(PARSE_SRC) $(PARSE_H): config.y
	$(YACC) --defines=$(PARSE_H) -o $(PARSE_SRC) $<

$(SCAN_SRC) $(SCAN_H): config.l $(PARSE_H)
	$(LEX) --header-file=$(SCAN_H) -o $(SCAN_SRC) $<

.PHONY: clean
clean:
	$(RM) $(OBJ) $(PROGRAM_NAME) $(SCAN_SRC) $(SCAN_H) \
		$(PARSE_SRC) $(PARSE_H)
