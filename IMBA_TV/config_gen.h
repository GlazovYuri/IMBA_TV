#pragma once

// Сгенерировано tools/gen_config.py из config.json, не править руками.
// После изменения config.json запустите: python3 tools/gen_config.py

#include <stdint.h>

// Настройки прошивки. Значения записывает веб-конфигуратор перед прошивкой.
struct __attribute__((packed)) fw_config_t
{
  uint8_t screen1_type;  // Экран 1
  uint8_t screen1_v1;  // Экран 1, поле 1
  uint8_t screen1_v2;  // Экран 1, поле 2
  uint8_t screen1_v3;  // Экран 1, поле 3
  uint8_t screen1_v4;  // Экран 1, поле 4
  uint8_t screen2_type;  // Экран 2
  uint8_t screen2_v1;  // Экран 2, поле 1
  uint8_t screen2_v2;  // Экран 2, поле 2
  uint8_t screen2_v3;  // Экран 2, поле 3
  uint8_t screen2_v4;  // Экран 2, поле 4
  uint8_t screen3_type;  // Экран 3
  uint8_t screen3_v1;  // Экран 3, поле 1
  uint8_t screen3_v2;  // Экран 3, поле 2
  uint8_t screen3_v3;  // Экран 3, поле 3
  uint8_t screen3_v4;  // Экран 3, поле 4
  uint8_t screen4_type;  // Экран 4
  uint8_t screen4_v1;  // Экран 4, поле 1
  uint8_t screen4_v2;  // Экран 4, поле 2
  uint8_t screen4_v3;  // Экран 4, поле 3
  uint8_t screen4_v4;  // Экран 4, поле 4
  uint8_t screen5_type;  // Экран 5
  uint8_t screen5_v1;  // Экран 5, поле 1
  uint8_t screen5_v2;  // Экран 5, поле 2
  uint8_t screen5_v3;  // Экран 5, поле 3
  uint8_t screen5_v4;  // Экран 5, поле 4
  uint16_t wheel_voltage_x10;  // Батарея колеса
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

static_assert(sizeof(fw_config_t) == 42, "config.json и fw_config_t разошлись");

#define FW_CONFIG_DEFAULTS { 1, 2, 0, 1, 5, 1, 3, 0, 1, 5, 2, 2, 3, 4, 5, 3, 2, 3, 4, 5, 0, 2, 3, 4, 5, 1512, 1, 0, 80, 100, 1, 0, 1, 1, 1, 1, 500, 1000, 1 }

enum
{
  CONFIG_SCREEN1_TYPE_NONE = 0,
  CONFIG_SCREEN1_TYPE_MAIN = 1,
  CONFIG_SCREEN1_TYPE_GRID = 2,
  CONFIG_SCREEN1_TYPE_INFO = 3,
  CONFIG_SCREEN1_V1_SPEED = 0,
  CONFIG_SCREEN1_V1_PWM = 1,
  CONFIG_SCREEN1_V1_CHARGE = 2,
  CONFIG_SCREEN1_V1_VOLTAGE = 3,
  CONFIG_SCREEN1_V1_TEMPERATURE = 4,
  CONFIG_SCREEN1_V1_TRIP = 5,
  CONFIG_SCREEN1_V2_SPEED = 0,
  CONFIG_SCREEN1_V2_PWM = 1,
  CONFIG_SCREEN1_V2_CHARGE = 2,
  CONFIG_SCREEN1_V2_VOLTAGE = 3,
  CONFIG_SCREEN1_V2_TEMPERATURE = 4,
  CONFIG_SCREEN1_V2_TRIP = 5,
  CONFIG_SCREEN1_V3_SPEED = 0,
  CONFIG_SCREEN1_V3_PWM = 1,
  CONFIG_SCREEN1_V3_CHARGE = 2,
  CONFIG_SCREEN1_V3_VOLTAGE = 3,
  CONFIG_SCREEN1_V3_TEMPERATURE = 4,
  CONFIG_SCREEN1_V3_TRIP = 5,
  CONFIG_SCREEN1_V4_SPEED = 0,
  CONFIG_SCREEN1_V4_PWM = 1,
  CONFIG_SCREEN1_V4_CHARGE = 2,
  CONFIG_SCREEN1_V4_VOLTAGE = 3,
  CONFIG_SCREEN1_V4_TEMPERATURE = 4,
  CONFIG_SCREEN1_V4_TRIP = 5,
  CONFIG_SCREEN2_TYPE_NONE = 0,
  CONFIG_SCREEN2_TYPE_MAIN = 1,
  CONFIG_SCREEN2_TYPE_GRID = 2,
  CONFIG_SCREEN2_TYPE_INFO = 3,
  CONFIG_SCREEN2_V1_SPEED = 0,
  CONFIG_SCREEN2_V1_PWM = 1,
  CONFIG_SCREEN2_V1_CHARGE = 2,
  CONFIG_SCREEN2_V1_VOLTAGE = 3,
  CONFIG_SCREEN2_V1_TEMPERATURE = 4,
  CONFIG_SCREEN2_V1_TRIP = 5,
  CONFIG_SCREEN2_V2_SPEED = 0,
  CONFIG_SCREEN2_V2_PWM = 1,
  CONFIG_SCREEN2_V2_CHARGE = 2,
  CONFIG_SCREEN2_V2_VOLTAGE = 3,
  CONFIG_SCREEN2_V2_TEMPERATURE = 4,
  CONFIG_SCREEN2_V2_TRIP = 5,
  CONFIG_SCREEN2_V3_SPEED = 0,
  CONFIG_SCREEN2_V3_PWM = 1,
  CONFIG_SCREEN2_V3_CHARGE = 2,
  CONFIG_SCREEN2_V3_VOLTAGE = 3,
  CONFIG_SCREEN2_V3_TEMPERATURE = 4,
  CONFIG_SCREEN2_V3_TRIP = 5,
  CONFIG_SCREEN2_V4_SPEED = 0,
  CONFIG_SCREEN2_V4_PWM = 1,
  CONFIG_SCREEN2_V4_CHARGE = 2,
  CONFIG_SCREEN2_V4_VOLTAGE = 3,
  CONFIG_SCREEN2_V4_TEMPERATURE = 4,
  CONFIG_SCREEN2_V4_TRIP = 5,
  CONFIG_SCREEN3_TYPE_NONE = 0,
  CONFIG_SCREEN3_TYPE_MAIN = 1,
  CONFIG_SCREEN3_TYPE_GRID = 2,
  CONFIG_SCREEN3_TYPE_INFO = 3,
  CONFIG_SCREEN3_V1_SPEED = 0,
  CONFIG_SCREEN3_V1_PWM = 1,
  CONFIG_SCREEN3_V1_CHARGE = 2,
  CONFIG_SCREEN3_V1_VOLTAGE = 3,
  CONFIG_SCREEN3_V1_TEMPERATURE = 4,
  CONFIG_SCREEN3_V1_TRIP = 5,
  CONFIG_SCREEN3_V2_SPEED = 0,
  CONFIG_SCREEN3_V2_PWM = 1,
  CONFIG_SCREEN3_V2_CHARGE = 2,
  CONFIG_SCREEN3_V2_VOLTAGE = 3,
  CONFIG_SCREEN3_V2_TEMPERATURE = 4,
  CONFIG_SCREEN3_V2_TRIP = 5,
  CONFIG_SCREEN3_V3_SPEED = 0,
  CONFIG_SCREEN3_V3_PWM = 1,
  CONFIG_SCREEN3_V3_CHARGE = 2,
  CONFIG_SCREEN3_V3_VOLTAGE = 3,
  CONFIG_SCREEN3_V3_TEMPERATURE = 4,
  CONFIG_SCREEN3_V3_TRIP = 5,
  CONFIG_SCREEN3_V4_SPEED = 0,
  CONFIG_SCREEN3_V4_PWM = 1,
  CONFIG_SCREEN3_V4_CHARGE = 2,
  CONFIG_SCREEN3_V4_VOLTAGE = 3,
  CONFIG_SCREEN3_V4_TEMPERATURE = 4,
  CONFIG_SCREEN3_V4_TRIP = 5,
  CONFIG_SCREEN4_TYPE_NONE = 0,
  CONFIG_SCREEN4_TYPE_MAIN = 1,
  CONFIG_SCREEN4_TYPE_GRID = 2,
  CONFIG_SCREEN4_TYPE_INFO = 3,
  CONFIG_SCREEN4_V1_SPEED = 0,
  CONFIG_SCREEN4_V1_PWM = 1,
  CONFIG_SCREEN4_V1_CHARGE = 2,
  CONFIG_SCREEN4_V1_VOLTAGE = 3,
  CONFIG_SCREEN4_V1_TEMPERATURE = 4,
  CONFIG_SCREEN4_V1_TRIP = 5,
  CONFIG_SCREEN4_V2_SPEED = 0,
  CONFIG_SCREEN4_V2_PWM = 1,
  CONFIG_SCREEN4_V2_CHARGE = 2,
  CONFIG_SCREEN4_V2_VOLTAGE = 3,
  CONFIG_SCREEN4_V2_TEMPERATURE = 4,
  CONFIG_SCREEN4_V2_TRIP = 5,
  CONFIG_SCREEN4_V3_SPEED = 0,
  CONFIG_SCREEN4_V3_PWM = 1,
  CONFIG_SCREEN4_V3_CHARGE = 2,
  CONFIG_SCREEN4_V3_VOLTAGE = 3,
  CONFIG_SCREEN4_V3_TEMPERATURE = 4,
  CONFIG_SCREEN4_V3_TRIP = 5,
  CONFIG_SCREEN4_V4_SPEED = 0,
  CONFIG_SCREEN4_V4_PWM = 1,
  CONFIG_SCREEN4_V4_CHARGE = 2,
  CONFIG_SCREEN4_V4_VOLTAGE = 3,
  CONFIG_SCREEN4_V4_TEMPERATURE = 4,
  CONFIG_SCREEN4_V4_TRIP = 5,
  CONFIG_SCREEN5_TYPE_NONE = 0,
  CONFIG_SCREEN5_TYPE_MAIN = 1,
  CONFIG_SCREEN5_TYPE_GRID = 2,
  CONFIG_SCREEN5_TYPE_INFO = 3,
  CONFIG_SCREEN5_V1_SPEED = 0,
  CONFIG_SCREEN5_V1_PWM = 1,
  CONFIG_SCREEN5_V1_CHARGE = 2,
  CONFIG_SCREEN5_V1_VOLTAGE = 3,
  CONFIG_SCREEN5_V1_TEMPERATURE = 4,
  CONFIG_SCREEN5_V1_TRIP = 5,
  CONFIG_SCREEN5_V2_SPEED = 0,
  CONFIG_SCREEN5_V2_PWM = 1,
  CONFIG_SCREEN5_V2_CHARGE = 2,
  CONFIG_SCREEN5_V2_VOLTAGE = 3,
  CONFIG_SCREEN5_V2_TEMPERATURE = 4,
  CONFIG_SCREEN5_V2_TRIP = 5,
  CONFIG_SCREEN5_V3_SPEED = 0,
  CONFIG_SCREEN5_V3_PWM = 1,
  CONFIG_SCREEN5_V3_CHARGE = 2,
  CONFIG_SCREEN5_V3_VOLTAGE = 3,
  CONFIG_SCREEN5_V3_TEMPERATURE = 4,
  CONFIG_SCREEN5_V3_TRIP = 5,
  CONFIG_SCREEN5_V4_SPEED = 0,
  CONFIG_SCREEN5_V4_PWM = 1,
  CONFIG_SCREEN5_V4_CHARGE = 2,
  CONFIG_SCREEN5_V4_VOLTAGE = 3,
  CONFIG_SCREEN5_V4_TEMPERATURE = 4,
  CONFIG_SCREEN5_V4_TRIP = 5,
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
  switch(c.screen1_type) { case 0: case 1: case 2: case 3: break; default: c.screen1_type = 1; }
  switch(c.screen1_v1) { case 0: case 1: case 2: case 3: case 4: case 5: break; default: c.screen1_v1 = 2; }
  switch(c.screen1_v2) { case 0: case 1: case 2: case 3: case 4: case 5: break; default: c.screen1_v2 = 0; }
  switch(c.screen1_v3) { case 0: case 1: case 2: case 3: case 4: case 5: break; default: c.screen1_v3 = 1; }
  switch(c.screen1_v4) { case 0: case 1: case 2: case 3: case 4: case 5: break; default: c.screen1_v4 = 5; }
  switch(c.screen2_type) { case 0: case 1: case 2: case 3: break; default: c.screen2_type = 1; }
  switch(c.screen2_v1) { case 0: case 1: case 2: case 3: case 4: case 5: break; default: c.screen2_v1 = 3; }
  switch(c.screen2_v2) { case 0: case 1: case 2: case 3: case 4: case 5: break; default: c.screen2_v2 = 0; }
  switch(c.screen2_v3) { case 0: case 1: case 2: case 3: case 4: case 5: break; default: c.screen2_v3 = 1; }
  switch(c.screen2_v4) { case 0: case 1: case 2: case 3: case 4: case 5: break; default: c.screen2_v4 = 5; }
  switch(c.screen3_type) { case 0: case 1: case 2: case 3: break; default: c.screen3_type = 2; }
  switch(c.screen3_v1) { case 0: case 1: case 2: case 3: case 4: case 5: break; default: c.screen3_v1 = 2; }
  switch(c.screen3_v2) { case 0: case 1: case 2: case 3: case 4: case 5: break; default: c.screen3_v2 = 3; }
  switch(c.screen3_v3) { case 0: case 1: case 2: case 3: case 4: case 5: break; default: c.screen3_v3 = 4; }
  switch(c.screen3_v4) { case 0: case 1: case 2: case 3: case 4: case 5: break; default: c.screen3_v4 = 5; }
  switch(c.screen4_type) { case 0: case 1: case 2: case 3: break; default: c.screen4_type = 3; }
  switch(c.screen4_v1) { case 0: case 1: case 2: case 3: case 4: case 5: break; default: c.screen4_v1 = 2; }
  switch(c.screen4_v2) { case 0: case 1: case 2: case 3: case 4: case 5: break; default: c.screen4_v2 = 3; }
  switch(c.screen4_v3) { case 0: case 1: case 2: case 3: case 4: case 5: break; default: c.screen4_v3 = 4; }
  switch(c.screen4_v4) { case 0: case 1: case 2: case 3: case 4: case 5: break; default: c.screen4_v4 = 5; }
  switch(c.screen5_type) { case 0: case 1: case 2: case 3: break; default: c.screen5_type = 0; }
  switch(c.screen5_v1) { case 0: case 1: case 2: case 3: case 4: case 5: break; default: c.screen5_v1 = 2; }
  switch(c.screen5_v2) { case 0: case 1: case 2: case 3: case 4: case 5: break; default: c.screen5_v2 = 3; }
  switch(c.screen5_v3) { case 0: case 1: case 2: case 3: case 4: case 5: break; default: c.screen5_v3 = 4; }
  switch(c.screen5_v4) { case 0: case 1: case 2: case 3: case 4: case 5: break; default: c.screen5_v4 = 5; }
  switch(c.wheel_voltage_x10) { case 840: case 1008: case 1260: case 1344: case 1512: case 1680: case 1764: break; default: c.wheel_voltage_x10 = 1512; }
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
  "ем кнопки: до пяти, в любом порядке\",\"widget\":\"screens\",\"fields\":[{\"key\":\"screen1_type\",\"type\":\"" \
  "u8\",\"default\":1,\"label\":\"Экран 1\",\"help\":\"Экраны идут по порядку, пустые пропускаются. Поля: у э" \
  "крана с двумя значениями 1 - верхняя строка (заряд или напряжение), 2 и 3 - крупные значения сле" \
  "ва и справа; у сетки 1-4 - клетки по строкам\",\"options\":[{\"value\":0,\"id\":\"NONE\",\"label\":\"Нет\"},{" \
  "\"value\":1,\"id\":\"MAIN\",\"label\":\"Два значения\"},{\"value\":2,\"id\":\"GRID\",\"label\":\"Четыре значения\"}," \
  "{\"value\":3,\"id\":\"INFO\",\"label\":\"Связь и версии\"}],\"offset\":0,\"size\":1},{\"key\":\"screen1_v1\",\"type" \
  "\":\"u8\",\"default\":2,\"label\":\"Экран 1, поле 1\",\"options\":[{\"value\":0,\"id\":\"SPEED\",\"label\":\"Скорост" \
  "ь, км/ч\"},{\"value\":1,\"id\":\"PWM\",\"label\":\"ШИМ, %\"},{\"value\":2,\"id\":\"CHARGE\",\"label\":\"Заряд колеса" \
  ", %\"},{\"value\":3,\"id\":\"VOLTAGE\",\"label\":\"Напряжение колеса, В\"},{\"value\":4,\"id\":\"TEMPERATURE\",\"l" \
  "abel\":\"Температура, °C\"},{\"value\":5,\"id\":\"TRIP\",\"label\":\"Пробег, км\"}],\"offset\":1,\"size\":1},{\"ke" \
  "y\":\"screen1_v2\",\"type\":\"u8\",\"default\":0,\"label\":\"Экран 1, поле 2\",\"options\":[{\"value\":0,\"id\":\"SP" \
  "EED\",\"label\":\"Скорость, км/ч\"},{\"value\":1,\"id\":\"PWM\",\"label\":\"ШИМ, %\"},{\"value\":2,\"id\":\"CHARGE\"," \
  "\"label\":\"Заряд колеса, %\"},{\"value\":3,\"id\":\"VOLTAGE\",\"label\":\"Напряжение колеса, В\"},{\"value\":4," \
  "\"id\":\"TEMPERATURE\",\"label\":\"Температура, °C\"},{\"value\":5,\"id\":\"TRIP\",\"label\":\"Пробег, км\"}],\"off" \
  "set\":2,\"size\":1},{\"key\":\"screen1_v3\",\"type\":\"u8\",\"default\":1,\"label\":\"Экран 1, поле 3\",\"options\"" \
  ":[{\"value\":0,\"id\":\"SPEED\",\"label\":\"Скорость, км/ч\"},{\"value\":1,\"id\":\"PWM\",\"label\":\"ШИМ, %\"},{\"va" \
  "lue\":2,\"id\":\"CHARGE\",\"label\":\"Заряд колеса, %\"},{\"value\":3,\"id\":\"VOLTAGE\",\"label\":\"Напряжение ко" \
  "леса, В\"},{\"value\":4,\"id\":\"TEMPERATURE\",\"label\":\"Температура, °C\"},{\"value\":5,\"id\":\"TRIP\",\"label" \
  "\":\"Пробег, км\"}],\"offset\":3,\"size\":1},{\"key\":\"screen1_v4\",\"type\":\"u8\",\"default\":5,\"label\":\"Экран" \
  " 1, поле 4\",\"options\":[{\"value\":0,\"id\":\"SPEED\",\"label\":\"Скорость, км/ч\"},{\"value\":1,\"id\":\"PWM\",\"" \
  "label\":\"ШИМ, %\"},{\"value\":2,\"id\":\"CHARGE\",\"label\":\"Заряд колеса, %\"},{\"value\":3,\"id\":\"VOLTAGE\",\"" \
  "label\":\"Напряжение колеса, В\"},{\"value\":4,\"id\":\"TEMPERATURE\",\"label\":\"Температура, °C\"},{\"value\"" \
  ":5,\"id\":\"TRIP\",\"label\":\"Пробег, км\"}],\"offset\":4,\"size\":1},{\"key\":\"screen2_type\",\"type\":\"u8\",\"de" \
  "fault\":1,\"label\":\"Экран 2\",\"options\":[{\"value\":0,\"id\":\"NONE\",\"label\":\"Нет\"},{\"value\":1,\"id\":\"MAI" \
  "N\",\"label\":\"Два значения\"},{\"value\":2,\"id\":\"GRID\",\"label\":\"Четыре значения\"},{\"value\":3,\"id\":\"IN" \
  "FO\",\"label\":\"Связь и версии\"}],\"offset\":5,\"size\":1},{\"key\":\"screen2_v1\",\"type\":\"u8\",\"default\":3," \
  "\"label\":\"Экран 2, поле 1\",\"options\":[{\"value\":0,\"id\":\"SPEED\",\"label\":\"Скорость, км/ч\"},{\"value\":" \
  "1,\"id\":\"PWM\",\"label\":\"ШИМ, %\"},{\"value\":2,\"id\":\"CHARGE\",\"label\":\"Заряд колеса, %\"},{\"value\":3,\"i" \
  "d\":\"VOLTAGE\",\"label\":\"Напряжение колеса, В\"},{\"value\":4,\"id\":\"TEMPERATURE\",\"label\":\"Температура," \
  " °C\"},{\"value\":5,\"id\":\"TRIP\",\"label\":\"Пробег, км\"}],\"offset\":6,\"size\":1},{\"key\":\"screen2_v2\",\"ty" \
  "pe\":\"u8\",\"default\":0,\"label\":\"Экран 2, поле 2\",\"options\":[{\"value\":0,\"id\":\"SPEED\",\"label\":\"Скоро" \
  "сть, км/ч\"},{\"value\":1,\"id\":\"PWM\",\"label\":\"ШИМ, %\"},{\"value\":2,\"id\":\"CHARGE\",\"label\":\"Заряд коле" \
  "са, %\"},{\"value\":3,\"id\":\"VOLTAGE\",\"label\":\"Напряжение колеса, В\"},{\"value\":4,\"id\":\"TEMPERATURE\"," \
  "\"label\":\"Температура, °C\"},{\"value\":5,\"id\":\"TRIP\",\"label\":\"Пробег, км\"}],\"offset\":7,\"size\":1},{\"" \
  "key\":\"screen2_v3\",\"type\":\"u8\",\"default\":1,\"label\":\"Экран 2, поле 3\",\"options\":[{\"value\":0,\"id\":\"" \
  "SPEED\",\"label\":\"Скорость, км/ч\"},{\"value\":1,\"id\":\"PWM\",\"label\":\"ШИМ, %\"},{\"value\":2,\"id\":\"CHARGE" \
  "\",\"label\":\"Заряд колеса, %\"},{\"value\":3,\"id\":\"VOLTAGE\",\"label\":\"Напряжение колеса, В\"},{\"value\":" \
  "4,\"id\":\"TEMPERATURE\",\"label\":\"Температура, °C\"},{\"value\":5,\"id\":\"TRIP\",\"label\":\"Пробег, км\"}],\"o" \
  "ffset\":8,\"size\":1},{\"key\":\"screen2_v4\",\"type\":\"u8\",\"default\":5,\"label\":\"Экран 2, поле 4\",\"option" \
  "s\":[{\"value\":0,\"id\":\"SPEED\",\"label\":\"Скорость, км/ч\"},{\"value\":1,\"id\":\"PWM\",\"label\":\"ШИМ, %\"},{\"" \
  "value\":2,\"id\":\"CHARGE\",\"label\":\"Заряд колеса, %\"},{\"value\":3,\"id\":\"VOLTAGE\",\"label\":\"Напряжение " \
  "колеса, В\"},{\"value\":4,\"id\":\"TEMPERATURE\",\"label\":\"Температура, °C\"},{\"value\":5,\"id\":\"TRIP\",\"lab" \
  "el\":\"Пробег, км\"}],\"offset\":9,\"size\":1},{\"key\":\"screen3_type\",\"type\":\"u8\",\"default\":2,\"label\":\"Э" \
  "кран 3\",\"options\":[{\"value\":0,\"id\":\"NONE\",\"label\":\"Нет\"},{\"value\":1,\"id\":\"MAIN\",\"label\":\"Два зна" \
  "чения\"},{\"value\":2,\"id\":\"GRID\",\"label\":\"Четыре значения\"},{\"value\":3,\"id\":\"INFO\",\"label\":\"Связь " \
  "и версии\"}],\"offset\":10,\"size\":1},{\"key\":\"screen3_v1\",\"type\":\"u8\",\"default\":2,\"label\":\"Экран 3, " \
  "поле 1\",\"options\":[{\"value\":0,\"id\":\"SPEED\",\"label\":\"Скорость, км/ч\"},{\"value\":1,\"id\":\"PWM\",\"labe" \
  "l\":\"ШИМ, %\"},{\"value\":2,\"id\":\"CHARGE\",\"label\":\"Заряд колеса, %\"},{\"value\":3,\"id\":\"VOLTAGE\",\"labe" \
  "l\":\"Напряжение колеса, В\"},{\"value\":4,\"id\":\"TEMPERATURE\",\"label\":\"Температура, °C\"},{\"value\":5,\"" \
  "id\":\"TRIP\",\"label\":\"Пробег, км\"}],\"offset\":11,\"size\":1},{\"key\":\"screen3_v2\",\"type\":\"u8\",\"default" \
  "\":3,\"label\":\"Экран 3, поле 2\",\"options\":[{\"value\":0,\"id\":\"SPEED\",\"label\":\"Скорость, км/ч\"},{\"val" \
  "ue\":1,\"id\":\"PWM\",\"label\":\"ШИМ, %\"},{\"value\":2,\"id\":\"CHARGE\",\"label\":\"Заряд колеса, %\"},{\"value\":" \
  "3,\"id\":\"VOLTAGE\",\"label\":\"Напряжение колеса, В\"},{\"value\":4,\"id\":\"TEMPERATURE\",\"label\":\"Температ" \
  "ура, °C\"},{\"value\":5,\"id\":\"TRIP\",\"label\":\"Пробег, км\"}],\"offset\":12,\"size\":1},{\"key\":\"screen3_v3" \
  "\",\"type\":\"u8\",\"default\":4,\"label\":\"Экран 3, поле 3\",\"options\":[{\"value\":0,\"id\":\"SPEED\",\"label\":\"" \
  "Скорость, км/ч\"},{\"value\":1,\"id\":\"PWM\",\"label\":\"ШИМ, %\"},{\"value\":2,\"id\":\"CHARGE\",\"label\":\"Заряд" \
  " колеса, %\"},{\"value\":3,\"id\":\"VOLTAGE\",\"label\":\"Напряжение колеса, В\"},{\"value\":4,\"id\":\"TEMPERAT" \
  "URE\",\"label\":\"Температура, °C\"},{\"value\":5,\"id\":\"TRIP\",\"label\":\"Пробег, км\"}],\"offset\":13,\"size\"" \
  ":1},{\"key\":\"screen3_v4\",\"type\":\"u8\",\"default\":5,\"label\":\"Экран 3, поле 4\",\"options\":[{\"value\":0," \
  "\"id\":\"SPEED\",\"label\":\"Скорость, км/ч\"},{\"value\":1,\"id\":\"PWM\",\"label\":\"ШИМ, %\"},{\"value\":2,\"id\":\"" \
  "CHARGE\",\"label\":\"Заряд колеса, %\"},{\"value\":3,\"id\":\"VOLTAGE\",\"label\":\"Напряжение колеса, В\"},{\"v" \
  "alue\":4,\"id\":\"TEMPERATURE\",\"label\":\"Температура, °C\"},{\"value\":5,\"id\":\"TRIP\",\"label\":\"Пробег, км" \
  "\"}],\"offset\":14,\"size\":1},{\"key\":\"screen4_type\",\"type\":\"u8\",\"default\":3,\"label\":\"Экран 4\",\"optio" \
  "ns\":[{\"value\":0,\"id\":\"NONE\",\"label\":\"Нет\"},{\"value\":1,\"id\":\"MAIN\",\"label\":\"Два значения\"},{\"valu" \
  "e\":2,\"id\":\"GRID\",\"label\":\"Четыре значения\"},{\"value\":3,\"id\":\"INFO\",\"label\":\"Связь и версии\"}],\"o" \
  "ffset\":15,\"size\":1},{\"key\":\"screen4_v1\",\"type\":\"u8\",\"default\":2,\"label\":\"Экран 4, поле 1\",\"optio" \
  "ns\":[{\"value\":0,\"id\":\"SPEED\",\"label\":\"Скорость, км/ч\"},{\"value\":1,\"id\":\"PWM\",\"label\":\"ШИМ, %\"},{" \
  "\"value\":2,\"id\":\"CHARGE\",\"label\":\"Заряд колеса, %\"},{\"value\":3,\"id\":\"VOLTAGE\",\"label\":\"Напряжение" \
  " колеса, В\"},{\"value\":4,\"id\":\"TEMPERATURE\",\"label\":\"Температура, °C\"},{\"value\":5,\"id\":\"TRIP\",\"la" \
  "bel\":\"Пробег, км\"}],\"offset\":16,\"size\":1},{\"key\":\"screen4_v2\",\"type\":\"u8\",\"default\":3,\"label\":\"Э" \
  "кран 4, поле 2\",\"options\":[{\"value\":0,\"id\":\"SPEED\",\"label\":\"Скорость, км/ч\"},{\"value\":1,\"id\":\"PW" \
  "M\",\"label\":\"ШИМ, %\"},{\"value\":2,\"id\":\"CHARGE\",\"label\":\"Заряд колеса, %\"},{\"value\":3,\"id\":\"VOLTAG" \
  "E\",\"label\":\"Напряжение колеса, В\"},{\"value\":4,\"id\":\"TEMPERATURE\",\"label\":\"Температура, °C\"},{\"va" \
  "lue\":5,\"id\":\"TRIP\",\"label\":\"Пробег, км\"}],\"offset\":17,\"size\":1},{\"key\":\"screen4_v3\",\"type\":\"u8\"," \
  "\"default\":4,\"label\":\"Экран 4, поле 3\",\"options\":[{\"value\":0,\"id\":\"SPEED\",\"label\":\"Скорость, км/ч" \
  "\"},{\"value\":1,\"id\":\"PWM\",\"label\":\"ШИМ, %\"},{\"value\":2,\"id\":\"CHARGE\",\"label\":\"Заряд колеса, %\"},{" \
  "\"value\":3,\"id\":\"VOLTAGE\",\"label\":\"Напряжение колеса, В\"},{\"value\":4,\"id\":\"TEMPERATURE\",\"label\":\"" \
  "Температура, °C\"},{\"value\":5,\"id\":\"TRIP\",\"label\":\"Пробег, км\"}],\"offset\":18,\"size\":1},{\"key\":\"sc" \
  "reen4_v4\",\"type\":\"u8\",\"default\":5,\"label\":\"Экран 4, поле 4\",\"options\":[{\"value\":0,\"id\":\"SPEED\",\"" \
  "label\":\"Скорость, км/ч\"},{\"value\":1,\"id\":\"PWM\",\"label\":\"ШИМ, %\"},{\"value\":2,\"id\":\"CHARGE\",\"label" \
  "\":\"Заряд колеса, %\"},{\"value\":3,\"id\":\"VOLTAGE\",\"label\":\"Напряжение колеса, В\"},{\"value\":4,\"id\":\"" \
  "TEMPERATURE\",\"label\":\"Температура, °C\"},{\"value\":5,\"id\":\"TRIP\",\"label\":\"Пробег, км\"}],\"offset\":1" \
  "9,\"size\":1},{\"key\":\"screen5_type\",\"type\":\"u8\",\"default\":0,\"label\":\"Экран 5\",\"options\":[{\"value\":" \
  "0,\"id\":\"NONE\",\"label\":\"Нет\"},{\"value\":1,\"id\":\"MAIN\",\"label\":\"Два значения\"},{\"value\":2,\"id\":\"GRI" \
  "D\",\"label\":\"Четыре значения\"},{\"value\":3,\"id\":\"INFO\",\"label\":\"Связь и версии\"}],\"offset\":20,\"siz" \
  "e\":1},{\"key\":\"screen5_v1\",\"type\":\"u8\",\"default\":2,\"label\":\"Экран 5, поле 1\",\"options\":[{\"value\":" \
  "0,\"id\":\"SPEED\",\"label\":\"Скорость, км/ч\"},{\"value\":1,\"id\":\"PWM\",\"label\":\"ШИМ, %\"},{\"value\":2,\"id\"" \
  ":\"CHARGE\",\"label\":\"Заряд колеса, %\"},{\"value\":3,\"id\":\"VOLTAGE\",\"label\":\"Напряжение колеса, В\"},{" \
  "\"value\":4,\"id\":\"TEMPERATURE\",\"label\":\"Температура, °C\"},{\"value\":5,\"id\":\"TRIP\",\"label\":\"Пробег, " \
  "км\"}],\"offset\":21,\"size\":1},{\"key\":\"screen5_v2\",\"type\":\"u8\",\"default\":3,\"label\":\"Экран 5, поле 2" \
  "\",\"options\":[{\"value\":0,\"id\":\"SPEED\",\"label\":\"Скорость, км/ч\"},{\"value\":1,\"id\":\"PWM\",\"label\":\"ШИ" \
  "М, %\"},{\"value\":2,\"id\":\"CHARGE\",\"label\":\"Заряд колеса, %\"},{\"value\":3,\"id\":\"VOLTAGE\",\"label\":\"На" \
  "пряжение колеса, В\"},{\"value\":4,\"id\":\"TEMPERATURE\",\"label\":\"Температура, °C\"},{\"value\":5,\"id\":\"T" \
  "RIP\",\"label\":\"Пробег, км\"}],\"offset\":22,\"size\":1},{\"key\":\"screen5_v3\",\"type\":\"u8\",\"default\":4,\"l" \
  "abel\":\"Экран 5, поле 3\",\"options\":[{\"value\":0,\"id\":\"SPEED\",\"label\":\"Скорость, км/ч\"},{\"value\":1," \
  "\"id\":\"PWM\",\"label\":\"ШИМ, %\"},{\"value\":2,\"id\":\"CHARGE\",\"label\":\"Заряд колеса, %\"},{\"value\":3,\"id\"" \
  ":\"VOLTAGE\",\"label\":\"Напряжение колеса, В\"},{\"value\":4,\"id\":\"TEMPERATURE\",\"label\":\"Температура, °" \
  "C\"},{\"value\":5,\"id\":\"TRIP\",\"label\":\"Пробег, км\"}],\"offset\":23,\"size\":1},{\"key\":\"screen5_v4\",\"typ" \
  "e\":\"u8\",\"default\":5,\"label\":\"Экран 5, поле 4\",\"options\":[{\"value\":0,\"id\":\"SPEED\",\"label\":\"Скорос" \
  "ть, км/ч\"},{\"value\":1,\"id\":\"PWM\",\"label\":\"ШИМ, %\"},{\"value\":2,\"id\":\"CHARGE\",\"label\":\"Заряд колес" \
  "а, %\"},{\"value\":3,\"id\":\"VOLTAGE\",\"label\":\"Напряжение колеса, В\"},{\"value\":4,\"id\":\"TEMPERATURE\",\"" \
  "label\":\"Температура, °C\"},{\"value\":5,\"id\":\"TRIP\",\"label\":\"Пробег, км\"}],\"offset\":24,\"size\":1},{\"" \
  "key\":\"wheel_voltage_x10\",\"type\":\"u16\",\"default\":1512,\"label\":\"Батарея колеса\",\"help\":\"По максима" \
  "льному напряжению рассчитывается место под цифры напряжения на экранах\",\"options\":[{\"value\":840," \
  "\"id\":\"20S\",\"label\":\"84 В (20S)\"},{\"value\":1008,\"id\":\"24S\",\"label\":\"100,8 В (24S)\"},{\"value\":1260" \
  ",\"id\":\"30S\",\"label\":\"126 В (30S)\"},{\"value\":1344,\"id\":\"32S\",\"label\":\"134,4 В (32S)\"},{\"value\":15" \
  "12,\"id\":\"36S\",\"label\":\"151,2 В (36S)\"},{\"value\":1680,\"id\":\"40S\",\"label\":\"168 В (40S)\"},{\"value\":" \
  "1764,\"id\":\"42S\",\"label\":\"176,4 В (42S)\"}],\"offset\":25,\"size\":2}]},{\"title\":\"Предупреждение о ШИМ" \
  "\",\"help\":\"Мигание экрана, когда ШИМ подходит к пределу\",\"fields\":[{\"key\":\"pwm_alarm\",\"type\":\"boo" \
  "l\",\"default\":true,\"label\":\"Мигать экраном при превышении порога ШИМ\",\"help\":\"Чем выше ШИМ, тем ч" \
  "аще мигание\",\"offset\":27,\"size\":1},{\"key\":\"pwm_alarm_source\",\"type\":\"u8\",\"default\":0,\"label\":\"По" \
  "рог\",\"options\":[{\"value\":0,\"id\":\"APP\",\"label\":\"Из настроек приложения LoEUC\"},{\"value\":1,\"id\":\"F" \
  "IXED\",\"label\":\"Свой\"}],\"depends\":{\"pwm_alarm\":true},\"offset\":28,\"size\":1},{\"key\":\"pwm_alarm_thre" \
  "shold\",\"type\":\"u8\",\"default\":80,\"min\":30,\"max\":99,\"unit\":\"%\",\"label\":\"Свой порог ШИМ\",\"depends\":" \
  "{\"pwm_alarm\":true,\"pwm_alarm_source\":1},\"offset\":29,\"size\":1}]},{\"title\":\"Дисплей\",\"help\":\"Яркос" \
  "ть экрана\",\"fields\":[{\"key\":\"brightness\",\"type\":\"u8\",\"default\":100,\"min\":5,\"max\":100,\"step\":5,\"u" \
  "nit\":\"%\",\"widget\":\"range\",\"label\":\"Яркость\",\"offset\":30,\"size\":1}]},{\"title\":\"Анимации\",\"help\":\"" \
  "Заставка при включении и анимация выключения\",\"fields\":[{\"key\":\"intro_logo\",\"type\":\"bool\",\"defau" \
  "lt\":true,\"section\":\"При включении\",\"label\":\"Логотип при включении\",\"help\":\"После логотипа играет" \
  " случайная из отмеченных заставок. Без логотипа данные появляются сразу\",\"preview\":\"logo\",\"offse" \
  "t\":31,\"size\":1},{\"key\":\"intro_clawd\",\"type\":\"bool\",\"default\":false,\"label\":\"Заставки Clawd\",\"hel" \
  "p\":\"Играют заставки Clawd\",\"widget\":\"clawd\",\"depends\":{\"intro_logo\":true},\"offset\":32,\"size\":1}," \
  "{\"key\":\"intro_wave\",\"type\":\"bool\",\"default\":true,\"label\":\"Волна\",\"help\":\"Волна с пузырями проход" \
  "ит слева направо и открывает данные\",\"preview\":\"wave\",\"depends\":{\"intro_logo\":true,\"intro_clawd\"" \
  ":false},\"subitem\":true,\"offset\":33,\"size\":1},{\"key\":\"intro_break\",\"type\":\"bool\",\"default\":true,\"" \
  "label\":\"Разрушение\",\"help\":\"Логотип трескается и рассыпается на куски\",\"preview\":\"break\",\"depend" \
  "s\":{\"intro_logo\":true,\"intro_clawd\":false},\"subitem\":true,\"offset\":34,\"size\":1},{\"key\":\"intro_ex" \
  "plosion\",\"type\":\"bool\",\"default\":true,\"label\":\"Взрыв\",\"help\":\"Логотип разлетается облаком частиц" \
  ", как TNT в Minecraft\",\"preview\":\"explosion\",\"depends\":{\"intro_logo\":true,\"intro_clawd\":false},\"" \
  "subitem\":true,\"offset\":35,\"size\":1},{\"key\":\"poweroff_animation\",\"type\":\"bool\",\"default\":true,\"se" \
  "ction\":\"При выключении\",\"label\":\"Анимация выключения\",\"help\":\"Экран сжимается в линию, затем в т" \
  "очку и гаснет, как у старого телевизора\",\"preview\":\"poweroff\",\"offset\":36,\"size\":1}]},{\"title\":\"" \
  "Кнопка\",\"help\":\"Сколько держать кнопку, чтобы включить и выключить дисплей\",\"fields\":[{\"key\":\"po" \
  "wer_on_ms\",\"type\":\"u16\",\"default\":500,\"min\":100,\"max\":3000,\"step\":100,\"unit\":\"мс\",\"label\":\"Удерж" \
  "ание для включения\",\"offset\":37,\"size\":2},{\"key\":\"power_off_ms\",\"type\":\"u16\",\"default\":1000,\"min" \
  "\":500,\"max\":5000,\"step\":100,\"unit\":\"мс\",\"label\":\"Удержание для выключения\",\"help\":\"Более коротко" \
  "е нажатие переключает экран\",\"offset\":39,\"size\":2}]},{\"title\":\"Обновление по Bluetooth\",\"help\":\"" \
  "Прошивка с телефона без кабеля\",\"fields\":[{\"key\":\"ble_update\",\"type\":\"bool\",\"default\":true,\"labe" \
  "l\":\"Разрешить обновление прошивки по Bluetooth\",\"help\":\"Режим обновления включается, только если" \
  " при включении держать кнопку ещё 3 секунды после того, как загорится экран, и заряд не ниже 30%" \
  ". В обычной работе прошить дисплей по Bluetooth нельзя\",\"offset\":41,\"size\":1}]}]}"
