#ifndef RTC_H
#define RTC_H

#include <stddef.h>
#include <stdint.h>

const char *get_date_time(const char *format, char *buf, size_t size);

uint8_t rtc_get_seconds(void);

uint8_t rtc_get_minutes(void);

uint8_t rtc_get_hour(void);

uint8_t rtc_get_month(void);

uint8_t rtc_get_day_of_month(void);

uint16_t rtc_get_year(void);

void rtc_init(void);

#endif
