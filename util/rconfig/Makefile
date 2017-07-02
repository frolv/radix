PROGRAM_NAME := rconfig

CC := gcc
LEX := flex
YACC := bison
RM := rm -f

CFLAGS := -O2 -Wall -Wextra -c
LDFLAGS := -lfl

HEADERS := rconfig.h gen.h lint.h
RCONFIG_OBJ := rconfig.o scanner.o parser.o structures.o gen.o

CLI_OBJ := cli.o

SCAN_SRC := scanner.c
SCAN_H := scanner.h
PARSE_SRC := parser.c
PARSE_H := parser.h

all: $(PROGRAM_NAME)

# default rconfig cli program
$(PROGRAM_NAME): librconfig $(CLI_OBJ)
	$(CC) $(RCONFIG_OBJ) $(CLI_OBJ) -o $@ $(LDFLAGS)

librconfig: parser $(RCONFIG_OBJ)

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
	$(RM) $(RCONFIG_OBJ) $(PROGRAM_NAME) $(SCAN_SRC) $(SCAN_H) \
	$(PARSE_SRC) $(PARSE_H) $(CLI_OBJ)
