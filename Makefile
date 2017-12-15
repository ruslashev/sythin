warnings = -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable \
		   -Wduplicated-cond -Wduplicated-branches -Wdouble-promotion \
		   -Wnull-dereference -Wformat=2 -Wdisabled-optimization \
		   -Wsuggest-override -Wlogical-op -Wtrampolines -Wfloat-equal
flags = -ggdb3 -Og -std=c++0x -fno-rtti -fno-exceptions
libraries = -lSDL2 -lGLEW -lGL
CC = gcc
CXX = g++
BIN = sythin

SOURCES = $(shell find . -type f -name '*.cc' -o -name '*.cpp')
OBJS = .objs/src/bison_parser.cc.o $(SOURCES:./%=.objs/%.o)
DEPS = $(OBJS:.o=.d)
CXXFLAGS = $(warnings) $(flags)
LDFLAGS = $(libraries)

$(shell mkdir -p .objs >/dev/null)
$(shell mkdir -p .objs/src >/dev/null)
$(shell mkdir -p .objs/thirdparty >/dev/null)

# all:
# 	@echo $(SOURCES)

all: $(BIN)
	./sythin

$(BIN): $(OBJS)
	@echo "Linking to $@"
	@$(CXX) -o $@ $^ $(LDFLAGS)

src/bison_parser.cc: src/bison_parser.y
	@echo "Generating parser"
	@bison src/bison_parser.y \
	  && mv bison_parser.cc src \
	  && mv bison_parser_tokens.hh src

# .objs/%.o: thirdparty/*.cpp

# .objs/%.o: ./%
# 	@echo "asdf $< $@"

.objs/%.o: % src/bison_parser.cc
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
	@rm -f $(BIN) $(OBJS) $(DEPS) src/bison_parser.cc src/bison_parser_tokens.hh

-include $(DEPS)

