CC=gcc
CFLAGS=-MMD -Wall -Wextra -pedantic

SRC=$(wildcard *.c)
OBJ=$(SRC:%.c=%.o)
DEP=$(OBJ:%.o=%.d)

DIR=bin
EXE=$(DIR)/skibicc
LIBS=$(addprefix -l,c)

all: debug

debug: CFLAGS += -g
debug: $(EXE)

release: CFLAGS += -O3 -DNDEBUG
release: $(EXE)

clean:
	rm -f $(OBJ) $(DEP) $(EXE)

$(EXE): $(OBJ) | dir
	$(CC) -o $@ $^ $(LIBS)
dir:
	mkdir -p $(DIR)

-include $(DEP)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
