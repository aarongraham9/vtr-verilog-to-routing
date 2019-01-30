#Authors: Aaron Graham (aaron.graham@unb.ca, aarongraham9@gmail.com),
#         Jean-Philippe Legault (jlegault@unb.ca, jeanphilippe.legault@gmail.com) and
#          Dr. Kenneth B. Kent (ken@unb.ca)
#          for the Reconfigurable Computing Research Lab at the
#           Univerity of New Brunswick in Fredericton, New Brunswick, Canada

ifeq (run,$(firstword $(MAKECMDGOALS)))	
  # use the rest as arguments for "make"
  RUN_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
  # ...and turn them into do-nothing targets
  $(eval $(RUN_ARGS):;@:)
endif
ifeq (gdb,$(firstword $(MAKECMDGOALS)))	
  # use the rest as arguments for "make"
  RUN_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
  # ...and turn them into do-nothing targets
  $(eval $(RUN_ARGS):;@:)
endif
ifeq (valgrind,$(firstword $(MAKECMDGOALS)))	
  # use the rest as arguments for "make"
  RUN_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
  # ...and turn them into do-nothing targets
  $(eval $(RUN_ARGS):;@:)
endif

INCLUDE = -Isrc/include
SRC = src/*.cpp

BIN = rtl_number

C = clang++ -std=c++14 -lpthread

cleanup_flags=\
-ferror-limit=1000 \
-Werror \
-Wpedantic \
-Weverything \
-Wall \
-ggdb -O0 -g \
-Wno-c++98-compat \
-Wno-padded
#  \
# -fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls

PHONY: error

error: 
	echo "can only use 'clean', 'debug', 'build', 'test', 'valgrind <args>', gdb <args> 'run <args>'"

debug: clean
	$(C) $(cleanup_flags) $(INCLUDE) $(SRC) main.cpp -o $(BIN)

build: clean
	$(C) $(INCLUDE) $(SRC) -o $(BIN)

run:
	./$(BIN) $(RUN_ARGS) 

test:
	./verify_librtlnumber.sh

valgrind: build
	valgrind --leak-check=full $(BIN) $(RUN_ARGS) 

gdb:
	gdb --args $(BIN) $(RUN_ARGS)

clean:
	$(RM) -f $(BIN)

