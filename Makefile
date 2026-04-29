CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -pedantic -Isrc
UI_CXXFLAGS := $(CXXFLAGS) -Wno-missing-field-initializers
UNAME_S := $(shell uname -s 2>/dev/null)
RAYLIB_CFLAGS := $(shell pkg-config --cflags raylib 2>/dev/null)
RAYLIB_LIBS := $(shell pkg-config --libs raylib 2>/dev/null)
THREAD_LIBS := -pthread

ifeq ($(RAYLIB_LIBS),)
	ifeq ($(UNAME_S),Darwin)
		RAYLIB_CFLAGS := -I/opt/homebrew/include -I/usr/local/include -I/opt/homebrew/opt/raylib/include -I/usr/local/opt/raylib/include
		RAYLIB_LIBS := -L/opt/homebrew/lib -L/usr/local/lib -L/opt/homebrew/opt/raylib/lib -L/usr/local/opt/raylib/lib -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
	else ifeq ($(UNAME_S),Linux)
		RAYLIB_LIBS := -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
	else
		RAYLIB_LIBS := -lraylib
	endif
endif

CORE_SRC := \
	src/core/card.cpp \
	src/core/deck.cpp \
	src/core/evaluator.cpp \
	src/core/game_state.cpp \
	src/core/simulation.cpp

UI_SRC := \
	src/main_raylib.cpp \
	src/ui/raylib_ui.cpp

ENGINE_BIN := poker_engine
TEST_BIN := poker_tests
UI_BIN := poker_ui

.PHONY: all engine ui check-raylib test clean

all: engine

# Raw equivalent:
# g++ -std=c++17 -Wall -Wextra -pedantic -Isrc src/main_console.cpp src/core/*.cpp -o poker_engine
engine:
	$(CXX) $(CXXFLAGS) src/main_console.cpp $(CORE_SRC) -o $(ENGINE_BIN)

# Preferred portable raw equivalent when Raylib provides pkg-config metadata:
# g++ -std=c++17 -Wall -Wextra -pedantic -Isrc $(pkg-config --cflags raylib) src/main_raylib.cpp src/core/*.cpp -o poker_ui $(pkg-config --libs raylib) -pthread
check-raylib:
	@printf '#include <raylib.h>\nint main(){return 0;}\n' | $(CXX) $(UI_CXXFLAGS) $(RAYLIB_CFLAGS) -x c++ -fsyntax-only - >/dev/null 2>&1 || \
	( echo "Raylib headers were not found."; \
	  echo "Install Raylib first, then rerun make ui."; \
	  echo "Recommended portable setup: install Raylib plus pkg-config support for your OS."; \
	  echo "macOS example: brew install raylib pkg-config"; \
	  echo "Ubuntu/Debian example: sudo apt install libraylib-dev pkg-config"; \
	  echo "Windows/MSYS2 example: pacman -S mingw-w64-ucrt-x86_64-raylib pkgconf"; \
	  exit 1 )

ui: check-raylib
	$(CXX) $(UI_CXXFLAGS) $(RAYLIB_CFLAGS) $(UI_SRC) $(CORE_SRC) -o $(UI_BIN) $(RAYLIB_LIBS) $(THREAD_LIBS)

test:
	$(CXX) $(CXXFLAGS) tests/core_tests.cpp $(CORE_SRC) -o $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -f $(ENGINE_BIN) $(TEST_BIN) $(UI_BIN)
