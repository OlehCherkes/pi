#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

int main() {
  const char* device = "/dev/spidev0.0";
  int spiHandle;

  // Open SPI
  spiHandle = open(device, O_RDWR);
  if (spiHandle == -1) {
    std::cerr << "Unable to open SPI device." << std::endl;
    return 1;
  }

  // Mod SPI
  int mode = SPI_MODE_0;
  if (ioctl(spiHandle, SPI_IOC_WR_MODE, &mode) == -1) {
    std::cerr << "Unable to set SPI mode." << std::endl;
    return 1;
  }

  // Speed SPI
  int speed = 500000; // Hz
  if (ioctl(spiHandle, SPI_IOC_WR_MAX_SPEED_HZ, &speed) == -1) {
    std::cerr << "Unable to set SPI speed." << std::endl;
    return 1;
  }

  // Eõample send
  unsigned char buffer[3] = { 0x01, 0x02, 0x03 };
  unsigned char rx_buffer[3] = { 0 };

  struct spi_ioc_transfer transfer = {
      .tx_buf = (unsigned long)buffer,
      .rx_buf = (unsigned long)NULL,
      .len = sizeof(buffer),
      .speed_hz = speed,
      .bits_per_word = 8,
  };
  if (ioctl(spiHandle, SPI_IOC_MESSAGE(1), &transfer) == -1) {
    std::cerr << "SPI transfer failed." << std::endl;
    return 1;
  }

  std::cout << "SPI send." << std::endl;

  // Process received data
  std::cout << "Received data: ";
  for (int i = 0; i < sizeof(rx_buffer); ++i) {
    std::cout << std::hex << (int)rx_buffer[i] << " ";
  }
  std::cout << std::endl;


  close(spiHandle);

  return 0;
}