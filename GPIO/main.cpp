#include <stdio.h>
#include <gpiod.h>
#include <unistd.h>

int main() {
  const char* chipname = "gpiochip0"; // Имя GPIO-чипа (может отличаться)
  unsigned int line_num = 18; // Номер GPIO-линии (номер пина)

  // Открытие GPIO-чипа
  struct gpiod_chip* chip = gpiod_chip_open_by_name(chipname);
  if (!chip) {
    perror("Не удалось открыть GPIO-чип");
    return 1;
  }

  // Получение GPIO-линии
  struct gpiod_line* line = gpiod_chip_get_line(chip, line_num);
  if (!line) {
    perror("Не удалось получить GPIO-линию");
    gpiod_chip_close(chip);
    return 1;
  }

  // Установка режима вывода
  if (gpiod_line_request_output(line, "toggle_gpio", GPIOD_LINE_ACTIVE_STATE_HIGH) < 0) {
    perror("Не удалось установить режим вывода");
    gpiod_line_release(line);
    gpiod_chip_close(chip);
    return 1;
  }

  while (1) {
    // Включение GPIO-пина
    gpiod_line_set_value(line, 1);
    //sleep(1);
    usleep(100000);

    // Выключение GPIO-пина
    gpiod_line_set_value(line, 0);
    //sleep(1);
    usleep(100000);
  }

  // Освобождение ресурсов
  gpiod_line_release(line);
  gpiod_chip_close(chip);

  return 0;

}
