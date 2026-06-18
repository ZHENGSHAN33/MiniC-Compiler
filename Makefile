CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -Iinclude

TARGET = compiler

SRC = src/main.cpp src/lexer.cpp src/parser.cpp src/semantic.cpp src/ir.cpp

all: $(TARGET)

$(TARGET): $(SRC) include/common.hpp include/lexer.hpp include/parser.hpp include/semantic.hpp include/ir.hpp

	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

clean:
	del /Q compiler.exe compiler 2>NUL || true

.PHONY: all clean
