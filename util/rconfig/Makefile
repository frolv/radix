FRONTEND ?= cli
LIB := librconfig.a

PROGRAM_NAME :=

################################################################################
# Define rconfig frontends here
# Each frontend must specify a unique PROGRAM_NAME and a list of required
# object files called ${FR}_OBJ, where ${FR} is the uppercase frontend name.
################################################################################

# default cli frontend
ifeq ($(FRONTEND),cli)
	PROGRAM_NAME := rconfig
endif

# objects files for cli frontend
CLI_OBJ := cli.o interactive.o

################################################################################

FRONTEND_UPPER := $(shell echo $(FRONTEND) | tr [:lower:] [:upper:])
FRONTEND_DIR := frontends/$(FRONTEND)

CC := gcc
AR := ar
LEX := flex
YACC := bison
RM := rm -f

LIBDIR := lib
INCLUDEDIR := include

CFLAGS := $(CFLAGS) -O2 -Wall -Wextra -c -I$(INCLUDEDIR)
LDFLAGS := $(LDFLAGS) -lfl
ARFLAGS := rcs

HEADERS := include/rconfig.h lib/gen.h lib/lint.h
_RCONFIG_OBJ := rconfig.o scanner.o parser.o structures.o gen.o
RCONFIG_OBJ := $(patsubst %,$(LIBDIR)/%,$(_RCONFIG_OBJ))

$(eval FRONTEND_OBJ := $(patsubst %,$(FRONTEND_DIR)/%,$($(FRONTEND_UPPER)_OBJ)))

SCAN_SRC := $(LIBDIR)/scanner.c
SCAN_H := $(LIBDIR)/scanner.h
PARSE_SRC := $(LIBDIR)/parser.c
PARSE_H := $(LIBDIR)/parser.h

all: librconfig $(PROGRAM_NAME)

$(PROGRAM_NAME): $(FRONTEND_OBJ) $(LIB)
	$(CC) $^ -o $@ $(LDFLAGS)

librconfig: $(LIB)

$(LIB): parser $(RCONFIG_OBJ)
	$(AR) $(ARFLAGS) $@ $(RCONFIG_OBJ)

$(libdir)/%.o: $(libdir)/%.c $(HEADERS)
	$(CC) $< -o $@ $(CFLAGS)

.PHONY: parser
parser: $(PARSE_SRC) $(PARSE_H) $(SCAN_SRC)

$(PARSE_SRC) $(PARSE_H): $(LIBDIR)/config.y
	$(YACC) --defines=$(PARSE_H) -o $(PARSE_SRC) $<

$(SCAN_SRC) $(SCAN_H): $(LIBDIR)/config.l $(PARSE_H)
	$(LEX) --header-file=$(SCAN_H) -o $(SCAN_SRC) $<

.PHONY: clean-all
clean-all: clean-frontend clean-librconfig

.PHONY: clean
clean: clean-frontend

.PHONY: clean-frontend
clean-frontend:
	$(RM) $(PROGRAM_NAME) $(FRONTEND_OBJ)

.PHONY: clean-librconfig
clean-librconfig:
	$(RM) $(RCONFIG_OBJ) $(SCAN_SRC) $(SCAN_H) $(PARSE_SRC) $(PARSE_H) $(LIB)
