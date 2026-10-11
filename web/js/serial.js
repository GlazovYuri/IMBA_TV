// Последовательный порт для прошивки по USB-кабелю.
// На компьютере это Web Serial. В Chrome на Android его нет, зато есть WebUSB:
// web-serial-polyfill даёт поверх него тот же интерфейс для устройств USB CDC,
// а именно так видны по USB и прошивка, и загрузчик платы. На компьютере WebUSB
// не подходит: CDC-устройство там сразу занимает драйвер системы.
import { serial as usbSerial } from './vendor/web-serial-polyfill.js';

export const isAndroid = /Android/.test(navigator.userAgent);

export const serial = isAndroid && 'usb' in navigator ? usbSerial : navigator.serial ?? null;

export const serialApi = serial === usbSerial ? 'WebUSB' : serial ? 'Web Serial' : null;
