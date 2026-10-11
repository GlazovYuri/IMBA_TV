// Сцены для видео о проекте: настоящий код экранов и анимаций прошивки на компьютере.
// Эмулятор SSH1106 и виртуальное время берутся из стенда превью (tools/previews/harness.cpp),
// его main переименован, здесь свои сценарии. Сценарии повторяют loop() прошивки:
// displayDrawIface + displayUpdateAlarm каждые 40 мс. Запуск через tools/brag/make.mjs.
#define main previews_main
#include "../previews/harness.cpp"
#undef main

static const uint32_t loop_ms = 40;

static euc_data_t ride(float speed, float pwm)
{
  euc_data_t d = sampleData();
  d.speed = (uint16_t)(speed + 0.5f);
  d.pwm = (uint8_t)(pwm + 0.5f);
  d.present |= EUC_HAS_PWM_LIMIT;
  d.last_frame_ms = millis();
  return d;
}

static float lerpf(float a, float b, float k) { if(k < 0) k = 0; if(k > 1) k = 1; return a + (b - a) * k; }
static float smooth(float k) { if(k < 0) k = 0; if(k > 1) k = 1; return k * k * (3 - 2 * k); }

// цикл прошивки на duration мс; data_at(t) - данные от телефона, t в секундах от начала
template<typename F> static void runLoop(uint32_t duration, F data_at, bool charging = false)
{
  uint32_t start = millis();
  while(millis() - start < duration)
  {
    euc_data_t d = data_at((float)(millis() - start) / 1000.0f);
    displayDrawIface(dev_charge, charging, d);
    displayUpdateAlarm(d);
    delay(loop_ms);
  }
}

static void savePath(const std::string& path) { save(path.c_str()); }

int main(int argc, char** argv)
{
  std::string out = argc > 1 ? argv[1] : ".";
  uint32_t seed = argc > 2 ? strtoul(argv[2], nullptr, 0) : 0x5EED1234u;

  configLoad();
  displayInit();

  // включение: логотип, взрыв TNT, под ним данные
  begin();
  {
    euc_data_t d = ride(27, 41);
    displayPlayIntroNum(2, seed, dev_charge, false, d);
    runLoop(6000, [](float) { return ride(27, 41); });
  }
  savePath(out + "/hook.bin");

  // связь: пока LoEUC не подключился, «нет связи», затем данные
  begin();
  runLoop(5000, [](float t) {
    euc_data_t d = ride(27, 41);
    if(t < 1.6f) d.is_connected = false;
    return d;
  });
  savePath(out + "/link.bin");

  // едем: скорость и ШИМ растут
  begin();
  runLoop(3800, [](float t) {
    float k = smooth((t - 0.2f) / 1.7f);
    return ride(lerpf(27, 36, k), lerpf(41, 62, k));
  });
  savePath(out + "/ride.bin");

  // ШИМ подходит к порогу 80% из LoEUC: экран мигает, чем выше ШИМ, тем чаще
  begin();
  runLoop(3800, [](float t) {
    float k = smooth(t / 2.0f);
    return ride(lerpf(36, 46, k), lerpf(62, 97, k));
  });
  savePath(out + "/alarm.bin");

  // кабель USB-C: стоим, затем вставили кабель - значок зарядки вместо батареи дисплея
  begin();
  runLoop(1200, [](float) { return ride(0, 0); });
  runLoop(2400, [](float) { return ride(0, 0); }, true);
  savePath(out + "/charging.bin");

  // режим обновления: QR-код сайта со спиннером, затем «Устройство подключено» (кадр замирает)
  begin();
  {
    uint32_t start = millis();
    while(millis() - start < 3000) { displayDrawUpdateMode(false); delay(loop_ms); }
    displayDrawUpdateMode(true);
    delay(4000);
  }
  savePath(out + "/update.bin");

  // выключение: данные, затем схлопывание как у ЭЛТ
  begin();
  runLoop(1500, [](float) { return ride(27, 41); });
  {
    euc_data_t d = ride(27, 41);
    displayPowerOffAnimation(dev_charge, false, d);
  }
  delay(1500);
  savePath(out + "/poweroff.bin");

  // логотип для концовки
  begin();
  displayDrawLogo();
  delay(1000);
  savePath(out + "/logo.bin");

  // заставки: логотип секунду и переход к данным
  const char* names[3] = {"wave", "break", "explosion"};
  for(int anim = 0; anim < 3; anim++)
  {
    begin();
    euc_data_t d = ride(27, 41);
    displayPlayIntroNum(anim, seed, dev_charge, false, d);
    runLoop(1600, [](float) { return ride(27, 41); });
    savePath(out + "/" + names[anim] + ".bin");
  }

  // экраны, настроенные на сайте: на первом справа температура вместо ШИМ.
  // Короткое нажатие переключает экран, точки внизу показывают номер
  {
    fw_config_t& cfg = configMut();
    cfg.screen1_type = 1; cfg.screen1_v1 = 2; cfg.screen1_v2 = 0; cfg.screen1_v3 = 4; cfg.screen1_v4 = 5;
    cfg.screen2_type = 1; cfg.screen2_v1 = 3; cfg.screen2_v2 = 0; cfg.screen2_v3 = 1; cfg.screen2_v4 = 5;
    cfg.screen3_type = 2; cfg.screen3_v1 = 2; cfg.screen3_v2 = 3; cfg.screen3_v3 = 4; cfg.screen3_v4 = 5;
    cfg.screen4_type = 3;
    cfg.screen5_type = 0;
    displayInit();
    begin();
    auto data = [](float) { return ride(31, 47); };
    runLoop(900, data);
    for(int i = 0; i < 3; i++)
    {
      displayNextMode();
      runLoop(700, data);
    }
    savePath(out + "/cycle.bin");
  }
  return 0;
}
