// Прогоняет настоящий displayPlayIntroNum() из прошивки и дописывает буфер OLED в файл на каждом delay().
// Использование: intro_harness out.bin anim seed
#define private public
#include "../../../IMBA_TV/display.cpp"
#undef private

static FILE* g_out;
static void hook(uint32_t) { fwrite(oled._oled_buffer, 1, 1024, g_out); }

int main(int argc, char** argv) {
  g_out = fopen(argv[1], "wb");
  g_delay_hook = hook;
  euc_data_t d;
  d.is_connected = true; d.speed = 0; d.pwm = 0; d.charge = 100;
  displayPlayIntroNum(atoi(argv[2]), (uint32_t)strtoul(argv[3], 0, 10), 100, false, d);
  fclose(g_out);
  return 0;
}
