#ifndef I2CTEST_H
#define I2CTEST_H

#include <stdint.h>
#include <driver/i2c_master.h>

#define I2C_SCL_PIN 1
#define I2C_SDA_PIN 0
#define I2C_FREQ_HZ 400000
#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_TIMEOUT_MS 1000

#define I2C_OK 0
#define I2C_ERR_ADDR_NACK 1
#define I2C_ERR_REG_NACK 2
#define I2C_ERR_DATA_NACK 3
#define I2C_ERR_READ_ADDR_NACK 4

void i2c_master_init_bus(i2c_master_bus_handle_t *bus_handle);
void i2c_master_init_handle(i2c_master_bus_handle_t *bus_handle, i2c_master_dev_handle_t *dev_handle, uint8_t slave_address);
esp_err_t i2c_read_byte(i2c_master_dev_handle_t dev_handle, uint8_t reg_addr, uint8_t *data, size_t len);
esp_err_t i2c_write_byte(i2c_master_dev_handle_t dev_handle, uint8_t reg_addr, uint8_t data);
esp_err_t i2c_write(i2c_master_dev_handle_t dev_handle, uint8_t len, uint8_t adr, uint8_t *buf);
esp_err_t i2c_read(i2c_master_dev_handle_t dev_handle, uint8_t len, uint8_t adr, uint8_t *buf);
#endif