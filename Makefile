CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -pthread
LDFLAGS = -pthread

SRCS = tablero.cpp config.cpp mesa.cpp torneo.cpp
OBJS = $(SRCS:.cpp=.o)

TARGET = torneo

all: $(TARGET)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f *.o $(TARGET) test_tablero

.PHONY: all clean
