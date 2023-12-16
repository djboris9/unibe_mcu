#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include "comm.h"

LOG_MODULE_DECLARE(unibe_mcu, CONFIG_LOG_DEFAULT_LEVEL);

const struct device *const dev_i2c = DEVICE_DT_GET(DT_NODELABEL(i2c1));



/*
static void icmMasterEnable() {
    selectBank(3);

    // Configure Clock of I2C_MST_CTRL
    uint8_t reg = 0x01;
    uint8_t data = 0x07; // Clock
    int ret = i2c_write(dev_i2c, &data, 1, 0x69);
    if (ret) {
        LOG_ERR("Error %d: Failed to enable I2C master\n", ret);
    }

    // Configure USER_CTRL to enable master
    selectBank(0);
    reg = 0x03;
    data = 0x20; // I2C_MST_EN
    ret = i2c_write(dev_i2c, &data, 1, 0x69);
    if (ret) {
        LOG_ERR("Error %d: Failed to enable I2C master\n", ret);
    }
}

static void resetICM() {
    selectBank(0);

    uint8_t reg = 0x06;
    uint8_t data = 0x80;
    int ret = i2c_write(dev_i2c, &data, 1, 0x69);
    if (ret) {
        LOG_ERR("Error %d: Failed to reset ICM\n", ret);
    }
}
*/

static uint8_t curBank;

static void selectBank(uint8_t bank) {    
    if (bank == curBank) {
        return;
    }

    uint8_t reg = 0x7f;
    uint8_t data = bank<<4;
    uint8_t write_data[2] = {reg, data};
    int ret = i2c_write(dev_i2c, write_data, 2, 0x69);
    if (ret) {
        LOG_ERR("Error %d: Failed to select bank\n", ret);
    }

    curBank = bank;
}

static void writeReg(uint8_t bank, uint8_t addr, uint8_t reg, uint8_t data) {
    selectBank(bank);

    uint8_t write_data[2] = {reg, data};
    int ret = i2c_write(dev_i2c, write_data, 2, addr);
    if (ret) {
        LOG_ERR("Error %d: Failed to write register\n", ret);
    }
}

int magneto_init(struct k_fifo *result_fifo) {
    int ret;

    // Make device ready
    if (!device_is_ready(dev_i2c)) {
        printk("Device %s is not ready; cannot connect\n", dev_i2c->name);
        return -1;
    }

    // Initialize magnetometer ICM-20948
    
    // Read WhoAmI register
    selectBank(0);

    uint8_t whoami_reg = 0x00;
    uint8_t whoami_data = 0x00;
    ret = i2c_write_read(dev_i2c, 0x69, &whoami_reg, 1, &whoami_data, 1);
    if (ret) {
        LOG_ERR("Error %d: Failed to read WhoAmI register\n", ret);
        return -1;
    }

    // Check if WhoAmI register is correct
    if (whoami_data != 0xEA) {
        LOG_ERR("Error: Incorrect WhoAmI register value: %d\n", whoami_data);
        return -1;
    }

    printf("WhoAmI ICM register value: 0x%02x\n", whoami_data);



    // reset
    writeReg(0, 0x69, 0x06, 0x80); // PWR_MGMT_1
    k_msleep(5);

    // Wakeup device
    writeReg(0, 0x69, 0x06, 0x01); // PWR_MGMT_1    
    k_msleep(5);
    
    writeReg(0, 0x69, 0x0f, 0x02); // INT_PIN_CFG
    k_msleep(5);


    // Read INT_PIN_CFG
    uint8_t int_pin_cfg_reg = 0x0f;
    uint8_t int_pin_cfg_data = 0x00;
    ret = i2c_write_read(dev_i2c, 0x69, &int_pin_cfg_reg, 1, &int_pin_cfg_data, 1);
    if (ret) {
        LOG_ERR("Error %d: Failed to read INT_PIN_CFG register\n", ret);
        return -1;
    }

    printf("INT_PIN_CFG register value: 0x%02x\n", int_pin_cfg_data);

/*
    // Request whoami
    writeReg(3, 0x69, 0x03, 0x0c | 0x80); // I2C_SLV0_ADDR -> AK09916 | READ
    writeReg(3, 0x69, 0x04, 0x00); // I2C_SLV0_REG -> Whoami
    writeReg(3, 0x69, 0x05, 0x80 | 1); // enable read | number of byte
    k_msleep(10);

    writeReg(3, 0x69, 0x03, 0x0c | 0x80); // I2C_SLV0_ADDR -> AK09916 | READ
    writeReg(3, 0x69, 0x11, 0x00); // I2C_SLV0_REG -> HXL
    writeReg(3, 0x69, 0x05, 0x80 | 8); // enable read | number of byte
    k_msleep(10);
*/
    // Read magnetometer WhoAmI register - AK09916
    //  selectBank(0);

    uint8_t mag_whoami_reg = 0x01;
    uint8_t mag_whoami_data = 0x00;
    ret = i2c_write_read(dev_i2c, 0x0c, &mag_whoami_reg, 1, &mag_whoami_data, 1);
    if (ret) {
        LOG_ERR("Error %d: Failed to read magnetometer WhoAmI register\n", ret);
        return -1;
    }

    // Check if magnetometer WhoAmI register is correct
    if (mag_whoami_data != 0x09) {
        LOG_ERR("Error: Incorrect magnetometer WhoAmI register value: %d\n", mag_whoami_data);
        return -1;
    }

    printf("WhoAmI AK register value: 0x%02x\n", mag_whoami_data);

    return 0;
}