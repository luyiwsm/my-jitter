CC = gcc

CFLAGS = -Wall -Wextra -Wpedantic -g

CPPFLAGS = -Iinclude

LDFLAGS =

LDLIBS = -lpcap

TARGET = jitter

SRC = $(wildcard src/*.c)

TEST_JITTER = test_jitter
TEST_FLOW = test_flow

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(CPPFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)

$(TEST_JITTER): tests/test_jitter.c src/jitter.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $^ -o $@ -lm

$(TEST_FLOW): tests/test_flow.c src/flow.c src/jitter.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $^ -o $@ $(LDFLAGS)

test: $(TEST_JITTER) $(TEST_FLOW)
	./$(TEST_JITTER)
	./$(TEST_FLOW)

clean:
	rm -f $(TARGET) $(TEST_JITTER) $(TEST_FLOW)