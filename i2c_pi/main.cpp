#include <iostream>
#include <cstdint>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <vector>
#include <ctime>


extern "C"
{
  #include <linux/i2c.h>
  #include <linux/i2c-dev.h>
  #include <i2c/smbus.h>
}

#define I2C_SLAVE_ADDR    0x6F     // Slave address
#define EEPROM_ADDR       0x57     // Slave address

#define ST                7        // Seconds register (TIME_REG) oscillator start/stop bit, 1==Start, 0==Stop
#define HR1224            6        // Hours register (TIME_REG+2) 12 or 24 hour mode (24 hour mode==0)
#define OSCON             5        // Day register (TIME_REG+3) oscillator running (set and cleared by hardware)
#define VBATEN            3        // Day register (TIME_REG+3) VBATEN==1 enables backup battery, VBATEN==0 disconnects the VBAT pin (e.g. to save battery)
#define VBAT              4        // Day register (TIME_REG+3) set by hardware when Vcc fails and RTC runs on battery.
#define LP                5        // Month register (TIME_REG+5) leap year bit
                          
                          
#define OSCRUN            0x20     // Oscillator Status bit
#define PWRFAIL           0x10     // Power Failure Status bit
#define VBEN              0x8      // External Battery Backup Supply (VBAT) Enable bit

#define RTCSEC            0x00
#define RTCMIN            0x01
#define RTCHOUR           0x02
#define RTCWKDAY          0x03
#define RTCDATE           0x04
#define RTCMTH            0x05
#define RTCYEAR           0x06
#define CONTROL           0x07
#define OSCTRIM           0x08
#define EEUNLOCK          0x09

#define RTCSRAM           0x20
#define RTCEEPROM         0x00

const char* i2c_device = "/dev/i2c-1";
int file = open(i2c_device, O_RDWR);

typedef enum {
  RTC_OK = 0,
  RTC_FAIL,
  RTC_CONNECTION,
  RTC_LINK,
  RTC_TRANSACTION,
  RTC_OSCILLATOR,
  RTC_POWER,
  RTC_BATTERY
} Rtc_error;

typedef struct {
  uint8_t sec;
  uint8_t min;
  uint8_t hour;
  uint8_t day;
  uint8_t month;
  uint8_t year;
  uint8_t wday;
  uint8_t yday;
  uint8_t isdst;
} DateTime_t;

DateTime_t t, t2;

uint8_t dec2bcd(uint8_t n) {
  return n + 6 * (n / 10);
}

uint8_t bcd2dec(uint8_t n) {
  return n - 6 * (n >> 4);
}

void DataInit(DateTime_t* tm) {
  tm->sec = 50;
  tm->min = 27;
  tm->hour = 15;
  tm->wday = 2;
  tm->day = 21;
  tm->month = 2;
  tm->year = 24;
}

void getcurrentTime(DateTime_t* tm) {
  std::time_t currentTime = std::time(nullptr);
  std::tm* localTime = std::localtime(&currentTime);

  tm->sec = static_cast<uint8_t>(localTime->tm_sec);
  tm->min = static_cast<uint8_t>(localTime->tm_min);
  tm->hour = static_cast<uint8_t>(localTime->tm_hour);
  tm->day = static_cast<uint8_t>(localTime->tm_mday);
  tm->month = static_cast<uint8_t>(localTime->tm_mon + 1);
  tm->year = static_cast<uint8_t>(localTime->tm_year - 100);
  tm->wday = static_cast<uint8_t>(localTime->tm_wday);
  tm->yday = static_cast<uint8_t>(localTime->tm_yday);
}

int readData(__u8 reg) {
    // Read Data
    int res = i2c_smbus_read_byte_data(file, reg);

    if (res < 0) {
      printf("ERROR HANDLING : i2c transaction failed");
      close(file);
      return 1;
    }

    return res;
}

int writeData(__u8 reg, uint8_t value) {
  // Write Data
  int res = i2c_smbus_write_byte_data(file, reg, value);

  if (res < 0) {
    perror("Failed to write byte data");
    close(file);
    return 1;
  }

  return res;
}

