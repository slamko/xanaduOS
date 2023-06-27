#include "drivers/rtc.h"
#include "drivers/cmos.h"
#include "drivers/pit.h"
#include "drivers/cmos.h"
#include "lib/slibc.h"
#include "drivers/fb.h"

uint8_t rtc_get_seconds(void) {
    cmos_select_reg(CMOS_SECONDS);
    return cmos_read_data();
}

uint8_t rtc_get_minutes(void) {
    cmos_select_reg(CMOS_MINUTES);
    return cmos_read_data();
}

uint8_t rtc_get_hour(void) {
    cmos_select_reg(CMOS_HOURS);
    return cmos_read_data();
}

uint8_t rtc_get_month(void) {
    cmos_select_reg(CMOS_MONTH);
    return cmos_read_data();
}

uint8_t rtc_get_day_of_month(void) {
    cmos_select_reg(CMOS_DAY_OF_MONTH);
    return cmos_read_data();
}

uint16_t rtc_get_year(void) {
    cmos_select_reg(CMOS_YEAR);
    uint16_t year = (uint16_t)cmos_read_data();
    return 2000 + year;
}

const char *get_date_time(const char *format, char *buf, size_t size) {
    memset(buf, 0, size);
    size_t cur_buf_pos = 0;

    for (unsigned int i = 0; format[i]; i ++) {
        char local_buf[16];
        memset(local_buf, 0, sizeof(local_buf));
        size_t chars_wrote = 0;
        char *res = NULL;
        
        switch (format[i]) {
        case 'h':;
            res = itoa(rtc_get_hour(), local_buf, size, &chars_wrote, 10);
            strcpy(buf + cur_buf_pos, res, chars_wrote);
            break;
        case 's':;
            res = itoa(rtc_get_seconds(), local_buf, size, &chars_wrote, 10);
            strcpy(buf + cur_buf_pos, res, chars_wrote);
            break;
        case 'm':;
            res = itoa(rtc_get_minutes(), local_buf, size, &chars_wrote, 10);
            strcpy(buf + cur_buf_pos, res, chars_wrote);
            break;
        case 'y':;
            res = itoa(rtc_get_year(), local_buf, size, &chars_wrote, 10);
            strcpy(buf + cur_buf_pos, res, chars_wrote);
            break;
        case 'd':;
            res = itoa(rtc_get_day_of_month(), local_buf, size, &chars_wrote, 10);
            strcpy(buf + cur_buf_pos, res, chars_wrote);
            break;
        case 'M':;
            res = itoa(rtc_get_month(), local_buf, size, &chars_wrote, 10);
            strcpy(buf + cur_buf_pos, res, chars_wrote);
            break;
        default:
            buf[cur_buf_pos] = format[i];
            cur_buf_pos += 1;
            break;
        }

        cur_buf_pos += chars_wrote;
    }

    return buf;
}

void rtc_init(void) {
}
