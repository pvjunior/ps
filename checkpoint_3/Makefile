# Checkpoint 3 - Macro-montador SIC/XE (integrante 6).
# Compilacao isolada, reutilizando as APIs do montador do Checkpoint 2.
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -O2
CPPFLAGS += -Iinclude -I../checkpoint_2/include -I../checkpoint_2/structures/include
DEPFLAGS := -MMD -MP

# Em Windows/MSYS2, o executavel fisico recebe extensao .exe.
EXEEXT   := $(if $(filter Windows_NT,$(OS)),.exe,)
TARGET   := macro_assembler$(EXEEXT)
TEST_BIN := build/test_macro_processor$(EXEEXT)

# Fontes proprios do Checkpoint 3: nao compilar o main do Checkpoint 2.
LOCAL_SRC := main.cpp $(wildcard src/*.cpp)
LOCAL_OBJ := $(patsubst %.cpp,build/%.o,$(LOCAL_SRC))

# Reaproveita o montador existente sem editar/copy-paste de seus modulos.
ASM_SRC  := $(wildcard ../checkpoint_2/src/*.cpp)
ASM_OBJ  := $(patsubst ../checkpoint_2/%.cpp,build/checkpoint_2/%.o,$(ASM_SRC))

# SimulatorBridge.cpp depende do nucleo da Machine; nao incluir TerminalUI.cpp.
SIM_SRC  := $(addprefix ../checkpoint_2/structures/src/,CPU.cpp Loader.cpp Machine.cpp Memory.cpp Registers.cpp)
SIM_OBJ  := $(patsubst ../checkpoint_2/%.cpp,build/checkpoint_2/%.o,$(SIM_SRC))

OBJECTS  := $(LOCAL_OBJ) $(ASM_OBJ) $(SIM_OBJ)
LIB_OBJ  := $(filter-out build/main.o,$(OBJECTS))
TEST_SRC := $(wildcard tests/*.cpp)
TEST_OBJ := $(patsubst %.cpp,build/%.o,$(TEST_SRC))

.PHONY: all test smoke clean run check-integration
all: $(TARGET)

# Sem a fachada real do integrante 5, nao finja que a expansao esta pronta.
check-integration:
	@if [ ! -f include/MacroAssemblerPipeline.h ] || \
	    [ ! -f src/MacroAssemblerPipeline.cpp ]; then \
		echo "ERRO: falta a fachada MacroAssemblerPipeline (integrante 5)." >&2; \
		echo "A CLI esta preparada; combine a API indicada em README.md." >&2; \
		exit 1; \
	fi

$(TARGET): $(OBJECTS) | check-integration
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $@

build/main.o: | check-integration

build/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

build/checkpoint_2/%.o: ../checkpoint_2/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

ifneq ($(strip $(TEST_SRC)),)
$(TEST_BIN): $(LIB_OBJ) $(TEST_OBJ) | check-integration
	$(CXX) $(CXXFLAGS) $^ -o $@

test: $(TEST_BIN) smoke
	./$(TEST_BIN)
else
test: smoke
	@echo "AVISO: tests/*.cpp ainda nao existe; executados apenas testes de CLI."
endif

# Testes de ponta a ponta com uma fonte sem macros (que deve passar inalterada).
# O integrante 7 acrescentara os casos de macros simples/aninhadas/erros.
smoke: $(TARGET)
	@mkdir -p build/smoke
	@printf '%s\n' 'COPY START 1000' 'FIRST LDA #3' '      STA VALUE' \
	    'VALUE RESW 1' '      END FIRST' > build/smoke/valid.asm
	@./$(TARGET) build/smoke/valid.asm -o build/smoke/valid.obj \
	    --expanded build/smoke/expanded.asm
	@grep -q '^H' build/smoke/valid.obj
	@grep -q '^T' build/smoke/valid.obj
	@grep -q '^E' build/smoke/valid.obj
	@cp build/smoke/valid.obj build/smoke/old.obj
	@printf '%s\n' 'BAD START 1000' '    LDA MISSING' '    END' \
	    > build/smoke/invalid.asm
	@status=0; ./$(TARGET) build/smoke/invalid.asm \
	    -o build/smoke/valid.obj --expanded build/smoke/expanded.asm \
	    > build/smoke/invalid.log 2>&1 || status=$$?; \
	    if [ $$status -ne 1 ]; then \
	        echo "ERRO: fonte invalido deveria retornar codigo 1." >&2; exit 1; fi
	@cmp -s build/smoke/valid.obj build/smoke/old.obj || \
	    (echo "ERRO: objeto anterior sobrescrito em caso de falha." >&2; exit 1)
	@status=0; ./$(TARGET) > build/smoke/usage.log 2>&1 || status=$$?; \
	    if [ $$status -ne 2 ]; then \
	        echo "ERRO: argumentos ausentes deveriam retornar codigo 2." >&2; exit 1; fi
	@echo "Smoke tests: CLI, montagem, codigos de saida e objeto preservado: OK"

run: $(TARGET)
	@if [ -z "$(INPUT)" ]; then \
		echo "Uso: make run INPUT=arquivo.asm [ARGS='-o saida.obj']" >&2; exit 2; \
	fi
	./$(TARGET) "$(INPUT)" $(ARGS)

clean:
	rm -rf build macro_assembler macro_assembler.exe

-include $(OBJECTS:.o=.d) $(TEST_OBJ:.o=.d)
