# Variables
CXX = clang++
CXXFLAGS = -std=c++20 -stdlib=libc++
TARGET = main
SRCS = TCPServer.cpp GameServer.cpp utils.cpp GameLogic.cpp

# Default target
all: $(TARGET)

# Rule to build the target
$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)

# Clean up the output
clean:
	rm -f $(TARGET)

# Phony targets
.PHONY: all clean
