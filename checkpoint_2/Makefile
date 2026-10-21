# Compilacao isolada da segunda etapa (montador SIC/XE).
# Nao interfere com o Makefile do checkpoint_1.
CXX       ?= g++
CXXFLAGS  ?= -std=c++17 -Wall -Wextra -Wpedantic -O2
CPPFLAGS  += -Iinclude -Istructures/include
DEPFLAGS  := -MMD -MP

TARGET    := assembler
TEST_BIN  := build/test_assembler

# Modulos do montador entram automaticamente quando os demais integrantes
# adicionarem seus arquivos em src/, sem precisar alterar este Makefile.
SRC       := $(wildcard *.cpp src/*.cpp)

# Ponte --load: vincula apenas o nucleo do simulador, sem a interface TUI.
SIM_SRC   := $(addprefix structures/src/,CPU.cpp Loader.cpp Machine.cpp Memory.cpp Registers.cpp)
SRC       += $(SIM_SRC)
OBJ       := $(patsubst %.cpp,build/%.o,$(SRC))

TEST_SRC  := $(wildcard tests/*.cpp)
TEST_OBJ  := $(patsubst %.cpp,build/%.o,$(TEST_SRC))
LIB_OBJ   := $(filter-out build/main.o,$(OBJ))

.PHONY: all test clean run
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $@

build/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

ifeq ($(strip $(TEST_SRC)),)
test:
	@echo "ERRO: nenhum teste encontrado em tests/*.cpp (aguardando integrante 7)." >&2
	@exit 1
else
test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): $(LIB_OBJ) $(TEST_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@
endif

run: $(TARGET)
	@if [ -z "$(INPUT)" ]; then \
		echo "Uso: make run INPUT=examples/valid.asm" >&2; exit 2; \
	fi
	./$(TARGET) "$(INPUT)"

clean:
	rm -rf build $(TARGET)

-include $(OBJ:.o=.d) $(TEST_OBJ:.o=.d)
