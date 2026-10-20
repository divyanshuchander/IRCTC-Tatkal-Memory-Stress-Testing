CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Iinclude -g -O0
LDFLAGS := -pthread
TARGET := build/tatkal_poc
SOURCES := src/main.c src/config.c src/memory.c src/queue.c src/booking.c src/output.c

.PHONY: all debug release clean run cppcheck valgrind

all: debug

debug: CFLAGS += -O0

debug: $(TARGET)

release: CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -O2
release: $(TARGET)

$(TARGET): $(SOURCES)
	@mkdir -p build
	$(CC) $(CFLAGS) $(SOURCES) $(LDFLAGS) -o $@

run: debug
	./$(TARGET)

cppcheck:
	cppcheck --enable=warning,style,performance,portability --error-exitcode=1 --suppress=missingIncludeSystem src

valgrind: debug
	@mkdir -p artifacts/valgrind
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TARGET) --seats 10 --requests 100 --workers 4 --output-dir artifacts/valgrind

clean:
	rm -rf build
