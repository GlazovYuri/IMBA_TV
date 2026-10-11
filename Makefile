FQBN   ?= adafruit:nrf52:imba_pro_micro
SKETCH ?= IMBA_TV
# Порт определяется автоматически; можно задать вручную: make upload PORT=/dev/cu.usbmodemXXXX
PORT   ?= $(shell arduino-cli board list | awk '/imba_pro_micro|usbmodem|ttyACM/ {for(i=1;i<=NF;i++) if($$i ~ /^\/dev\//) {print $$i; exit}}')
# Порт веб-интерфейса эмулятора: make emu EMU_PORT=9000
EMU_PORT ?= 8080

.PHONY: build upload ports emu emu-san emu-test emu-clean

build:
	arduino-cli compile -b $(FQBN) $(SKETCH)

upload: build
	@test -n "$(PORT)" || { echo "Плата не найдена. Подключите её по USB или задайте PORT=..."; exit 1; }
	arduino-cli upload -p $(PORT) -b $(FQBN) $(SKETCH)

ports:
	arduino-cli board list

# Эмулятор платы (см. emulator/README.md)
# собрать и запустить, интерфейс на http://localhost:$(EMU_PORT)
emu:
	$(MAKE) -C emulator run PORT=$(EMU_PORT)

# то же с AddressSanitizer и UBSan: выход за границы массивов и подобное видно в журнале
emu-san:
	$(MAKE) -C emulator run SAN=1 PORT=$(EMU_PORT)

# автотесты: сценарии emulator/scripts/*.emu без браузера
emu-test:
	$(MAKE) -C emulator test

emu-clean:
	$(MAKE) -C emulator clean
