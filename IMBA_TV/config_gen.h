#pragma once

// Сгенерировано tools/gen_config.py из config.json, не править руками.
// После изменения config.json запустите: python3 tools/gen_config.py

#include <stdint.h>

// Настройки прошивки. Значения записывает веб-конфигуратор перед прошивкой.
struct __attribute__((packed)) fw_config_t
{
  uint8_t screen_charge;  // Скорость, ШИМ и заряд колеса
  uint8_t screen_voltage;  // Скорость, ШИМ и напряжение колеса
  uint8_t screen_grid;  // Сетка: заряд, напряжение, температура, пробег, связь
  uint8_t pwm_alarm;  // Мигать экраном при превышении порога ШИМ
  uint8_t pwm_alarm_source;  // Порог
  uint8_t pwm_alarm_threshold;  // Свой порог ШИМ
  uint8_t brightness;  // Яркость
  uint16_t wheel_voltage_x10;  // Батарея колеса
  uint8_t intro;  // Заставка при включении
  uint8_t poweroff_animation;  // Анимация выключения, как у старого телевизора
  uint16_t power_on_ms;  // Удержание для включения
  uint16_t power_off_ms;  // Удержание для выключения
};

static_assert(sizeof(fw_config_t) == 15, "config.json и fw_config_t разошлись");

#define FW_CONFIG_DEFAULTS { 1, 1, 1, 1, 0, 80, 100, 1512, 2, 1, 500, 1000 }

enum
{
  CONFIG_PWM_ALARM_SOURCE_APP = 0,
  CONFIG_PWM_ALARM_SOURCE_FIXED = 1,
  CONFIG_WHEEL_VOLTAGE_X10_20S = 840,
  CONFIG_WHEEL_VOLTAGE_X10_24S = 1008,
  CONFIG_WHEEL_VOLTAGE_X10_30S = 1260,
  CONFIG_WHEEL_VOLTAGE_X10_32S = 1344,
  CONFIG_WHEEL_VOLTAGE_X10_36S = 1512,
  CONFIG_WHEEL_VOLTAGE_X10_40S = 1680,
  CONFIG_WHEEL_VOLTAGE_X10_42S = 1764,
  CONFIG_INTRO_NONE = 0,
  CONFIG_INTRO_LOGO = 1,
  CONFIG_INTRO_RANDOM = 2,
  CONFIG_INTRO_WAVE = 3,
  CONFIG_INTRO_BREAK = 4,
  CONFIG_INTRO_EXPLOSION = 5,
};

// Значения вне допустимых заменяются значениями по умолчанию
static inline void configSanitize(fw_config_t& c)
{
  if(c.screen_charge > 1) c.screen_charge = 1;
  if(c.screen_voltage > 1) c.screen_voltage = 1;
  if(c.screen_grid > 1) c.screen_grid = 1;
  if(c.pwm_alarm > 1) c.pwm_alarm = 1;
  switch(c.pwm_alarm_source) { case 0: case 1: break; default: c.pwm_alarm_source = 0; }
  if(c.pwm_alarm_threshold < 30 || c.pwm_alarm_threshold > 99) c.pwm_alarm_threshold = 80;
  if(c.brightness < 5 || c.brightness > 100) c.brightness = 100;
  switch(c.wheel_voltage_x10) { case 840: case 1008: case 1260: case 1344: case 1512: case 1680: case 1764: break; default: c.wheel_voltage_x10 = 1512; }
  switch(c.intro) { case 0: case 1: case 2: case 3: case 4: case 5: break; default: c.intro = 2; }
  if(c.poweroff_animation > 1) c.poweroff_animation = 1;
  if(c.power_on_ms < 100 || c.power_on_ms > 3000) c.power_on_ms = 500;
  if(c.power_off_ms < 500 || c.power_off_ms > 5000) c.power_off_ms = 1000;
}

