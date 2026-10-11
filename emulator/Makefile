# Эмулятор платы IMBA TV: прошивка собирается под компьютер с моделью железа.
#   make            - собрать
#   make run        - собрать и открыть веб-интерфейс (http://localhost:8080)
#   make test       - прогнать сценарий scripts/smoke.emu без интерфейса
#   make SAN=1 ...  - собрать с AddressSanitizer/UBSan (ловит выход за границы массивов и т.п.)

ROOT  := ..
FW    := $(ROOT)/IMBA_TV
BOARD := $(ROOT)/boards/imba_pro_micro
BUILD := build$(if $(SAN),-san)
BIN   := $(BUILD)/imba_emu
PORT  ?= 8080

# GyverOLED: берём установленную в Arduino IDE, иначе скачиваем зафиксированную версию
GYVEROLED_REPO   := https://github.com/GyverLibs/GyverOLED
GYVEROLED_COMMIT := 0fc70748b1bbffc325263a8f38198371e310f9a9
GYVEROLED_DIR ?= $(firstword $(wildcard \
	$(HOME)/Documents/Arduino/libraries/GyverOLED/src \
	$(HOME)/Arduino/libraries/GyverOLED/src) .deps/GyverOLED/src)

CXX      ?= c++
CXXFLAGS ?= -O1 -g
CXXFLAGS += -std=c++17 -Wall -Wno-unused-function -Wno-unused-variable -pthread -MMD -MP \
	-DARDUINO=10819 -DARDUINO_NRF52_ADAFRUIT -DNRF52840_XXAA -DIMBA_EMULATOR
ifdef SAN
CXXFLAGS += -fsanitize=address,undefined -fno-omit-frame-pointer
LDFLAGS  += -fsanitize=address,undefined
endif

FW_INC  := -Isrc/hal -I$(BOARD) -I$(GYVEROLED_DIR)
EMU_INC := -Isrc $(FW_INC)

FW_SRC  := $(wildcard $(FW)/*.cpp)
FW_OBJ  := $(patsubst $(FW)/%.cpp,$(BUILD)/fw/%.o,$(FW_SRC)) $(BUILD)/fw/IMBA_TV.ino.o
EMU_OBJ := $(BUILD)/emu/main.o $(BUILD)/emu/hal.o

.PHONY: all run test clean
all: $(BIN)

$(GYVEROLED_DIR)/GyverOLED.h:
	@echo "GyverOLED не найдена в Arduino IDE - скачиваю в emulator/.deps"
	rm -rf .deps/GyverOLED && mkdir -p .deps/GyverOLED
	cd .deps/GyverOLED && git init -q && git fetch -q --depth 1 $(GYVEROLED_REPO) $(GYVEROLED_COMMIT) && git checkout -q FETCH_HEAD

$(BUILD)/fw/%.o: $(FW)/%.cpp | $(GYVEROLED_DIR)/GyverOLED.h
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(FW_INC) -c $< -o $@

$(BUILD)/fw/IMBA_TV.ino.o: $(FW)/IMBA_TV.ino | $(GYVEROLED_DIR)/GyverOLED.h
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(FW_INC) -x c++ -include Arduino.h -c $< -o $@

$(BUILD)/emu/%.o: src/%.cpp | $(GYVEROLED_DIR)/GyverOLED.h
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(EMU_INC) -DEMU_WEB_DIR='"$(abspath web)"' -c $< -o $@

$(BIN): $(FW_OBJ) $(EMU_OBJ) tools/gen_syms.py
	$(CXX) $(CXXFLAGS) $(FW_OBJ) $(EMU_OBJ) $(LDFLAGS) -o $@
	python3 tools/gen_syms.py $@ $@.syms $(FW_OBJ)

run: $(BIN)
	./$(BIN) --port $(PORT)

test: $(BIN)
	./$(BIN) --headless --rate 4 --script scripts/smoke.emu

clean:
	rm -rf build build-san

-include $(FW_OBJ:.o=.d) $(EMU_OBJ:.o=.d)
