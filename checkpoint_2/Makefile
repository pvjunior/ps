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

# Verifica a disponibilidade dos arquivos da parte 5. A compilacao
# tambem depende da API publica compativel entre PassTwo.h e ObjectProgram.h.
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
# Testes de fumaca executados pela CLI caso o modulo de testes ainda nao exista.
# Os arquivos temporarios ficam em build/smoke/, sem poluir o projeto.
test: check-integration $(TARGET)
	@mkdir -p build/smoke
	@printf '%s\n' "COPY START 1000" "FIRST LDA #3" "      CLEAR X" \
	    "      +JSUB SUBR" "      STA BUFFER,X" "      BYTE C'EOF'" \
	    "BUFFER RESW 1" "SUBR  RSUB" "      END FIRST" > build/smoke/valid.asm
	@./$(TARGET) build/smoke/valid.asm -o build/smoke/valid.obj --load
	@grep -q '^H' build/smoke/valid.obj
	@grep -q '^T' build/smoke/valid.obj
	@grep -q '^M' build/smoke/valid.obj
	@grep -q '^E' build/smoke/valid.obj
	@printf '%s\n' "BAD START 1000" "    LDA MISSING" "    END" > build/smoke/invalid.asm
	@cp build/smoke/valid.obj build/smoke/invalid.obj
	@if ./$(TARGET) build/smoke/invalid.asm -o build/smoke/invalid.obj; then \
	    echo "ERRO: o montador aceitou um simbolo inexistente" >&2; exit 1; \
	 fi
	@cmp -s build/smoke/valid.obj build/smoke/invalid.obj || \
	    (echo "ERRO: objeto anterior foi sobrescrito em caso de falha" >&2; exit 1)
	@echo "Smoke tests: CLI valida, --load, H/T/M/E e preservacao de objeto: OK"
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
