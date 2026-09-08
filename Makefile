CC = gcc

CFLAGS = -Wall -Wextra -Wpedantic -g

CPPFLAGS = -Iinclude

LDFLAGS =

LDLIBS = -lpcap

ASAN_FLAGS = -fsanitize=address

TARGET = jitter

SRC = $(wildcard src/*.c)

TEST_JITTER = test_jitter
TEST_FLOW = test_flow

ASAN_TARGET = jitter_asan
ASAN_TEST_JITTER = test_jitter_asan
ASAN_TEST_FLOW = test_flow_asan

.PHONY: all clean test asan

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

$(ASAN_TARGET): $(SRC)
	$(CC) $(CFLAGS) $(ASAN_FLAGS) $(CPPFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)

$(ASAN_TEST_JITTER): tests/test_jitter.c src/jitter.c
	$(CC) $(CFLAGS) $(ASAN_FLAGS) $(CPPFLAGS) $^ -o $@ -lm

$(ASAN_TEST_FLOW): tests/test_flow.c src/flow.c src/jitter.c
	$(CC) $(CFLAGS) $(ASAN_FLAGS) $(CPPFLAGS) $^ -o $@ $(LDFLAGS)

asan: $(ASAN_TARGET) $(ASAN_TEST_JITTER) $(ASAN_TEST_FLOW)
	./$(ASAN_TEST_JITTER)
	./$(ASAN_TEST_FLOW)

clean:
	rm -f $(TARGET) \
	      $(TEST_JITTER) \
	      $(TEST_FLOW) \
	      $(ASAN_TARGET) \
	      $(ASAN_TEST_JITTER) \
	      $(ASAN_TEST_FLOW)
