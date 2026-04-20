CC=gcc
CFLAGS=-I. -MMD -Wall -Wextra -pedantic

BIN=bin/
MAIN=skibicc
EXE=$(BIN)$(MAIN)
LIBS=$(addprefix -l,c)

SRC=$(wildcard *.c)
OBJ=$(SRC:%.c=%.o)
DEP=$(OBJ:%.o=%.d)

TEST=test/
PFX=test_
UNITY=unity/
EXT=out
SRCT=$(wildcard $(TEST)*.c)
OBJT=$(SRCT:%.c=%.o)
DEPT=$(OBJT:%.o=%.d)

all: debug

debug: CFLAGS += -g
debug: $(EXE)

release: CFLAGS += -O3 -DNDEBUG
release: $(EXE)

# mkdir first then compile
$(EXE): $(OBJ) | $(BIN)
	$(CC) -o $@ $^ $(LIBS)

$(BIN):
	mkdir -p $(BIN)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Files with names like $(PFX)*.txt
RESULTS = $(patsubst $(TEST)$(PFX)%.c,$(TEST)$(PFX)%.txt,$(SRCT))

# Display results
PASSED = `grep -s PASS $(TEST)*.txt`
FAIL = `grep -s FAIL $(TEST)*.txt`
IGNORE = `grep -s IGNORE $(TEST)*.txt`

test: CFLAGS += -g -I$(UNITY) -DTEST
test: $(RESULTS)
	@echo "-----------------------IGNORES:-----------------------"
	@echo "$(IGNORE)"
	@echo "-----------------------FAILURES:-----------------------"
	@echo "$(FAIL)"
	@echo "-----------------------PASSED:-----------------------"
	@echo "$(PASSED)"
	@echo "DONE"

# Execute every test case, each spits out a .txt file
$(TEST)%.txt: $(TEST)%.$(EXT)
	-./$< > $@ 2>&1

# Compile each test file individually, excluding the object file for the  main
# executable, otherwise the main() function would be defined twice.
$(TEST)$(PFX)%.$(EXT): $(filter-out $(MAIN).o,$(OBJ)) $(TEST)$(PFX)%.o $(UNITY)unity.o
	$(CC) -o $@ $^

# unity (our test framework) object files
$(UNITY)%.o: $(UNITY)%.c $(UNITY)%.h
	$(CC) $(CFLAGS) -c -o $@ $<

# Test case object files
$(TEST)%.o: $(TEST)%.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJ) $(DEP) $(EXE)
	rm -f $(OBJT) $(DEPT) $(UNITY)unity.o
	rm -f $(TEST)*.$(EXT) $(TEST)*.txt

doc:
	doxygen Doxyfile

-include $(DEP)
-include $(DEPT)

.PHONY: clean
.PHONY: test

.PRECIOUS: $(TEST)$(PFX)%.$(EXT)
.PRECIOUS: $(TEST)%.o
