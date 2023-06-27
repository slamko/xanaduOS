#include "drivers/rtc.h"
#include "drivers/cmos.h"
#include "drivers/pit.h"
#include "drivers/cmos.h"
#include "lib/slibc.h"
#include "drivers/fb.h"
#include "lib/kernel.h"
#include <stdint.h>

enum time_mode {
    H12_MODE = 0x1,
    BCD_MODE,
    H24_MODE,
    BINARY_MODE,
};

static enum time_mode time_mode;

uint8_t rtc_get_val(uint8_t reg) {
    cmos_select_reg(CMOS_STAT_A);

    while(cmos_read_data() & (1 << 7)) {
        cmos_select_reg(CMOS_STAT_A);
    }
   
    cmos_select_reg(reg);
    uint8_t value = cmos_read_data();

    if (time_mode == BCD_MODE) {
        value = ((value & 0xF0) >> 1) + ((value & 0xF0) >> 3) + (value & 0xf);
    }
    
    return value;
}

uint8_t rtc_get_seconds(void) {
    return rtc_get_val(CMOS_SECONDS);
}

uint8_t rtc_get_minutes(void) {
    return rtc_get_val(CMOS_MINUTES);
}

uint8_t rtc_get_hour(void) {
    return rtc_get_val(CMOS_HOURS);
}

uint8_t rtc_get_month(void) {
    return rtc_get_val(CMOS_MONTH);
}

uint8_t rtc_get_day_of_month(void) {
    cmos_select_reg(CMOS_DAY_OF_MONTH);
    return cmos_read_data();
}

uint16_t rtc_get_year(void) {
    uint16_t year = rtc_get_val(CMOS_YEAR);
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
    cmos_select_reg(CMOS_STAT_B);
    
    time_mode = cmos_read_data(); 

    char date[16];
    klog("Real Time Clock configured\n");
    
    klog("Current time: ");
    fb_print_black(get_date_time("h:m:s", date, 16));
}
