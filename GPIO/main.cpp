#include <stdio.h>
#include <gpiod.h>
#include <unistd.h>

int main() {
  const char* chipname = "gpiochip0"; // ��� GPIO-���� (����� ����������)
  unsigned int line_num = 18; // ����� GPIO-����� (����� ����)

  // �������� GPIO-����
  struct gpiod_chip* chip = gpiod_chip_open_by_name(chipname);
  if (!chip) {
    perror("�� ������� ������� GPIO-���");
    return 1;
  }

  // ��������� GPIO-�����
  struct gpiod_line* line = gpiod_chip_get_line(chip, line_num);
  if (!line) {
    perror("�� ������� �������� GPIO-�����");
    gpiod_chip_close(chip);
    return 1;
  }

  // ��������� ������ ������
  if (gpiod_line_request_output(line, "toggle_gpio", GPIOD_LINE_ACTIVE_STATE_HIGH) < 0) {
    perror("�� ������� ���������� ����� ������");
    gpiod_line_release(line);
    gpiod_chip_close(chip);
    return 1;
  }

  while (1) {
    // ��������� GPIO-����
    gpiod_line_set_value(line, 1);
    //sleep(1);
    usleep(100000);

    // ���������� GPIO-����
    gpiod_line_set_value(line, 0);
    //sleep(1);
    usleep(100000);
  }

  // ������������ ��������
  gpiod_line_release(line);
  gpiod_chip_close(chip);

  return 0;

}
