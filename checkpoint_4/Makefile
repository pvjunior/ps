# Checkpoint 4 - Carregador SIC/XE (integrante 6).
CXX      ?= g++
CXXFLAGS ?= -Wall -Wextra -Wpedantic -O2
CXXFLAGS += -std=c++17
DEPFLAGS := -MMD -MP
.DEFAULT_GOAL := all

# Reutiliza o nucleo do simulador, sem o main nem a interface de terminal.
# SIM_DIR pode ser redefinido caso a equipe centralize a API em outro checkpoint.
SIM_DIR  ?= ../checkpoint_1/structures
CPPFLAGS += -Iinclude -I$(SIM_DIR)/include
EXEEXT   := $(if $(filter Windows_NT,$(OS)),.exe,)
TARGET   := object_loader$(EXEEXT)
TEST_BIN := build/test_object_loader$(EXEEXT)

# Lista explicita: nao incorpora as copias dos modulos de macros do CP3.
LOADER_MODULES := ObjectParser LoadPlanner ObjectLoader
# ObjectLoader ja fornece fallback quando o Relocator ainda nao foi publicado.
# Se apenas um dos dois arquivos existir, o check-integration acusa a falta do outro.
RELOCATOR_FILES := include/Relocator.h src/Relocator.cpp
ifneq ($(strip $(wildcard $(RELOCATOR_FILES))),)
LOADER_MODULES += Relocator
RELOCATOR_MODE := external
else
RELOCATOR_MODE := fallback
endif
RELOCATOR_STAMP := build/.relocator-$(RELOCATOR_MODE)
LOADER_SRC := $(addprefix src/,$(addsuffix .cpp,$(LOADER_MODULES)))
SIM_MODULES := CPU Loader Machine Memory Registers
SIM_SRC := $(addprefix $(SIM_DIR)/src/,$(addsuffix .cpp,$(SIM_MODULES)))
REQUIRED_FILES := include/LoaderTypes.h \
    $(addprefix include/,$(addsuffix .h,$(LOADER_MODULES))) $(LOADER_SRC) \
    $(addprefix $(SIM_DIR)/include/,$(addsuffix .h,$(SIM_MODULES))) $(SIM_SRC)

LOCAL_OBJ := build/main.o $(patsubst %.cpp,build/%.o,$(LOADER_SRC))
SIM_OBJ := $(addprefix build/simulator/,$(addsuffix .o,$(SIM_MODULES)))
OBJECTS := $(LOCAL_OBJ) $(SIM_OBJ)
LIB_OBJ := $(filter-out build/main.o,$(OBJECTS))
TEST_SRC := $(wildcard tests/test_object_loader.cpp)
TEST_OBJ := $(patsubst %.cpp,build/%.o,$(TEST_SRC))

.PHONY: all status check-integration test smoke clean
all: $(TARGET)

status:
	@echo "Modo do relocador: $(RELOCATOR_MODE)"
	@for f in $(REQUIRED_FILES); do \
	    if [ -f "$$f" ]; then printf 'OK       %s\n' "$$f"; \
	    else printf 'PENDENTE %s\n' "$$f"; fi; \
	done

check-integration:
ifeq ($(RELOCATOR_MODE),fallback)
	@echo 'Modo de relocacao: integrado ao ObjectLoader.'
endif
	@missing=0; for f in $(REQUIRED_FILES); do \
	    if [ ! -f "$$f" ]; then echo "PENDENTE: $$f" >&2; missing=1; fi; \
	done; \
	if [ $$missing -ne 0 ]; then \
	    echo "ERRO: faltam modulos do carregador. Consulte make status." >&2; exit 1; \
	fi

# Forca a recompilacao da fachada ao mudar entre fallback e modulo externo,
# inclusive se Relocator.h ainda nao constava no arquivo de dependencias .d.
$(RELOCATOR_STAMP): | check-integration
	@mkdir -p build
	@rm -f build/.relocator-fallback build/.relocator-external
	@touch $@

build/src/ObjectLoader.o: $(RELOCATOR_STAMP) $(wildcard include/Relocator.h)

# A verificacao deve acontecer antes dos objetos, inclusive com make -j.
$(OBJECTS) $(TEST_OBJ): | check-integration

$(TARGET): $(OBJECTS) | check-integration
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

build/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

build/simulator/%.o: $(SIM_DIR)/src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

# Testes basicos da CLI real; a suite detalhada e a Parte 7.
# Objeto minimo sem M: 3 bytes em 0x1000, com entrada em 0x1000.
smoke: $(TARGET)
	@mkdir -p build/smoke
	@printf '%s\n' 'HTEST  001000000003' 'T00100003010003' 'E001000' > build/smoke/valid.obj
	@./$(TARGET) build/smoke/valid.obj > build/smoke/default.log
	@grep -q '^Base de carga: 0x001000$$' build/smoke/default.log
	@grep -q '^Bytes carregados: 3$$' build/smoke/default.log
	@grep -q '^Endereco de entrada: 0x001000$$' build/smoke/default.log
	@grep -q '^PC final: 0x001000$$' build/smoke/default.log
	@./$(TARGET) build/smoke/valid.obj --load-address 2000 > build/smoke/relocated.log
	@grep -q '^Base de carga: 0x002000$$' build/smoke/relocated.log
	@grep -q '^Bytes carregados: 3$$' build/smoke/relocated.log
	@grep -q '^Endereco de entrada: 0x002000$$' build/smoke/relocated.log
	@grep -q '^PC final: 0x002000$$' build/smoke/relocated.log
	@status=0; ./$(TARGET) build/smoke/valid.obj --load-address ZZ \
	    > build/smoke/usage.log 2>&1 || status=$$?; \
	    if [ $$status -ne 2 ]; then echo 'ERRO: CLI deveria retornar 2.' >&2; exit 1; fi
	@printf '%s\n' 'INVALIDO' > build/smoke/invalid.obj
	@status=0; ./$(TARGET) build/smoke/invalid.obj \
	    > build/smoke/invalid.log 2>&1 || status=$$?; \
	    if [ $$status -ne 1 ]; then echo 'ERRO: carga invalida deveria retornar 1.' >&2; exit 1; fi
	@echo 'Smoke tests: base padrao, base explicita, PC e codigos de saida: OK'

ifneq ($(strip $(TEST_SRC)),)
$(TEST_BIN): $(LIB_OBJ) $(TEST_OBJ) | check-integration
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

test: smoke $(TEST_BIN)
	./$(TEST_BIN)
else
test: smoke
	@echo 'Testes basicos de integracao concluidos.'
endif

clean:
	rm -rf build object_loader object_loader.exe

-include $(OBJECTS:.o=.d) $(TEST_OBJ:.o=.d)
