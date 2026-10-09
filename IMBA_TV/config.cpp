#include "config.h"
#include <stddef.h>

// Блок настроек в образе прошивки. Сайт находит его по magic, строит форму по schema
// и записывает выбранные значения в data. volatile не даёт компилятору подставить
// значения по умолчанию прямо в код: их нужно читать из флеша.
struct __attribute__((packed)) fw_config_block_t
{
  char magic[8];
  uint16_t data_size;
  uint16_t schema_size;
  fw_config_t data;
  char schema[sizeof(FW_CONFIG_SCHEMA)];
};

extern const volatile fw_config_block_t fw_config_block;
__attribute__((used)) const volatile fw_config_block_t fw_config_block = {
  {'I', 'M', 'B', 'A', '_', 'C', 'F', 'G'},
  sizeof(fw_config_t),
  sizeof(FW_CONFIG_SCHEMA),
  FW_CONFIG_DEFAULTS,
  FW_CONFIG_SCHEMA,
};

static fw_config_t cfg = FW_CONFIG_DEFAULTS;

void configLoad()
{
  const volatile uint8_t* src = (const volatile uint8_t*)&fw_config_block.data;
  uint8_t* dst = (uint8_t*)&cfg;
  for(size_t i = 0; i < sizeof(cfg); i++) dst[i] = src[i];

  configSanitize(cfg);
  // хотя бы один экран должен остаться
  if(!cfg.screen_charge && !cfg.screen_voltage && !cfg.screen_grid) cfg.screen_charge = 1;
}

const fw_config_t& config()
{
  return cfg;
}
