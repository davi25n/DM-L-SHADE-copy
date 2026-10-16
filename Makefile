HEADER = de.h

TARGET = solver

OBJS := $(patsubst %.cc,%.o,$(shell find src -name '*.cc')) \
	$(patsubst %.cpp,%.o,$(shell find src -name '*.cpp' ! -path 'src/função/*'))
CC = g++
OPTION = -std=c++14 -O3
LIB_DIR = ./src/lib
INC_DIR = ./src/include

# Link the static library directly
LDFLAGS = $(LIB_DIR)/libpyclustering.a -lm

# Add -I to specify the header file directory
CFLAGS = -I$(INC_DIR) -I$(INC_DIR)/pyclustering

# -MMD -MP generate .d files so objects are rebuilt when a header changes
DEPFLAGS = -MMD -MP

# Debug build: no optimization + debug symbols, separate objects (.dbg.o) and binary
DEBUG_TARGET = solver_debug
DEBUG_OPTION = -std=c++14 -O0 -g3
DEBUG_OBJS := $(OBJS:.o=.dbg.o)

DEPS := $(OBJS:.o=.d) $(DEBUG_OBJS:.o=.d)

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(OPTION) $(LDFLAGS)

debug: $(DEBUG_TARGET)

$(DEBUG_TARGET): $(DEBUG_OBJS)
	$(CC) -o $(DEBUG_TARGET) $(DEBUG_OBJS) $(DEBUG_OPTION) $(LDFLAGS)

%.o: %.cpp
	$(CC) $(OPTION) $(CFLAGS) $(DEPFLAGS) -c $< -o $@
%.o: %.cc
	$(CC) $(OPTION) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

%.dbg.o: %.cpp
	$(CC) $(DEBUG_OPTION) $(CFLAGS) $(DEPFLAGS) -c $< -o $@
%.dbg.o: %.cc
	$(CC) $(DEBUG_OPTION) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

-include $(DEPS)

.PHONY: debug clean

clean:
	rm -f $(OBJS) $(DEBUG_OBJS) $(DEPS) $(TARGET) $(DEBUG_TARGET)