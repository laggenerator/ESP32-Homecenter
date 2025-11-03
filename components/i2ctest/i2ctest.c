#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "i2ctest.h"
#include "string.h"

static const char* TAG = "I2C";

void i2c_master_init_bus(i2c_master_bus_handle_t *bus_handle){
    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = I2C_SCL_PIN,
        .sda_io_num = I2C_SDA_PIN,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, bus_handle));
    ESP_LOGI(TAG, "Zainicjalizowano I2C");
}

void i2c_master_init_handle(i2c_master_bus_handle_t *bus_handle, i2c_master_dev_handle_t *dev_handle, uint8_t slave_address){
    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = slave_address,
        .scl_speed_hz = I2C_FREQ_HZ
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(*bus_handle, &dev_config, dev_handle));
    ESP_LOGI(TAG, "Dodano urzadzenie 0x%02X", slave_address);
};


// Odczytywanie byte'a
esp_err_t i2c_read_byte(i2c_master_dev_handle_t dev_handle, uint8_t reg_addr, uint8_t *data, size_t len){
    return i2c_master_transmit_receive(dev_handle, &reg_addr, 1, data, len, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}
// Zapisywanie byte'a
esp_err_t i2c_write_byte(i2c_master_dev_handle_t dev_handle, uint8_t reg_addr, uint8_t data){
    uint8_t write_buf[2] = {reg_addr, data};
    return i2c_master_transmit(dev_handle, write_buf, sizeof(write_buf), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

// Wysylanie wiecej niz bajta
esp_err_t i2c_write(i2c_master_dev_handle_t dev_handle, uint8_t len, uint8_t adr, uint8_t *buf) {
    uint8_t *write_buf = malloc(len + 1);
    if (write_buf == NULL) {
        return ESP_ERR_NO_MEM;
    }
    
    write_buf[0] = adr;  // Adres rejestru
    memcpy(&write_buf[1], buf, len);  // Dane
    
    esp_err_t ret = i2c_master_transmit(dev_handle, write_buf, len + 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    
    free(write_buf);
    return ret;
}

// Odczytywanie wiecej niz bajta
esp_err_t i2c_read(i2c_master_dev_handle_t dev_handle, uint8_t len, uint8_t adr, uint8_t *buf) {
    esp_err_t ret;
    
    ret = i2c_master_transmit(dev_handle, &adr, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    if (ret != ESP_OK) {
        return ret;
    }
    
    ret = i2c_master_receive(dev_handle, buf, len, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    if (ret != ESP_OK) {
        return ret;
    }
    
    return ESP_OK;
}
