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

#include <device.h>
#include <drivers/pwm.h>

#include "lcs.h"

/* get the defines from dt (based on alias 'pwm-led0') */
#define PWM_DRIVER	DT_ALIAS_PWM_LED0_PWMS_CONTROLLER
#define PWM_CHANNEL	DT_ALIAS_PWM_LED0_PWMS_CHANNEL

/* Servo specifications, in microseconds*/
#define MIN_PULSE	17.5 * USEC_PER_MSEC
#define MAX_PULSE	19.5 * USEC_PER_MSEC
#define PERIOD	20U * USEC_PER_MSEC

/* Custom Service Variables */
static struct bt_uuid_128 lcs_uuid = BT_UUID_INIT_128(
	0x36, 0xa4, 0x21, 0xa3, 0xa5, 0xd4, 0x4a, 0xdc,
    0x8e, 0x4a, 0xf6, 0x5d, 0x00, 0xf7, 0x3c, 0x67);

static struct bt_uuid_128 lcs_pow_uuid = BT_UUID_INIT_128(
	0x36, 0xa4, 0x21, 0xa3, 0xa5, 0xd4, 0x4a, 0xdc,
    0x8e, 0x4a, 0xf6, 0x5d, 0x01, 0xf7, 0x3c, 0x67);

static struct bt_uuid_128 lcs_dim_uuid = BT_UUID_INIT_128(
	0x36, 0xa4, 0x21, 0xa3, 0xa5, 0xd4, 0x4a, 0xdc,
    0x8e, 0x4a, 0xf6, 0x5d, 0x03, 0xf7, 0x3c, 0x67);

static u8_t lcs_pow_value_update = 0;
static u8_t lcs_dim_value_update = 0;

static u8_t lcs_pow_value = 0x00;
static u8_t lcs_dim_value = 0x00;

static u8_t lcs_pow_cud[] = {'P', 'o', 'w', 'e', 'r', '\0'};
static u8_t lcs_dim_cud[] = {'D', 'i', 'm', 'm', 'e', 'r', '\0'};

static ssize_t read_lcs_pow(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			   void *buf, u16_t len, u16_t offset)
{
	const char *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(lcs_pow_value));
}

static ssize_t write_lcs_pow(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			    const void *buf, u16_t len, u16_t offset,
			    u8_t flags)
{
	u8_t *value = attr->user_data;

	if (offset + len > sizeof(lcs_pow_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf, len);

	lcs_pow_value_update = 1;

	return len;
}

static ssize_t read_lcs_dim(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			   void *buf, u16_t len, u16_t offset)
{
	const char *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(lcs_dim_value));
}

static ssize_t write_lcs_dim(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			    const void *buf, u16_t len, u16_t offset,
			    u8_t flags)
{
	u8_t *value = attr->user_data;

	if (offset + len > sizeof(lcs_dim_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	lcs_dim_value_update = 1;

	memcpy(value + offset, buf, len);

	return len;
}

/* Vendor Primary Service Declaration */
BT_GATT_SERVICE_DEFINE(lcs_svc,
	BT_GATT_PRIMARY_SERVICE(&lcs_uuid),
	BT_GATT_CHARACTERISTIC(&lcs_pow_uuid.uuid, BT_GATT_CHRC_READ |
			       BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			       read_lcs_pow, write_lcs_pow, &lcs_pow_value),
	BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_CUD(lcs_pow_cud, BT_GATT_PERM_READ),
	BT_GATT_CHARACTERISTIC(&lcs_dim_uuid.uuid, BT_GATT_CHRC_READ |
			       BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			       read_lcs_dim, write_lcs_dim, &lcs_dim_value),
	BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_CUD(lcs_dim_cud, BT_GATT_PERM_READ),
);

u32_t angle_to_pulse(u8_t angle) {
	u32_t pulse = 17500U + 11U * angle;
	if (pulse < MIN_PULSE) pulse = MIN_PULSE;
	if (pulse > MAX_PULSE) pulse = MAX_PULSE;
	return pulse;
}

void lcs_set_dim(u8_t dim_angle) {
	struct device *pwm_dev;
	u32_t period = PERIOD;

	pwm_dev = device_get_binding(PWM_DRIVER);
	if (!pwm_dev) {
		printk("Cannot find %s!\n", PWM_DRIVER);
		return;
	}

	int err = pwm_pin_set_usec(pwm_dev, PWM_CHANNEL, period, angle_to_pulse(lcs_dim_value));
	if (err) {
		printk("pwm pin set fails, err: %d\n", err);
		return;
	}
}

void lcs_pow_notify(void) {
    if (!lcs_pow_value_update) {
        return;
    }

    lcs_pow_value_update = 0;

    bt_gatt_notify(NULL, &lcs_svc.attrs[1], &lcs_pow_value, sizeof(lcs_pow_value));
}

void lcs_dim_notify(void) {
    if (!lcs_dim_value_update) {
        return;
    }

    lcs_dim_value_update = 0;

	lcs_set_dim(lcs_dim_value);

    bt_gatt_notify(NULL, &lcs_svc.attrs[4], &lcs_dim_value, sizeof(lcs_dim_value));
}