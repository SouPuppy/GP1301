CXX := g++
CC := gcc
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra
CFLAGS := -O2 -Wall -Wextra
LDFLAGS := -lpthread

SRCS := main.cpp generate_frame_vector.c compression.c
OBJS := $(SRCS:.cpp=.o)
OBJS := $(OBJS:.c=.o)

TARGET := main

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	@if [ -n "$(word 2,$(MAKECMDGOALS))" ]; then \
		./$(TARGET) $(word 2,$(MAKECMDGOALS)); \
	else \
		echo "Usage: make run <interval>"; \
	fi

%:
	@:

clean:
	rm -f $(TARGET) $(OBJS)
