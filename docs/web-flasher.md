# Прошивка через браузер
Самый простой способ обновить дисплей — сайт [glazovyuri.github.io/IMBA_TV](https://glazovyuri.github.io/IMBA_TV/):
1. Откройте сайт в Chrome, Edge, Яндекс Браузере или Opera на компьютере (нужна поддержка Web Serial)
2. Выберите версию прошивки или загрузите свой файл (.zip из Arduino IDE, .hex или .uf2)
3. Подключите устройство по USB, нажмите «Прошить» и выберите порт IMBA TV

Плата сама перезагрузится в режим загрузчика. В первый раз браузер попросит выбрать порт загрузчика ещё раз.

По кабелю можно прошить и с Android-телефона в Chrome: нужен кабель USB-C на USB-C или OTG-переходник. В Chrome на Android нет Web Serial, поэтому сайт работает с платой через WebUSB и библиотеку [web-serial-polyfill](https://github.com/google/web-serial-polyfill) (лежит в [web/js/vendor](../web/js/vendor/)). На iPhone прошивка по кабелю невозможна, а прошивка по Bluetooth пока отключена ([bluetooth-update.md](bluetooth-update.md)).

Там же можно скачать .uf2: при двойном замыкании RST на GND плата появляется как USB-диск, и файл достаточно скопировать на него.
