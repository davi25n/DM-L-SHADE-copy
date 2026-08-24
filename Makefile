HEADER = de.h
TARGET = solver

OBJS := $(patsubst %.cc,%.o,$(shell find src -name '*.cc'))
FLOW_OBJS := função/conversão.o
CC = g++
OPTION = -std=c++14 -O3
LIB_DIR = ./src/lib
INC_DIR = ./src/include

# Link the static library directly
LDFLAGS = $(LIB_DIR)/libpyclustering.a -lm

# Add -I to specify the header file directory
CFLAGS = -I$(INC_DIR) -I$(INC_DIR)/pyclustering

$(TARGET): $(OBJS) $(FLOW_OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(FLOW_OBJS) $(OPTION) $(LDFLAGS)

%.o: %.cc
	$(CC) $(CFLAGS) -c $< -o $@

função/%.o: função/%.cpp
	$(CC) $(CFLAGS) -std=c++14 -c $< -o $@

clean:
	rm -rf src/*.o função/*.o $(TARGET)
