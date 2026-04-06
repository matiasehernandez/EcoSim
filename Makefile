CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -I include
LDFLAGS = -lfltk -lfltk_images -lpthread -lm -lX11
TARGET = bin/ecosim

SRCS = src/main.cpp \
       src/grass.cpp \
       src/agent.cpp \
       src/prey.cpp \
       src/predator.cpp \
       src/popgraph.cpp \
       src/sim.cpp

OBJS = $(patsubst src/%.cpp, build/%.o, $(SRCS))

.PHONY: all clean run

all: $(TARGET)

# Enlazar
$(TARGET): $(OBJS)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Compilar cada .cpp a su .o en build/
build/%.o: src/%.cpp include/ecosim.h
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf build bin/ecosim
