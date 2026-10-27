CC := gcc
CXX := g++

CFLAGS := -Iinclude -std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -g -O0
CXXFLAGS := -Iinclude -std=c++17 -Wall -Wextra -Wpedantic -g -O0
MISRA_CFLAGS := -Iinclude -std=c11 -D_POSIX_C_SOURCE=200809L \
	-Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wcast-qual \
	-Wpointer-arith -Wstrict-prototypes -Wmissing-prototypes \
	-Wold-style-definition -Wswitch-enum -Wundef -Wvla -Wformat=2 \
	-Werror -g -O0

APP := build/tatkal_poc
TEST := gtest/runTest

APP_SRC := src/main.c src/config.c src/memory.c src/queue.c src/booking.c src/output.c
LIB_SRC := src/config.c src/memory.c src/queue.c src/booking.c src/output.c
APP_OBJ := $(APP_SRC:src/%.c=build/%.o)
TEST_OBJ := $(LIB_SRC:src/%.c=build/gtest/%.o)

.PHONY: all build misra-compile test run coverage cppcheck valgrind clean

all: build

build: $(APP)

$(APP): $(APP_OBJ)
	$(CC) $^ -pthread -o $@

build/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

misra-compile:
	@rm -rf build/misra
	@mkdir -p build/misra
	@for source in $(APP_SRC); do \
		object=build/misra/$$(basename "$${source%.c}.o"); \
		$(CC) $(MISRA_CFLAGS) -c "$$source" -pthread -o "$$object" || exit 1; \
	done
	$(CC) build/misra/*.o -pthread -o build/misra/tatkal_poc

test: $(TEST)
	./$(TEST)

$(TEST): build/gtest/functionalTesting.o $(TEST_OBJ)
	$(CXX) $^ -pthread -lgtest_main -lgtest -o $@

build/gtest/functionalTesting.o: gtest/functionalTesting.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

build/gtest/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

run: $(APP)
	./$(APP)

coverage:
	@rm -rf build/coverage
	@mkdir -p build/coverage
	$(CXX) $(CXXFLAGS) --coverage -c gtest/functionalTesting.cpp -o build/coverage/functionalTesting.o
	@for source in $(LIB_SRC); do \
		object=build/coverage/$$(basename "$${source%.c}.o"); \
		$(CC) $(CFLAGS) --coverage -c "$$source" -o "$$object" || exit 1; \
	done
	$(CXX) --coverage build/coverage/*.o -pthread -lgtest_main -lgtest -o build/coverage/runTest
	./build/coverage/runTest
	@for source in config.c memory.c queue.c booking.c output.c; do \
		gcov -b -c -o build/coverage "src/$$source" && \
		mv "$$source.gcov" build/coverage/; \
	done

cppcheck:
	cppcheck --enable=warning,style,performance,portability \
		--error-exitcode=1 --suppress=missingIncludeSystem src

valgrind: $(APP)
	@mkdir -p artifacts/valgrind
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
		./$(APP) --seats 10 --requests 100 --workers 4 \
		--output-dir artifacts/valgrind

clean:
	rm -rf build gtest/runTest
