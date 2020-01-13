#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <zephyr.h>

#include <drivers/i2c.h>
#include <device.h>

#include "DS3231.h"

#define I2C_DEV "I2C_0"
#define DS3231_I2C_ADDR	0x68

struct device *i2c_dev;

void ds_init(void) {
	i2c_dev = device_get_binding(I2C_DEV);
	if (!i2c_dev) {
		printk("I2C: Device driver %s not found.\n", I2C_DEV);
	}
}

void ds_write_time(u8_t ct_in[], u8_t ct_len) {
	int err = i2c_burst_write(i2c_dev, DS3231_I2C_ADDR, REG_SECONDS, ct_in, ct_len);
	if (err) {
		printk("Failed to burst write by i2c, err: %d", err);
	}
}

void ds_read_time(u8_t ct_out[], u8_t ct_len) {
	u8_t start_reg[1] = {REG_SECONDS};
	u8_t tmp_reg[1] = {0};
	int err = i2c_write_read(i2c_dev, DS3231_I2C_ADDR, start_reg, 1, tmp_reg, 1);
	if (err) {
		printk("Failed to read by i2c, err: %d", err);
	}
}

void ds_modify_reg(u8_t reg_addr, u8_t mask, u8_t value) {
	int err = i2c_reg_update_byte(i2c_dev, DS3231_I2C_ADDR, reg_addr,
						mask, value);
	if (err) {
		printk("Failed to modify bit by i2c, err: %d", err);
	}
}