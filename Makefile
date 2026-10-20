CXX = g++
CXXFLAGS = -std=c++20 -O3 -Wall -Wextra -Isrc
LDFLAGS = -static -static-libgcc -static-libstdc++ -lws2_32 -lshell32 -lole32

SRCS = src/main.cpp src/benchmark.cpp src/drives.cpp src/server.cpp
TARGET = rw-benchmark.exe

all: $(TARGET)

$(TARGET): $(SRCS) src/embedded_assets.hpp
	$(CXX) $(CXXFLAGS) $(SRCS) $(LDFLAGS) -o $(TARGET)

assets:
	python tools/generate_assets.py

clean:
	rm -f $(TARGET)

.PHONY: all clean assets
