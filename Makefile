CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -Iinclude

TARGET = compiler
<<<<<<< HEAD
SRC = src/main.cpp

all: $(TARGET)

$(TARGET): $(SRC) include/common.hpp
=======
SRC = src/main.cpp src/lexer.cpp

all: $(TARGET)

$(TARGET): $(SRC) include/common.hpp include/lexer.hpp
>>>>>>> 7cf4835 (feat(lexer):完成词法分析器功能)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

clean:
	del /Q compiler.exe compiler 2>NUL || true

.PHONY: all clean
