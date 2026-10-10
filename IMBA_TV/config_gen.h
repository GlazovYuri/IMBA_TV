#pragma once

// Сгенерировано tools/gen_config.py из config.json, не править руками.
// После изменения config.json запустите: python3 tools/gen_config.py

#include <stdint.h>

// Настройки прошивки. Значения записывает веб-конфигуратор перед прошивкой.
struct __attribute__((packed)) fw_config_t
{
  uint8_t screen_charge;  // Скорость, ШИМ и заряд колеса
  uint8_t screen_voltage;  // Скорость, ШИМ и напряжение колеса
  uint8_t screen_grid;  // Сетка: заряд, напряжение, температура, пробег
  uint8_t screen_info;  // Связь и версии: задержка и частота данных, версии прошивки и загрузчика
  uint16_t wheel_voltage_x10;  // Батарея колеса
  uint8_t pwm_alarm;  // Мигать экраном при превышении порога ШИМ
  uint8_t pwm_alarm_source;  // Порог
  uint8_t pwm_alarm_threshold;  // Свой порог ШИМ
  uint8_t brightness;  // Яркость
  uint8_t intro_logo;  // Логотип при включении
  uint8_t intro_wave;  // Волна
  uint8_t intro_break;  // Разрушение
  uint8_t intro_explosion;  // Взрыв
  uint8_t intro_clawd_type;  // Clawd печатает интерфейс
  uint8_t intro_clawd_laptop;  // Clawd: экран ноутбука
  uint8_t intro_clawd_pixels;  // Clawd: пересборка из пикселей
  uint8_t poweroff_animation;  // Анимация выключения
  uint16_t power_on_ms;  // Удержание для включения
  uint16_t power_off_ms;  // Удержание для выключения
  uint8_t ble_update;  // Разрешить обновление прошивки по Bluetooth
};

static_assert(sizeof(fw_config_t) == 23, "config.json и fw_config_t разошлись");

#define FW_CONFIG_DEFAULTS { 1, 1, 1, 1, 1512, 1, 0, 80, 100, 1, 1, 1, 1, 0, 0, 0, 1, 500, 1000, 1 }

enum
{
  CONFIG_WHEEL_VOLTAGE_X10_20S = 840,
  CONFIG_WHEEL_VOLTAGE_X10_24S = 1008,
  CONFIG_WHEEL_VOLTAGE_X10_30S = 1260,
  CONFIG_WHEEL_VOLTAGE_X10_32S = 1344,
  CONFIG_WHEEL_VOLTAGE_X10_36S = 1512,
  CONFIG_WHEEL_VOLTAGE_X10_40S = 1680,
  CONFIG_WHEEL_VOLTAGE_X10_42S = 1764,
  CONFIG_PWM_ALARM_SOURCE_APP = 0,
  CONFIG_PWM_ALARM_SOURCE_FIXED = 1,
};

// Значения вне допустимых заменяются значениями по умолчанию
static inline void configSanitize(fw_config_t& c)
{
  if(c.screen_charge > 1) c.screen_charge = 1;
  if(c.screen_voltage > 1) c.screen_voltage = 1;
  if(c.screen_grid > 1) c.screen_grid = 1;
  if(c.screen_info > 1) c.screen_info = 1;
  switch(c.wheel_voltage_x10) { case 840: case 1008: case 1260: case 1344: case 1512: case 1680: case 1764: break; default: c.wheel_voltage_x10 = 1512; }
  if(c.pwm_alarm > 1) c.pwm_alarm = 1;
  switch(c.pwm_alarm_source) { case 0: case 1: break; default: c.pwm_alarm_source = 0; }
  if(c.pwm_alarm_threshold < 30 || c.pwm_alarm_threshold > 99) c.pwm_alarm_threshold = 80;
  if(c.brightness < 5 || c.brightness > 100) c.brightness = 100;
  if(c.intro_logo > 1) c.intro_logo = 1;
  if(c.intro_wave > 1) c.intro_wave = 1;
  if(c.intro_break > 1) c.intro_break = 1;
  if(c.intro_explosion > 1) c.intro_explosion = 1;
  if(c.intro_clawd_type > 1) c.intro_clawd_type = 1;
  if(c.intro_clawd_laptop > 1) c.intro_clawd_laptop = 1;
  if(c.intro_clawd_pixels > 1) c.intro_clawd_pixels = 1;
  if(c.poweroff_animation > 1) c.poweroff_animation = 1;
  if(c.power_on_ms < 100 || c.power_on_ms > 3000) c.power_on_ms = 500;
  if(c.power_off_ms < 500 || c.power_off_ms > 5000) c.power_off_ms = 1000;
  if(c.ble_update > 1) c.ble_update = 1;
}

