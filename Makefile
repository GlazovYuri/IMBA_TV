FQBN   ?= adafruit:nrf52:imba_pro_micro
SKETCH ?= IMBA_TV
# Порт определяется автоматически; можно задать вручную: make upload PORT=/dev/cu.usbmodemXXXX
PORT   ?= $(shell arduino-cli board list | awk '/imba_pro_micro|usbmodem|ttyACM/ {for(i=1;i<=NF;i++) if($$i ~ /^\/dev\//) {print $$i; exit}}')

.PHONY: build upload ports

build:
	arduino-cli compile -b $(FQBN) $(SKETCH)

upload: build
	@test -n "$(PORT)" || { echo "Плата не найдена. Подключите её по USB или задайте PORT=..."; exit 1; }
	arduino-cli upload -p $(PORT) -b $(FQBN) $(SKETCH)

ports:
	arduino-cli board list
