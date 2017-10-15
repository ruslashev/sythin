warnings = -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable \
		   -Wduplicated-cond -Wduplicated-branches -Wdouble-promotion \
		   -Wnull-dereference -Wformat=2 -Wdisabled-optimization \
		   -Wsuggest-override -Wlogical-op -Wtrampolines -Wfloat-equal
flags = -ggdb3 -Og -std=c++0x -fno-rtti -fno-exceptions
libraries =
CC = gcc
CXX = g++
BIN = sythin

CC_OBJS = $(patsubst src/%.cc,.objs/%.o, $(shell find src/ -type f -iname '*.cc'))
OBJS = .objs/lemon_parser.o $(CC_OBJS) $(CPP_OBJS)
DEPS = $(OBJS:.o=.d)
CXXFLAGS = $(warnings) $(flags)
LDFLAGS = $(libraries)

$(shell mkdir -p .objs >/dev/null)

all: $(BIN)
	./sythin

$(BIN): $(OBJS)
	@echo "Linking to $@"
	@$(CXX) -o $@ $^ $(LDFLAGS)

thirdparty/lemon:
	@echo "Compiling thirdparty/lemon"
	@$(CC) thirdparty/lemon.c -o thirdparty/lemon

src/lemon_parser.cc: thirdparty/lemon src/lemon_parser.y
	@echo "Generating parser"
	@thirdparty/lemon src/lemon_parser.y \
		&& mv src/lemon_parser.c src/lemon_parser.cc \
		&& mv src/lemon_parser.h src/lemon_parser_tokens.hh \
		&& rm src/lemon_parser.out

.objs/%.o: src/%.cc
	@echo "Compiling $<"
	@$(CXX) -MMD -MP -c -o $@ $< $(CXXFLAGS)

gdb: $(BIN)
	gdb $(BIN)

valgrind: $(BIN)
	valgrind --leak-check=full ./$(BIN)

callgrind: $(BIN)
	@valgrind --tool=callgrind ./$(BIN)
	@kcachegrind callgrind.out.$!

.PHONY : clean
clean:
	@rm -f $(BIN) $(OBJS) $(DEPS) src/lemon_parser.cc src/lemon_parser_tokens.hh

-include $(DEPS)

