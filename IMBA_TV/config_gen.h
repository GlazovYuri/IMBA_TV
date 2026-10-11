#pragma once

// Сгенерировано tools/gen_config.py из config.json, не править руками.
// После изменения config.json запустите: python3 tools/gen_config.py

#include <stdint.h>

// Настройки прошивки. Значения записывает веб-конфигуратор перед прошивкой.
struct __attribute__((packed)) fw_config_t
{
  uint8_t screen_charge;  // Основной: два крупных значения и заряд колеса
  uint8_t screen_voltage;  // Основной: два крупных значения и напряжение колеса
  uint8_t screen_grid;  // Сетка из четырёх значений
  uint8_t grid_1;  // Слева сверху
  uint8_t grid_2;  // Справа сверху
  uint8_t grid_3;  // Слева снизу
  uint8_t grid_4;  // Справа снизу
  uint8_t screen_info;  // Связь и версии: задержка и частота данных, версии прошивки и загрузчика
  uint16_t wheel_voltage_x10;  // Батарея колеса
  uint8_t main_left;  // Крупное значение слева
  uint8_t main_right;  // Крупное значение справа
  uint8_t pwm_alarm;  // Мигать экраном при превышении порога ШИМ
  uint8_t pwm_alarm_source;  // Порог
  uint8_t pwm_alarm_threshold;  // Свой порог ШИМ
  uint8_t brightness;  // Яркость
  uint8_t intro_logo;  // Логотип при включении
  uint8_t intro_clawd;  // Заставки Clawd
  uint8_t intro_wave;  // Волна
  uint8_t intro_break;  // Разрушение
  uint8_t intro_explosion;  // Взрыв
  uint8_t poweroff_animation;  // Анимация выключения
  uint16_t power_on_ms;  // Удержание для включения
  uint16_t power_off_ms;  // Удержание для выключения
  uint8_t ble_update;  // Разрешить обновление прошивки по Bluetooth
};

static_assert(sizeof(fw_config_t) == 27, "config.json и fw_config_t разошлись");

#define FW_CONFIG_DEFAULTS { 1, 1, 1, 2, 3, 4, 5, 1, 1512, 0, 1, 1, 0, 80, 100, 1, 0, 1, 1, 1, 1, 500, 1000, 1 }

enum
{
  CONFIG_GRID_1_SPEED = 0,
  CONFIG_GRID_1_PWM = 1,
  CONFIG_GRID_1_CHARGE = 2,
  CONFIG_GRID_1_VOLTAGE = 3,
  CONFIG_GRID_1_TEMPERATURE = 4,
  CONFIG_GRID_1_TRIP = 5,
  CONFIG_GRID_2_SPEED = 0,
  CONFIG_GRID_2_PWM = 1,
  CONFIG_GRID_2_CHARGE = 2,
  CONFIG_GRID_2_VOLTAGE = 3,
  CONFIG_GRID_2_TEMPERATURE = 4,
  CONFIG_GRID_2_TRIP = 5,
  CONFIG_GRID_3_SPEED = 0,
  CONFIG_GRID_3_PWM = 1,
  CONFIG_GRID_3_CHARGE = 2,
  CONFIG_GRID_3_VOLTAGE = 3,
  CONFIG_GRID_3_TEMPERATURE = 4,
  CONFIG_GRID_3_TRIP = 5,
  CONFIG_GRID_4_SPEED = 0,
  CONFIG_GRID_4_PWM = 1,
  CONFIG_GRID_4_CHARGE = 2,
  CONFIG_GRID_4_VOLTAGE = 3,
  CONFIG_GRID_4_TEMPERATURE = 4,
  CONFIG_GRID_4_TRIP = 5,
  CONFIG_WHEEL_VOLTAGE_X10_20S = 840,
  CONFIG_WHEEL_VOLTAGE_X10_24S = 1008,
  CONFIG_WHEEL_VOLTAGE_X10_30S = 1260,
  CONFIG_WHEEL_VOLTAGE_X10_32S = 1344,
  CONFIG_WHEEL_VOLTAGE_X10_36S = 1512,
  CONFIG_WHEEL_VOLTAGE_X10_40S = 1680,
  CONFIG_WHEEL_VOLTAGE_X10_42S = 1764,
  CONFIG_MAIN_LEFT_SPEED = 0,
  CONFIG_MAIN_LEFT_PWM = 1,
  CONFIG_MAIN_LEFT_TEMPERATURE = 4,
  CONFIG_MAIN_RIGHT_SPEED = 0,
  CONFIG_MAIN_RIGHT_PWM = 1,
  CONFIG_MAIN_RIGHT_TEMPERATURE = 4,
  CONFIG_PWM_ALARM_SOURCE_APP = 0,
  CONFIG_PWM_ALARM_SOURCE_FIXED = 1,
};

