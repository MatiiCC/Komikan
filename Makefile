CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17
LDFLAGS = -lpthread

SRCS = tablero.cpp config.cpp
OBJS = $(SRCS:.cpp=.o)

all: test_tablero

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

test_tablero: test_tablero.o $(OBJS)
	$(CXX) $(CXXFLAGS) test_tablero.o $(OBJS) -o test_tablero $(LDFLAGS)

clean:
	rm -f *.o test_tablero torneo

.PHONY: all clean
