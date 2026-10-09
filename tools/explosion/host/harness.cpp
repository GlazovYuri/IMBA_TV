// Рендерит настоящий displayDrawIface() из прошивки и пишет буфер OLED (1024 байта) в файл.
// Использование: harness out.bin dev_charge charging connected speed pwm euc_charge
#define private public
#include "../../../IMBA_TV/display.cpp"
#undef private

int main(int argc, char** argv) {
  if (argc < 8) return 1;
  euc_data_t d;
  d.is_connected = atoi(argv[4]);
  d.speed = atoi(argv[5]);
  d.pwm = atoi(argv[6]);
  d.charge = atoi(argv[7]);
  displayDrawIface(atoi(argv[2]), atoi(argv[3]), d);
  FILE* f = fopen(argv[1], "wb");
  fwrite(oled._oled_buffer, 1, 1024, f);
  fclose(f);
  return 0;
}