// Значения вне допустимых заменяются значениями по умолчанию
static inline void configSanitize(fw_config_t& c)
{
  if(c.screen_charge > 1) c.screen_charge = 1;
  if(c.screen_voltage > 1) c.screen_voltage = 1;
  if(c.screen_grid > 1) c.screen_grid = 1;
  switch(c.grid_1) { case 0: case 1: case 2: case 3: case 4: case 5: break; default: c.grid_1 = 2; }
  switch(c.grid_2) { case 0: case 1: case 2: case 3: case 4: case 5: break; default: c.grid_2 = 3; }
  switch(c.grid_3) { case 0: case 1: case 2: case 3: case 4: case 5: break; default: c.grid_3 = 4; }
  switch(c.grid_4) { case 0: case 1: case 2: case 3: case 4: case 5: break; default: c.grid_4 = 5; }
  if(c.screen_info > 1) c.screen_info = 1;
  switch(c.wheel_voltage_x10) { case 840: case 1008: case 1260: case 1344: case 1512: case 1680: case 1764: break; default: c.wheel_voltage_x10 = 1512; }
  switch(c.main_left) { case 0: case 1: case 4: break; default: c.main_left = 0; }
  switch(c.main_right) { case 0: case 1: case 4: break; default: c.main_right = 1; }
  if(c.pwm_alarm > 1) c.pwm_alarm = 1;
  switch(c.pwm_alarm_source) { case 0: case 1: break; default: c.pwm_alarm_source = 0; }
  if(c.pwm_alarm_threshold < 30 || c.pwm_alarm_threshold > 99) c.pwm_alarm_threshold = 80;
  if(c.brightness < 5 || c.brightness > 100) c.brightness = 100;
  if(c.intro_logo > 1) c.intro_logo = 1;
  if(c.intro_clawd > 1) c.intro_clawd = 1;
  if(c.intro_wave > 1) c.intro_wave = 1;
  if(c.intro_break > 1) c.intro_break = 1;
  if(c.intro_explosion > 1) c.intro_explosion = 1;
  if(c.poweroff_animation > 1) c.poweroff_animation = 1;
  if(c.power_on_ms < 100 || c.power_on_ms > 3000) c.power_on_ms = 500;
  if(c.power_off_ms < 500 || c.power_off_ms > 5000) c.power_off_ms = 1000;
  if(c.ble_update > 1) c.ble_update = 1;
}

