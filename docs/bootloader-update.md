# Обновление загрузчика до 0.10.0
Загрузчик принимает новую прошивку по USB и по Bluetooth, сама прошивка его не меняет. На платах nice!nano и совместимых с ней обычно стоит Adafruit nRF52 Bootloader 0.6.0. В нём при сбое записи во флеш во время прошивки по Bluetooth загрузчик сам рвёт соединение и перезагружается, а старая прошивка к этому моменту уже стёрта. В 0.10.0 это исправлено ([список изменений](https://github.com/adafruit/Adafruit_nRF52_Bootloader/releases/tag/0.10.0)).

Ставьте именно 0.10.0. В 0.11.0 обновление по Bluetooth сломано ([adafruit/Adafruit_nRF52_Bootloader#399](https://github.com/adafruit/Adafruit_nRF52_Bootloader/issues/399)).

Плата IMBA nRF52840 ProMicro разведена как nice!nano (светодиод на P0.15, кнопки у загрузчика нет), поэтому нужна сборка `nice_nano`.

## Что нужно
* Файл [update-nice_nano_bootloader-0.10.0_nosd.uf2](https://github.com/adafruit/Adafruit_nRF52_Bootloader/releases/download/0.10.0/update-nice_nano_bootloader-0.10.0_nosd.uf2). Он обновляет только загрузчик, SoftDevice S140 6.1.1 остаётся прежним
* Кабель USB-C с передачей данных
* Не отключать кабель, пока идёт запись: если в этот момент пропадёт питание, плату получится восстановить только программатором (см. [ниже](#если-плата-не-отвечает))

## Узнать текущую версию
На дисплее версия видна на экране «Связь и версии», в строке «Загрузчик» (экран добавляется в список на сайте). Если там «--», откройте файл `INFO_UF2.TXT` на USB-диске загрузчика (как его открыть, см. шаг 1 ниже). В файле должно быть:
* `UF2 Bootloader 0.x.y`: если версия ниже 0.4.0, файл `.uf2` не сработает, обновляйте [через adafruit-nrfutil](#без-вскрытия-корпуса)
* `Board-ID: nRF52840-nicenano`: если указана другая плата, этот файл для неё не подходит

## Обновление через USB-диск
1. Откройте корпус, подключите плату по USB и дважды быстро замкните RST на GND. Появится диск `NICENANO`. Кнопка «Прошить» на сайте для этого не подходит: она переводит плату в загрузчик без диска, только с последовательным портом
2. Скопируйте на диск файл `update-nice_nano_bootloader-0.10.0_nosd.uf2`. Светодиод помигает, через несколько секунд диск пропадёт: загрузчик записан. Если Finder на macOS выдаёт ошибку при копировании, скопируйте через терминал: `cp -X ~/Downloads/update-nice_nano_bootloader-0.10.0_nosd.uf2 /Volumes/NICENANO/`
3. Снова дважды замкните RST на GND и проверьте, что в `INFO_UF2.TXT` стоит `UF2 Bootloader 0.10.0`
4. Прошейте IMBA TV заново: программа обновления записывается на место прошивки, и без этого дисплей не включится. Прошить можно на сайте по USB (порт загрузчика может называться nice!nano) или скопировать «UF2 с настройками» на диск `NICENANO`

После этого на экране «Связь и версии» будет «Загрузчик 0.10.0».

## Без вскрытия корпуса
Можно обновить через последовательный порт программой `adafruit-nrfutil`: она сама переводит плату в загрузчик, как сайт. Этот способ перезаписывает загрузчик вместе с SoftDevice (той же версии 6.1.1), поэтому запись идёт дольше и обрыв опаснее.

Нужен файл [nice_nano_bootloader-0.10.0_s140_6.1.1.zip](https://github.com/adafruit/Adafruit_nRF52_Bootloader/releases/download/0.10.0/nice_nano_bootloader-0.10.0_s140_6.1.1.zip). `adafruit-nrfutil` есть в ядре Adafruit nRF52 для Arduino (на macOS: `~/Library/Arduino15/packages/adafruit/hardware/nrf52/1.7.0/tools/adafruit-nrfutil/macos/adafruit-nrfutil`), без Arduino его можно поставить через `pip3 install --user adafruit-nrfutil`.

1. Включите дисплей и подключите его по USB. Порт покажет `make ports` или `ls /dev/cu.usbmodem*`
2. Запустите, подставив порт:
   ```
   adafruit-nrfutil --verbose dfu serial --package nice_nano_bootloader-0.10.0_s140_6.1.1.zip -p /dev/cu.usbmodemXXXX -b 115200 --singlebank --touch 1200
   ```
3. Если дисплей после этого не включается, прошейте IMBA TV на сайте по USB

## Если плата не отвечает
Если запись загрузчика оборвалась, плата не появляется ни как диск, ни как порт. Восстановить её можно только через выводы SWDIO и SWCLK программатором J-Link, ST-Link или Raspberry Pi Pico с прошивкой debugprobe, записав файл [nice_nano_bootloader-0.10.0_s140_6.1.1.hex](https://github.com/adafruit/Adafruit_nRF52_Bootloader/releases/download/0.10.0/nice_nano_bootloader-0.10.0_s140_6.1.1.hex) с полным стиранием чипа. Для J-Link: `nrfjprog --program nice_nano_bootloader-0.10.0_s140_6.1.1.hex -f nrf52 --chiperase --reset`. Затем прошейте IMBA TV по USB.
