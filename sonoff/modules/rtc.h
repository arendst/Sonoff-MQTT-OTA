#ifndef _RTC_H_
#define _RTC_H_

#define NTP_SERVER1 "pool.ntp.org"
#define NTP_SERVER2 "nl.pool.ntp.org"
#define NTP_SERVER3 "0.nl.pool.ntp.org"

typedef struct {
  uint8_t Second;
  uint8_t Minute;
  uint8_t Hour;
  uint8_t Wday;   // day of week, sunday is day 1
  uint8_t Day;
  uint8_t Month;
  uint16_t Year;
  unsigned long Valid;
} TIME_T;

extern TIME_T rtcTime;

void rtc_init(uint8_t timezone);
void rtc_timezone(uint8_t timezone);
void rtc_second();

#endif