// Описание настроек для веб-конфигуратора: config.json с адресами полей
#define FW_CONFIG_SCHEMA \
  "{\"version\":2,\"groups\":[{\"title\":\"Экраны\",\"help\":\"Страницы, которые переключаются коротким нажати" \
  "ем кнопки\",\"require_one\":true,\"fields\":[{\"key\":\"screen_charge\",\"type\":\"bool\",\"default\":true,\"lab" \
  "el\":\"Основной: два крупных значения и заряд колеса\",\"offset\":0,\"size\":1},{\"key\":\"screen_voltage\"" \
  ",\"type\":\"bool\",\"default\":true,\"label\":\"Основной: два крупных значения и напряжение колеса\",\"offs" \
  "et\":1,\"size\":1},{\"key\":\"screen_grid\",\"type\":\"bool\",\"default\":true,\"label\":\"Сетка из четырёх знач" \
  "ений\",\"offset\":2,\"size\":1},{\"key\":\"grid_1\",\"type\":\"u8\",\"default\":2,\"label\":\"Слева сверху\",\"optio" \
  "ns\":[{\"value\":0,\"id\":\"SPEED\",\"label\":\"Скорость, км/ч\"},{\"value\":1,\"id\":\"PWM\",\"label\":\"ШИМ, %\"},{" \
  "\"value\":2,\"id\":\"CHARGE\",\"label\":\"Заряд колеса, %\"},{\"value\":3,\"id\":\"VOLTAGE\",\"label\":\"Напряжение" \
  " колеса, В\"},{\"value\":4,\"id\":\"TEMPERATURE\",\"label\":\"Температура, °C\"},{\"value\":5,\"id\":\"TRIP\",\"la" \
  "bel\":\"Пробег, км\"}],\"depends\":{\"screen_grid\":true},\"subitem\":true,\"offset\":3,\"size\":1},{\"key\":\"g" \
  "rid_2\",\"type\":\"u8\",\"default\":3,\"label\":\"Справа сверху\",\"options\":[{\"value\":0,\"id\":\"SPEED\",\"label" \
  "\":\"Скорость, км/ч\"},{\"value\":1,\"id\":\"PWM\",\"label\":\"ШИМ, %\"},{\"value\":2,\"id\":\"CHARGE\",\"label\":\"За" \
  "ряд колеса, %\"},{\"value\":3,\"id\":\"VOLTAGE\",\"label\":\"Напряжение колеса, В\"},{\"value\":4,\"id\":\"TEMPE" \
  "RATURE\",\"label\":\"Температура, °C\"},{\"value\":5,\"id\":\"TRIP\",\"label\":\"Пробег, км\"}],\"depends\":{\"scr" \
  "een_grid\":true},\"subitem\":true,\"offset\":4,\"size\":1},{\"key\":\"grid_3\",\"type\":\"u8\",\"default\":4,\"lab" \
  "el\":\"Слева снизу\",\"options\":[{\"value\":0,\"id\":\"SPEED\",\"label\":\"Скорость, км/ч\"},{\"value\":1,\"id\":\"" \
  "PWM\",\"label\":\"ШИМ, %\"},{\"value\":2,\"id\":\"CHARGE\",\"label\":\"Заряд колеса, %\"},{\"value\":3,\"id\":\"VOLT" \
  "AGE\",\"label\":\"Напряжение колеса, В\"},{\"value\":4,\"id\":\"TEMPERATURE\",\"label\":\"Температура, °C\"},{\"" \
  "value\":5,\"id\":\"TRIP\",\"label\":\"Пробег, км\"}],\"depends\":{\"screen_grid\":true},\"subitem\":true,\"offse" \
  "t\":5,\"size\":1},{\"key\":\"grid_4\",\"type\":\"u8\",\"default\":5,\"label\":\"Справа снизу\",\"options\":[{\"value" \
  "\":0,\"id\":\"SPEED\",\"label\":\"Скорость, км/ч\"},{\"value\":1,\"id\":\"PWM\",\"label\":\"ШИМ, %\"},{\"value\":2,\"i" \
  "d\":\"CHARGE\",\"label\":\"Заряд колеса, %\"},{\"value\":3,\"id\":\"VOLTAGE\",\"label\":\"Напряжение колеса, В\"}" \
  ",{\"value\":4,\"id\":\"TEMPERATURE\",\"label\":\"Температура, °C\"},{\"value\":5,\"id\":\"TRIP\",\"label\":\"Пробег" \
  ", км\"}],\"depends\":{\"screen_grid\":true},\"subitem\":true,\"offset\":6,\"size\":1},{\"key\":\"screen_info\"," \
  "\"type\":\"bool\",\"default\":true,\"label\":\"Связь и версии: задержка и частота данных, версии прошивки" \
  " и загрузчика\",\"offset\":7,\"size\":1},{\"key\":\"wheel_voltage_x10\",\"type\":\"u16\",\"default\":1512,\"labe" \
  "l\":\"Батарея колеса\",\"help\":\"По максимальному напряжению рассчитывается место под цифры напряжени" \
  "я на экранах\",\"options\":[{\"value\":840,\"id\":\"20S\",\"label\":\"84 В (20S)\"},{\"value\":1008,\"id\":\"24S\"," \
  "\"label\":\"100,8 В (24S)\"},{\"value\":1260,\"id\":\"30S\",\"label\":\"126 В (30S)\"},{\"value\":1344,\"id\":\"32S" \
  "\",\"label\":\"134,4 В (32S)\"},{\"value\":1512,\"id\":\"36S\",\"label\":\"151,2 В (36S)\"},{\"value\":1680,\"id\":" \
  "\"40S\",\"label\":\"168 В (40S)\"},{\"value\":1764,\"id\":\"42S\",\"label\":\"176,4 В (42S)\"}],\"offset\":8,\"size" \
  "\":2},{\"key\":\"main_left\",\"type\":\"u8\",\"default\":0,\"section\":\"Основной экран\",\"label\":\"Крупное знач" \
  "ение слева\",\"options\":[{\"value\":0,\"id\":\"SPEED\",\"label\":\"Скорость, км/ч\"},{\"value\":1,\"id\":\"PWM\",\"" \
  "label\":\"ШИМ, %\"},{\"value\":4,\"id\":\"TEMPERATURE\",\"label\":\"Температура, °C\"}],\"offset\":10,\"size\":1}" \
  ",{\"key\":\"main_right\",\"type\":\"u8\",\"default\":1,\"label\":\"Крупное значение справа\",\"help\":\"Число, ко" \
  "торое не помещается крупно, выводится шрифтом поменьше\",\"options\":[{\"value\":0,\"id\":\"SPEED\",\"labe" \
  "l\":\"Скорость, км/ч\"},{\"value\":1,\"id\":\"PWM\",\"label\":\"ШИМ, %\"},{\"value\":4,\"id\":\"TEMPERATURE\",\"labe" \
  "l\":\"Температура, °C\"}],\"offset\":11,\"size\":1}]},{\"title\":\"Предупреждение о ШИМ\",\"help\":\"Мигание э" \
  "крана, когда ШИМ подходит к пределу\",\"fields\":[{\"key\":\"pwm_alarm\",\"type\":\"bool\",\"default\":true,\"" \
  "label\":\"Мигать экраном при превышении порога ШИМ\",\"help\":\"Чем выше ШИМ, тем чаще мигание\",\"offse" \
  "t\":12,\"size\":1},{\"key\":\"pwm_alarm_source\",\"type\":\"u8\",\"default\":0,\"label\":\"Порог\",\"options\":[{\"v" \
  "alue\":0,\"id\":\"APP\",\"label\":\"Из настроек приложения LoEUC\"},{\"value\":1,\"id\":\"FIXED\",\"label\":\"Свой" \
  "\"}],\"depends\":{\"pwm_alarm\":true},\"offset\":13,\"size\":1},{\"key\":\"pwm_alarm_threshold\",\"type\":\"u8\"," \
  "\"default\":80,\"min\":30,\"max\":99,\"unit\":\"%\",\"label\":\"Свой порог ШИМ\",\"depends\":{\"pwm_alarm\":true,\"" \
  "pwm_alarm_source\":1},\"offset\":14,\"size\":1}]},{\"title\":\"Дисплей\",\"help\":\"Яркость экрана\",\"fields\"" \
  ":[{\"key\":\"brightness\",\"type\":\"u8\",\"default\":100,\"min\":5,\"max\":100,\"step\":5,\"unit\":\"%\",\"widget\":\"" \
  "range\",\"label\":\"Яркость\",\"offset\":15,\"size\":1}]},{\"title\":\"Анимации\",\"help\":\"Заставка при включе" \
  "нии и анимация выключения\",\"fields\":[{\"key\":\"intro_logo\",\"type\":\"bool\",\"default\":true,\"section\":" \
  "\"При включении\",\"label\":\"Логотип при включении\",\"help\":\"После логотипа играет случайная из отмеч" \
  "енных заставок. Без логотипа данные появляются сразу\",\"preview\":\"logo\",\"offset\":16,\"size\":1},{\"k" \
  "ey\":\"intro_clawd\",\"type\":\"bool\",\"default\":false,\"label\":\"Заставки Clawd\",\"help\":\"Играют заставки" \
  " Clawd\",\"widget\":\"clawd\",\"depends\":{\"intro_logo\":true},\"offset\":17,\"size\":1},{\"key\":\"intro_wave\"" \
  ",\"type\":\"bool\",\"default\":true,\"label\":\"Волна\",\"help\":\"Волна с пузырями проходит слева направо и " \
  "открывает данные\",\"preview\":\"wave\",\"depends\":{\"intro_logo\":true,\"intro_clawd\":false},\"subitem\":t" \
  "rue,\"offset\":18,\"size\":1},{\"key\":\"intro_break\",\"type\":\"bool\",\"default\":true,\"label\":\"Разрушение\"" \
  ",\"help\":\"Логотип трескается и рассыпается на куски\",\"preview\":\"break\",\"depends\":{\"intro_logo\":tr" \
  "ue,\"intro_clawd\":false},\"subitem\":true,\"offset\":19,\"size\":1},{\"key\":\"intro_explosion\",\"type\":\"bo" \
  "ol\",\"default\":true,\"label\":\"Взрыв\",\"help\":\"Логотип разлетается облаком частиц, как TNT в Minecra" \
  "ft\",\"preview\":\"explosion\",\"depends\":{\"intro_logo\":true,\"intro_clawd\":false},\"subitem\":true,\"offs" \
  "et\":20,\"size\":1},{\"key\":\"poweroff_animation\",\"type\":\"bool\",\"default\":true,\"section\":\"При выключе" \
  "нии\",\"label\":\"Анимация выключения\",\"help\":\"Экран сжимается в линию, затем в точку и гаснет, как " \
  "у старого телевизора\",\"preview\":\"poweroff\",\"offset\":21,\"size\":1}]},{\"title\":\"Кнопка\",\"help\":\"Ско" \
  "лько держать кнопку, чтобы включить и выключить дисплей\",\"fields\":[{\"key\":\"power_on_ms\",\"type\":\"" \
  "u16\",\"default\":500,\"min\":100,\"max\":3000,\"step\":100,\"unit\":\"мс\",\"label\":\"Удержание для включения\"" \
  ",\"offset\":22,\"size\":2},{\"key\":\"power_off_ms\",\"type\":\"u16\",\"default\":1000,\"min\":500,\"max\":5000,\"s" \
  "tep\":100,\"unit\":\"мс\",\"label\":\"Удержание для выключения\",\"help\":\"Более короткое нажатие переключа" \
  "ет экран\",\"offset\":24,\"size\":2}]},{\"title\":\"Обновление по Bluetooth\",\"help\":\"Прошивка с телефона" \
  " без кабеля\",\"fields\":[{\"key\":\"ble_update\",\"type\":\"bool\",\"default\":true,\"label\":\"Разрешить обнов" \
  "ление прошивки по Bluetooth\",\"help\":\"Режим обновления включается, только если при включении держ" \
  "ать кнопку ещё 3 секунды после того, как загорится экран, и заряд не ниже 30%. В обычной работе " \
  "прошить дисплей по Bluetooth нельзя\",\"offset\":26,\"size\":1}]}]}"
