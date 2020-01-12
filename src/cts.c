
#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <zephyr.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <drivers/i2c.h>
#include <device.h>

#include <time.h>

#include "cts.h"

#define DS3231_I2C_ADDR		0x68
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

#define I2C_DEV "I2C_0"

struct device *i2c_dev;

static u8_t ct[7] = {21, 05, 00, 07, 12, 01, 20};
static u8_t ct_update;

static void ct_ccc_cfg_changed(const struct bt_gatt_attr *attr, u16_t value)
{
	//EMPTY
}

static ssize_t read_ct(struct bt_conn *conn, const struct bt_gatt_attr *attr,
		       void *buf, u16_t len, u16_t offset)
{
	const char *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(ct));
}

static ssize_t write_ct(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			const void *buf, u16_t len, u16_t offset,
			u8_t flags)
{
	u8_t *value = attr->user_data;

	if (offset + len > sizeof(ct)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf, len);
	ct_update = 1U;

	encode_time(ct[2], ct[1], ct[0], ct[3], ct[4], ct[5], ct[6]);

	return len;
}

/* Current Time Service Declaration */
BT_GATT_SERVICE_DEFINE(cts_cvs,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_CTS),
	BT_GATT_CHARACTERISTIC(BT_UUID_CTS_CURRENT_TIME, BT_GATT_CHRC_READ |
			       BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			       read_ct, write_ct, ct),
	BT_GATT_CCC(ct_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

void encode_time(u8_t sec, u8_t min, u8_t hour, u8_t day,
						u8_t date, u8_t month, u8_t year) {
	u8_t enc_ct[7];

	enc_ct[0] = ((sec / 10) << 4) + (sec % 10);
	enc_ct[1] = ((min / 10) << 4) + (min % 10);
	enc_ct[2] = ((hour / 10) << 4) + (hour % 10);
	enc_ct[3] = day;
	enc_ct[4] = ((date / 10) << 4) + (date % 10);
	enc_ct[5] = ((month / 10) << 4) + (month % 10);
	enc_ct[5] |= ((year / 100) << 7) & 0x80;
	enc_ct[6] = ((year / 10) << 4) + (year % 10);

	int err = i2c_burst_write(i2c_dev, DS3231_I2C_ADDR, REG_SECONDS, enc_ct, 7);
	if (err) {
		printk("Failed to encode time, err: %d", err);
	}
}

void decode_time(void) {
	u8_t start_reg[1] = {REG_SECONDS};
	i2c_write_read(i2c_dev, DS3231_I2C_ADDR, start_reg, 1, ct, 7);

	ct[0] = (((ct[0] >> 4) & 0x7) * 10) + (ct[0] & 0xf);	//Seconds
	ct[1] = (((ct[1] >> 4) & 0x7) * 10) + (ct[1] & 0xf);	//Minutes
	ct[2] = (((ct[2] >> 4) & 0x3) * 10) + (ct[2] & 0xf);	//Hours
	ct[3] = ct[3] & 0x7;									//Days
	ct[4] = (((ct[4] >> 4) & 0x3) * 10) + (ct[4] & 0xf);	//Date
	ct[5] = (((ct[5] >> 4) & 0x1) * 10) + (ct[5] & 0xf);	//Month
	ct[6] = (((ct[5] >> 7) & 0x1) * 100) + 					//Century
			(((ct[6] >> 4) & 0xf) * 10) + (ct[6] & 0xf);	//Year
	
	/*Swap sec and hour for clarity*/
	u8_t tmp = ct[0];
	ct[0] = ct[2];
	ct[2] = tmp;
}

void print_ct(void) {
	decode_time();

	printk("Time: %d : %d : %d\tDay: %d\tDate: %d / %d / %d\n",
			ct[0], ct[1], ct[2], ct[3],
			ct[4], ct[5], ct[6]);
}

int cts_init(void)
{
	int err;

	i2c_dev = device_get_binding(I2C_DEV);
	if (!i2c_dev) {
		printk("I2C: Device driver %s not found.\n", I2C_DEV);
		return err;
	}

	/*Configure DS device*/
	/*Set 24h clock*/
	i2c_reg_update_byte(i2c_dev, DS3231_I2C_ADDR, REG_HOURS,
						(1 << 6), 0);
	/*Enable both interrupt on alarm 1 and alarm 2*/
	i2c_reg_update_byte(i2c_dev, DS3231_I2C_ADDR, REG_CONTROL,
						0x3, 0x3);
	/*Set current time*/
	encode_time(ct[2], ct[1], ct[0], ct[3], ct[4], ct[5], ct[6]);

	return 0;
}

void cts_notify(void)
{	
	bt_gatt_notify(NULL, &cts_cvs.attrs[1], &ct, sizeof(ct));
}
