# Compilacao isolada da segunda etapa (montador SIC/XE).
# Nao interfere com o Makefile do checkpoint_1.
CXX       ?= g++
CXXFLAGS  ?= -std=c++17 -Wall -Wextra -Wpedantic -O2
CPPFLAGS  += -Iinclude -Istructures/include
DEPFLAGS  := -MMD -MP

# No Windows/MinGW o arquivo fisico geralmente termina em .exe.
# Com Git Bash/MSYS2 ainda e possivel executar usando ./assembler.
EXEEXT    := $(if $(filter Windows_NT,$(OS)),.exe,)
TARGET    := assembler$(EXEEXT)
TEST_BIN  := build/test_assembler$(EXEEXT)

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

# Diagnostico claro enquanto os arquivos da parte 5 ainda nao estiverem
# presentes. Nao acrescenta implementacoes provisórias ao repositorio.
.PHONY: all test clean run check-integration
all: check-integration $(TARGET)

check-integration:
	@for f in include/ObjectProgram.h include/SimulatorBridge.h \
	          src/ObjectProgram.cpp src/SimulatorBridge.cpp; do \
	    if [ ! -f "$$f" ]; then \
	        echo "ERRO: falta $$f (aguardando os arquivos do integrante 5)." >&2; \
	        exit 1; \
	    fi; \
	done

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
test: check-integration $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): $(LIB_OBJ) $(TEST_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@
endif

run: check-integration $(TARGET)
	@if [ -z "$(INPUT)" ]; then \
		echo "Uso: make run INPUT=examples/valid.asm" >&2; exit 2; \
	fi
	./$(TARGET) "$(INPUT)"

clean:
	rm -rf build assembler assembler.exe

-include $(OBJ:.o=.d) $(TEST_OBJ:.o=.d)