// Описание настроек для веб-конфигуратора: config.json с адресами полей
#define FW_CONFIG_SCHEMA \
  "{\"version\":2,\"groups\":[{\"title\":\"Экраны\",\"help\":\"Страницы, которые переключаются коротким нажати" \
  "ем кнопки\",\"require_one\":true,\"fields\":[{\"key\":\"screen_charge\",\"type\":\"bool\",\"default\":true,\"lab" \
  "el\":\"Скорость, ШИМ и заряд колеса\",\"offset\":0,\"size\":1},{\"key\":\"screen_voltage\",\"type\":\"bool\",\"d" \
  "efault\":true,\"label\":\"Скорость, ШИМ и напряжение колеса\",\"offset\":1,\"size\":1},{\"key\":\"screen_gri" \
  "d\",\"type\":\"bool\",\"default\":true,\"label\":\"Сетка: заряд, напряжение, температура, пробег\",\"offset\"" \
  ":2,\"size\":1},{\"key\":\"screen_info\",\"type\":\"bool\",\"default\":true,\"label\":\"Связь и версии: задержка" \
  " и частота данных, версии прошивки и загрузчика\",\"offset\":3,\"size\":1},{\"key\":\"wheel_voltage_x10\"" \
  ",\"type\":\"u16\",\"default\":1512,\"label\":\"Батарея колеса\",\"help\":\"По максимальному напряжению рассчи" \
  "тывается место под цифры на экране напряжения\",\"options\":[{\"value\":840,\"id\":\"20S\",\"label\":\"84 В " \
  "(20S)\"},{\"value\":1008,\"id\":\"24S\",\"label\":\"100,8 В (24S)\"},{\"value\":1260,\"id\":\"30S\",\"label\":\"126 " \
  "В (30S)\"},{\"value\":1344,\"id\":\"32S\",\"label\":\"134,4 В (32S)\"},{\"value\":1512,\"id\":\"36S\",\"label\":\"15" \
  "1,2 В (36S)\"},{\"value\":1680,\"id\":\"40S\",\"label\":\"168 В (40S)\"},{\"value\":1764,\"id\":\"42S\",\"label\":\"" \
  "176,4 В (42S)\"}],\"offset\":4,\"size\":2}]},{\"title\":\"Предупреждение о ШИМ\",\"help\":\"Мигание экрана, " \
  "когда ШИМ подходит к пределу\",\"fields\":[{\"key\":\"pwm_alarm\",\"type\":\"bool\",\"default\":true,\"label\":" \
  "\"Мигать экраном при превышении порога ШИМ\",\"help\":\"Чем выше ШИМ, тем чаще мигание\",\"offset\":6,\"s" \
  "ize\":1},{\"key\":\"pwm_alarm_source\",\"type\":\"u8\",\"default\":0,\"label\":\"Порог\",\"options\":[{\"value\":0," \
  "\"id\":\"APP\",\"label\":\"Из настроек приложения LoEUC\"},{\"value\":1,\"id\":\"FIXED\",\"label\":\"Свой\"}],\"dep" \
  "ends\":{\"pwm_alarm\":true},\"offset\":7,\"size\":1},{\"key\":\"pwm_alarm_threshold\",\"type\":\"u8\",\"default\"" \
  ":80,\"min\":30,\"max\":99,\"unit\":\"%\",\"label\":\"Свой порог ШИМ\",\"depends\":{\"pwm_alarm\":true,\"pwm_alarm" \
  "_source\":1},\"offset\":8,\"size\":1}]},{\"title\":\"Дисплей\",\"help\":\"Яркость экрана\",\"fields\":[{\"key\":\"" \
  "brightness\",\"type\":\"u8\",\"default\":100,\"min\":5,\"max\":100,\"step\":5,\"unit\":\"%\",\"widget\":\"range\",\"la" \
  "bel\":\"Яркость\",\"offset\":9,\"size\":1}]},{\"title\":\"Анимации\",\"help\":\"Заставка при включении и анима" \
  "ция выключения\",\"fields\":[{\"key\":\"intro_logo\",\"type\":\"bool\",\"default\":true,\"section\":\"При включе" \
  "нии\",\"label\":\"Логотип при включении\",\"help\":\"После логотипа играет случайная из отмеченных заста" \
  "вок. Без логотипа данные появляются сразу\",\"preview\":\"logo\",\"offset\":10,\"size\":1},{\"key\":\"intro_" \
  "wave\",\"type\":\"bool\",\"default\":true,\"label\":\"Волна\",\"help\":\"Волна с пузырями проходит слева напра" \
  "во и открывает данные\",\"preview\":\"wave\",\"depends\":{\"intro_logo\":true},\"subitem\":true,\"offset\":11" \
  ",\"size\":1},{\"key\":\"intro_break\",\"type\":\"bool\",\"default\":true,\"label\":\"Разрушение\",\"help\":\"Логоти" \
  "п трескается и рассыпается на куски\",\"preview\":\"break\",\"depends\":{\"intro_logo\":true},\"subitem\":t" \
  "rue,\"offset\":12,\"size\":1},{\"key\":\"intro_explosion\",\"type\":\"bool\",\"default\":true,\"label\":\"Взрыв\"," \
  "\"help\":\"Логотип разлетается облаком частиц, как TNT в Minecraft\",\"preview\":\"explosion\",\"depends\"" \
  ":{\"intro_logo\":true},\"subitem\":true,\"offset\":13,\"size\":1},{\"key\":\"intro_clawd_type\",\"type\":\"bool" \
  "\",\"default\":false,\"label\":\"Clawd печатает интерфейс\",\"help\":\"Clawd из Claude Code садится за ноу" \
  "тбук, стирает логотип и набирает данные с курсором\",\"preview\":\"clawd_type\",\"depends\":{\"intro_log" \
  "o\":true},\"subitem\":true,\"offset\":14,\"size\":1},{\"key\":\"intro_clawd_laptop\",\"type\":\"bool\",\"default" \
  "\":false,\"label\":\"Clawd: экран ноутбука\",\"help\":\"Логотип оказывается на экране ноутбука, Clawd пи" \
  "шет код, и камера въезжает в готовый экран\",\"preview\":\"clawd_laptop\",\"depends\":{\"intro_logo\":tru" \
  "e},\"subitem\":true,\"offset\":15,\"size\":1},{\"key\":\"intro_clawd_pixels\",\"type\":\"bool\",\"default\":fals" \
  "e,\"label\":\"Clawd: пересборка из пикселей\",\"help\":\"Ноутбук Clawd втягивает логотип и выпускает пи" \
  "ксели данных на их места\",\"preview\":\"clawd_pixels\",\"depends\":{\"intro_logo\":true},\"subitem\":true," \
  "\"offset\":16,\"size\":1},{\"key\":\"poweroff_animation\",\"type\":\"bool\",\"default\":true,\"section\":\"При вы" \
  "ключении\",\"label\":\"Анимация выключения\",\"help\":\"Экран сжимается в линию, затем в точку и гаснет," \
  " как у старого телевизора\",\"preview\":\"poweroff\",\"offset\":17,\"size\":1}]},{\"title\":\"Кнопка\",\"help\"" \
  ":\"Сколько держать кнопку, чтобы включить и выключить дисплей\",\"fields\":[{\"key\":\"power_on_ms\",\"ty" \
  "pe\":\"u16\",\"default\":500,\"min\":100,\"max\":3000,\"step\":100,\"unit\":\"мс\",\"label\":\"Удержание для включ" \
  "ения\",\"offset\":18,\"size\":2},{\"key\":\"power_off_ms\",\"type\":\"u16\",\"default\":1000,\"min\":500,\"max\":50" \
  "00,\"step\":100,\"unit\":\"мс\",\"label\":\"Удержание для выключения\",\"help\":\"Более короткое нажатие пере" \
  "ключает экран\",\"offset\":20,\"size\":2}]},{\"title\":\"Обновление по Bluetooth\",\"help\":\"Прошивка с тел" \
  "ефона без кабеля\",\"fields\":[{\"key\":\"ble_update\",\"type\":\"bool\",\"default\":true,\"label\":\"Разрешить " \
  "обновление прошивки по Bluetooth\",\"help\":\"Режим обновления включается, только если при включении" \
  " держать кнопку ещё 3 секунды после того, как загорится экран, и заряд не ниже 30%. В обычной ра" \
  "боте прошить дисплей по Bluetooth нельзя\",\"offset\":22,\"size\":1}]}]}"