// Описание настроек для веб-конфигуратора: config.json с адресами полей
#define FW_CONFIG_SCHEMA \
  "{\"version\":1,\"groups\":[{\"title\":\"Экраны\",\"help\":\"Страницы, которые переключаются коротким нажати" \
  "ем кнопки\",\"require_one\":true,\"fields\":[{\"key\":\"screen_charge\",\"type\":\"bool\",\"default\":true,\"lab" \
  "el\":\"Скорость, ШИМ и заряд колеса\",\"offset\":0,\"size\":1},{\"key\":\"screen_voltage\",\"type\":\"bool\",\"d" \
  "efault\":true,\"label\":\"Скорость, ШИМ и напряжение колеса\",\"offset\":1,\"size\":1},{\"key\":\"screen_gri" \
  "d\",\"type\":\"bool\",\"default\":true,\"label\":\"Сетка: заряд, напряжение, температура, пробег, связь\",\"" \
  "offset\":2,\"size\":1}]},{\"title\":\"Предупреждение о ШИМ\",\"fields\":[{\"key\":\"pwm_alarm\",\"type\":\"bool\"" \
  ",\"default\":true,\"label\":\"Мигать экраном при превышении порога ШИМ\",\"help\":\"Чем выше ШИМ, тем чащ" \
  "е мигание\",\"offset\":3,\"size\":1},{\"key\":\"pwm_alarm_source\",\"type\":\"u8\",\"default\":0,\"label\":\"Порог" \
  "\",\"options\":[{\"value\":0,\"id\":\"APP\",\"label\":\"Из настроек приложения LoEUC\"},{\"value\":1,\"id\":\"FIXE" \
  "D\",\"label\":\"Свой\"}],\"depends\":{\"pwm_alarm\":true},\"offset\":4,\"size\":1},{\"key\":\"pwm_alarm_threshol" \
  "d\",\"type\":\"u8\",\"default\":80,\"min\":30,\"max\":99,\"unit\":\"%\",\"label\":\"Свой порог ШИМ\",\"depends\":{\"pw" \
  "m_alarm\":true,\"pwm_alarm_source\":1},\"offset\":5,\"size\":1}]},{\"title\":\"Дисплей\",\"fields\":[{\"key\":\"" \
  "brightness\",\"type\":\"u8\",\"default\":100,\"min\":5,\"max\":100,\"step\":5,\"unit\":\"%\",\"widget\":\"range\",\"la" \
  "bel\":\"Яркость\",\"offset\":6,\"size\":1},{\"key\":\"wheel_voltage_x10\",\"type\":\"u16\",\"default\":1512,\"labe" \
  "l\":\"Батарея колеса\",\"help\":\"По максимальному напряжению рассчитывается место под цифры на экране" \
  " напряжения\",\"options\":[{\"value\":840,\"id\":\"20S\",\"label\":\"84 В (20S)\"},{\"value\":1008,\"id\":\"24S\",\"" \
  "label\":\"100,8 В (24S)\"},{\"value\":1260,\"id\":\"30S\",\"label\":\"126 В (30S)\"},{\"value\":1344,\"id\":\"32S\"" \
  ",\"label\":\"134,4 В (32S)\"},{\"value\":1512,\"id\":\"36S\",\"label\":\"151,2 В (36S)\"},{\"value\":1680,\"id\":\"" \
  "40S\",\"label\":\"168 В (40S)\"},{\"value\":1764,\"id\":\"42S\",\"label\":\"176,4 В (42S)\"}],\"offset\":7,\"size\"" \
  ":2}]},{\"title\":\"Анимации\",\"fields\":[{\"key\":\"intro\",\"type\":\"u8\",\"default\":2,\"label\":\"Заставка при" \
  " включении\",\"options\":[{\"value\":0,\"id\":\"NONE\",\"label\":\"Без заставки\"},{\"value\":1,\"id\":\"LOGO\",\"la" \
  "bel\":\"Только логотип\"},{\"value\":2,\"id\":\"RANDOM\",\"label\":\"Случайная анимация\"},{\"value\":3,\"id\":\"W" \
  "AVE\",\"label\":\"Волна\"},{\"value\":4,\"id\":\"BREAK\",\"label\":\"Разрушение\"},{\"value\":5,\"id\":\"EXPLOSION\"," \
  "\"label\":\"Взрыв\"}],\"offset\":9,\"size\":1},{\"key\":\"poweroff_animation\",\"type\":\"bool\",\"default\":true," \
  "\"label\":\"Анимация выключения, как у старого телевизора\",\"offset\":10,\"size\":1}]},{\"title\":\"Кнопка" \
  "\",\"fields\":[{\"key\":\"power_on_ms\",\"type\":\"u16\",\"default\":500,\"min\":100,\"max\":3000,\"step\":100,\"uni" \
  "t\":\"мс\",\"label\":\"Удержание для включения\",\"offset\":11,\"size\":2},{\"key\":\"power_off_ms\",\"type\":\"u1" \
  "6\",\"default\":1000,\"min\":500,\"max\":5000,\"step\":100,\"unit\":\"мс\",\"label\":\"Удержание для выключения\"" \
  ",\"help\":\"Более короткое нажатие переключает экран\",\"offset\":13,\"size\":2}]}]}"
