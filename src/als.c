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

#include "DS3231.h"
#include "als.h"

/* Custom Service Variables */
static struct bt_uuid_128 als_uuid = BT_UUID_INIT_128(
	0x14, 0x54, 0xd7, 0x54, 0xff, 0xec, 0x4c, 0x07,
	0x88, 0x85, 0x78, 0x41, 0x00, 0x30, 0xf1, 0x01);

static struct bt_uuid_128 als_al1_en_uuid = BT_UUID_INIT_128(
	0x14, 0x54, 0xd7, 0x54, 0xff, 0xec, 0x4c, 0x07,
	0x88, 0x85, 0x78, 0x41, 0x01, 0x30, 0xf1, 0x01);

static struct bt_uuid_128 als_al1_time_uuid = BT_UUID_INIT_128(
	0x14, 0x54, 0xd7, 0x54, 0xff, 0xec, 0x4c, 0x07,
	0x88, 0x85, 0x78, 0x41, 0x03, 0x30, 0xf1, 0x01);

static struct bt_uuid_128 als_al2_en_uuid = BT_UUID_INIT_128(
	0x14, 0x54, 0xd7, 0x54, 0xff, 0xec, 0x4c, 0x07,
	0x88, 0x85, 0x78, 0x41, 0x05, 0x30, 0xf1, 0x01);

static struct bt_uuid_128 als_al2_time_uuid = BT_UUID_INIT_128(
	0x14, 0x54, 0xd7, 0x54, 0xff, 0xec, 0x4c, 0x07,
	0x88, 0x85, 0x78, 0x41, 0x07, 0x30, 0xf1, 0x01);

static u8_t als_al1_en_value = 0;
static u8_t als_al1_time_value[3] = {0x00, 0x00, 0x00};
static u8_t als_al2_en_value = 0;
static u8_t als_al2_time_value[3] = {0x00, 0x00, 0x00};

static u8_t als_al1_en_cud[] = {'A', 'l', 'a', 'r', 'm', ' ', '1', ':', ' ',
								'E', 'n', 'a', 'b', 'l', 'e', '\0'};
static u8_t als_al1_time_cud[] = {'A', 'l', 'a', 'r', 'm', ' ', '1', ':', ' ',
								'T', 'i', 'm', 'e', '\0'};
static u8_t als_al2_en_cud[] = {'A', 'l', 'a', 'r', 'm', ' ', '2', ':', ' ',
								'E', 'n', 'a', 'b', 'l', 'e', '\0'};
static u8_t als_al2_time_cud[] = {'A', 'l', 'a', 'r', 'm', ' ', '2', ':', ' ',
								'T', 'i', 'm', 'e', '\0'};

static ssize_t read_als_al1_en(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			   void *buf, u16_t len, u16_t offset)
{
	const char *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(als_al1_en_value));
}

static ssize_t write_als_al1_en(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			    const void *buf, u16_t len, u16_t offset,
			    u8_t flags)
{
	u8_t *value = attr->user_data;

	if (offset + len > sizeof(als_al1_en_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf, len);

	return len;
}

static ssize_t read_als_al1_time(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			   void *buf, u16_t len, u16_t offset)
{
	const char *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(als_al1_time_value));
}

static ssize_t write_als_al1_time(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			    const void *buf, u16_t len, u16_t offset,
			    u8_t flags)
{
	u8_t *value = attr->user_data;

	if (offset + len > sizeof(als_al1_time_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf, len);

	return len;
}

static ssize_t read_als_al2_en(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			   void *buf, u16_t len, u16_t offset)
{
	const char *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(als_al2_en_value));
}

static ssize_t write_als_al2_en(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			    const void *buf, u16_t len, u16_t offset,
			    u8_t flags)
{
	u8_t *value = attr->user_data;

	if (offset + len > sizeof(als_al2_en_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf, len);

	return len;
}

static ssize_t read_als_al2_time(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			   void *buf, u16_t len, u16_t offset)
{
	const char *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(als_al2_time_value));
}

static ssize_t write_als_al2_time(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			    const void *buf, u16_t len, u16_t offset,
			    u8_t flags)
{
	u8_t *value = attr->user_data;

	if (offset + len > sizeof(als_al2_time_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf, len);

	return len;
}

/* Vendor Primary Service Declaration */
BT_GATT_SERVICE_DEFINE(als_svc,
	BT_GATT_PRIMARY_SERVICE(&als_uuid),
	BT_GATT_CHARACTERISTIC(&als_al1_en_uuid.uuid, BT_GATT_CHRC_READ |
			       BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			       read_als_al1_en, write_als_al1_en, &als_al1_en_value),
	BT_GATT_CUD(als_al1_en_cud, BT_GATT_PERM_READ),
	BT_GATT_CHARACTERISTIC(&als_al1_time_uuid.uuid, BT_GATT_CHRC_READ |
			       BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			       read_als_al1_time, write_als_al1_time, &als_al1_time_value),
	BT_GATT_CUD(als_al1_time_cud, BT_GATT_PERM_READ),
	BT_GATT_CHARACTERISTIC(&als_al2_en_uuid.uuid, BT_GATT_CHRC_READ |
			       BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			       read_als_al2_en, write_als_al2_en, &als_al2_en_value),
	BT_GATT_CUD(als_al2_en_cud, BT_GATT_PERM_READ),
	BT_GATT_CHARACTERISTIC(&als_al2_time_uuid.uuid, BT_GATT_CHRC_READ |
			       BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			       read_als_al2_time, write_als_al2_time, &als_al2_time_value),
	BT_GATT_CUD(als_al2_time_cud, BT_GATT_PERM_READ),
);

void als_init(void) {
	/*Enable interrupt on alarm 1 and alarm 2*/
	ds_modify_reg(REG_CONTROL, 0x3, 0x3);

	//More initializations
}

