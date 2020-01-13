
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

#include "cts.h"
#include "DS3231.h"

#define CT_LEN 7
static u8_t ct[CT_LEN] = {21, 05, 00, 07, 12, 01, 20};

static ssize_t read_ct(struct bt_conn *conn, const struct bt_gatt_attr *attr,
		       void *buf, u16_t len, u16_t offset)
{
	const char *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(ct));
}

void encode_time(u8_t sec, u8_t min, u8_t hour, u8_t day,
						u8_t date, u8_t month, u8_t year);

static ssize_t write_ct(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			const void *buf, u16_t len, u16_t offset,
			u8_t flags)
{
	u8_t *value = attr->user_data;

	if (offset + len > sizeof(ct)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf, len);

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
	BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

void encode_time(u8_t sec, u8_t min, u8_t hour, u8_t day,
						u8_t date, u8_t month, u8_t year) {
	u8_t enc_ct[CT_LEN];

	enc_ct[0] = ((sec / 10) << 4) + (sec % 10);
	enc_ct[1] = ((min / 10) << 4) + (min % 10);
	enc_ct[2] = ((hour / 10) << 4) + (hour % 10);
	enc_ct[3] = day;
	enc_ct[4] = ((date / 10) << 4) + (date % 10);
	enc_ct[5] = ((month / 10) << 4) + (month % 10);
	enc_ct[5] |= ((year / 100) << 7) & 0x80;
	enc_ct[6] = ((year / 10) << 4) + (year % 10);

	ds_write_time(enc_ct, CT_LEN);
}

void decode_time(void) {
	ds_read_time(ct, CT_LEN);

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

void cts_print_ct(void) {
	decode_time();

	printk("Time: %d : %d : %d\tDay: %d\tDate: %d / %d / %d\n",
			ct[0], ct[1], ct[2], ct[3],
			ct[4], ct[5], ct[6]);
}

void cts_init(void)
{
	/*Configure DS device*/
	/*Set 24h clock*/
	ds_modify_reg(REG_HOURS, (1 << 6), 0);
	/*Set current time*/
	encode_time(ct[2], ct[1], ct[0], ct[3], ct[4], ct[5], ct[6]);
}

void cts_notify(void)
{	
	bt_gatt_notify(NULL, &cts_cvs.attrs[1], &ct, sizeof(ct));
}
