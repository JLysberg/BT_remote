#ifndef DS3231_RTC_H
#define DS3231_RTC_H

#include <zephyr/types.h>

#define REG_SECONDS			0x00
#define REG_MINUTES 		0x01
#define REG_HOURS			0x02
#define REG_DAY				0x03
#define REG_DATE			0x04
#define REG_MONTH			0x05
#define REG_YEAR 			0x06
#define REG_AL1_SECS 		0x07
#define REG_AL1_MINS 		0x08
#define REG_AL1_HRS 		0x09
#define REG_AL1_DAY_DATE	0x0A
#define REG_AL2_MINS 		0x0B
#define REG_AL2_HRS 		0x0C
#define REG_AL2_DAY_DATE 	0x0D
#define REG_CONTROL 		0x0E
#define REG_STATUS 			0x0F
#define REG_AG_OFFS 		0x10
#define REG_TEMP_MSB 		0x11
#define REG_TEMP_LSB 		0x12

void ds_init(void);

void ds_write_time(u8_t ct_in[], u8_t ct_len);

void ds_read_time(u8_t ct_out[], u8_t ct_len);

void ds_modify_reg(u8_t reg_addr, u8_t mask, u8_t value);

#endif