//--------------------- Get Time -----------------------------------//
Rtc_error getTimeUtc(DateTime_t* tm) {
  tm->sec = readData(RTCSEC);
  tm->min = readData(RTCMIN);
  tm->hour = readData(RTCHOUR);
  tm->wday = readData(RTCWKDAY);
  tm->day = readData(RTCDATE);
  tm->month = readData(RTCMTH);
  tm->year= readData(RTCYEAR);

  // Perform necessary bit masking and BCD to decimal conversion
  tm->sec = bcd2dec(tm->sec & ~(1 << 7));
  tm->hour = bcd2dec(tm->hour & ~(1 << HR1224));
  tm->min = bcd2dec(tm->min);
  tm->wday &= ~((1 << OSCON) | (1 << VBAT) | (1 << VBATEN));
  tm->day = bcd2dec(tm->day);
  tm->month = bcd2dec(tm->month & ~(1 << LP));
  tm->year = bcd2dec(tm->year);

  return RTC_OK;
}

//---------------------------------------------------------------------------
//Rtc_error getTimeUtc2(DateTime_t* tm) {
//  __u8 data[7];
//
//  i2c_smbus_read_block_data(file, 0x00, data);
//
//  tm->sec = data[0];
//  tm->min = data[1];
//  tm->hour = data[2];
//  tm->wday = data[3];
//  tm->day = data[4];
//  tm->month = data[5];
//  tm->year = data[6];
//
//  // Perform necessary bit masking and BCD to decimal conversion
//  tm->sec = bcd2dec(tm->sec & ~(1 << 7));
//  tm->hour = bcd2dec(tm->hour & ~(1 << HR1224));
//  tm->min = bcd2dec(tm->min);
//  tm->wday &= ~((1 << OSCON) | (1 << VBAT) | (1 << VBATEN));
//  tm->day = bcd2dec(tm->day);
//  tm->month = bcd2dec(tm->month & ~(1 << LP));
//  tm->year = bcd2dec(tm->year);
//
//  return RTC_OK;
//}
//---------------------------------------------------------------------------

//--------------------- Set Time -----------------------------------//
Rtc_error setTime(DateTime_t* tm) {
  
  writeData(RTCSEC, 0x00);                       // stops the oscillator (Bit 7, ST == 0)
  writeData(RTCMIN, dec2bcd(tm->min));
  writeData(RTCHOUR, dec2bcd(tm->hour));
  writeData(RTCWKDAY, tm->wday | (1 << VBATEN)); // enable battery backup operation
  writeData(RTCDATE, dec2bcd(tm->day));
  writeData(RTCMTH, dec2bcd(tm->month));
  writeData(RTCYEAR, dec2bcd(tm->year));

  writeData(RTCSEC, dec2bcd(tm->sec) | (1 << ST)); // start the oscillator (Bit 7, ST == 1)

  return RTC_OK;
}

//--------------------- Get MEM -----------------------------------//
void getSRAM(__u8 reg, size_t size) {
  std::vector<uint8_t> values(size);

  for (auto& data : values) {
    data = readData(reg);
    printf("Mem DATA: %d\n", data);
    reg++;
  }
}

//--------------------- SET MEM -----------------------------------//
void setSRAM(__u8 reg, size_t size) {
  for (uint8_t i = 0; i < size; i++) {
    writeData(reg, i);
    reg++;
  }
}

void linuxTime() {
  std::time_t currentTime = std::time(nullptr);
  std::string currentTimeString = std::ctime(&currentTime);
  std::cout << "LINUX: " << currentTimeString;
}

int main() {

  if (file < 0) {
    perror("Failed to open I2C device");
    return 1;
  }

  if (ioctl(file, I2C_SLAVE, I2C_SLAVE_ADDR) < 0) {
    perror("Failed to set I2C address");
    close(file);
    return 1;
  }

  // Init time
  //DataInit(&t2);
  getcurrentTime(&t2);
  setTime(&t2);

  // Set Mem
  //setSRAM(RTCSEC, 64);

  while (true)
  {
    // Get time
    getTimeUtc(&t);
    printf("RTC: %02u:%02u:%02u %02u/%02u/%02u %02u      ", t.hour, t.min, t.sec, t.day, t.month, t.year, t.wday);

    linuxTime();

    // Get mem
    //getSRAM (RTCSEC, 64);

    usleep(1000000);
  }

  close(file);
  return 0;
}