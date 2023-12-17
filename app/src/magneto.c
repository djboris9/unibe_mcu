#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <math.h>
#include "comm.h"

LOG_MODULE_DECLARE(unibe_mcu, CONFIG_LOG_DEFAULT_LEVEL);

const struct device *const dev_i2c = DEVICE_DT_GET(DT_NODELABEL(i2c1));

static uint8_t curBank;

// slectBank selects the bank of the ICM-20948
static void selectBank(uint8_t bank) {    
    if (bank == curBank) {
        return;
    }

    int ret = i2c_reg_write_byte(dev_i2c, 0x69, 0x7F, bank<<4);
    if (ret) {
        LOG_ERR("Error %d: Failed to select bank\n", ret);
    }

    curBank = bank;
}

// magneto_read reads the magnetometer data from the AK09916
int magneto_read(struct k_fifo *result_fifo) {
    int ret;

    // Read magnetometer data
    selectBank(3);

    uint8_t mag_data_reg = 0x11;
    uint8_t mag_data[6] = {0};
    ret = i2c_write_read(dev_i2c, 0x0c, &mag_data_reg, 1, mag_data, 6);
    if (ret) {
        LOG_ERR("Error %d: Failed to read magnetometer data\n", ret);
        return -1;
    }

    // Read ST2 register to end measurement
    mag_data_reg = 0x18;
    uint8_t tmp;
    ret = i2c_write_read(dev_i2c, 0x0c, &mag_data_reg, 1, &tmp, 1);
    if (ret) {
        LOG_ERR("Error %d: Failed to read ST2 register\n", ret);
        return -1;
    }

    // Convert magnetometer data to int16_t
    int16_t mag_x = (mag_data[1] << 8) | mag_data[0];
    int16_t mag_y = (mag_data[3] << 8) | mag_data[2];
    int16_t mag_z = (mag_data[5] << 8) | mag_data[4];

    // Convert magnetometer data to float and scale to uT
    float mag_x_f = (float)mag_x * 0.15f;
    float mag_y_f = (float)mag_y * 0.15f;
    float mag_z_f = (float)mag_z * 0.15f;

    // Send magnetometer data to the result FIFO
    printf("magneto: %f %f %f\n", mag_x_f, mag_y_f, mag_z_f);

    // Convert data to compass heading in degrees
    // Reference: https://cdn-shop.adafruit.com/datasheets/AN203_Compass_Heading_Using_Magnetometers.pdf
    float heading;
    if (mag_y_f > 0) {
        heading = 90.0f - atanf(mag_x_f / mag_y_f) * 180.0f / 3.14159265359;
    } else if (mag_y_f < 0) {
        heading = 270.0f - atanf(mag_x_f / mag_y_f) * 180.0f / 3.14159265359;
    } else if (mag_x_f < 0) { // y == 0
        heading = 180.0f;
    } else { // y == 0
        heading = 0.0f;
    }

    printf("heading: %f\n", heading);

    // Send heading to the result FIFO
    int *heading_int = k_malloc(sizeof(int));
    *heading_int = (int)heading;

    struct locsvc_fifo_t *tx_data = k_malloc(sizeof(struct locsvc_fifo_t));
    tx_data->type = LOCSVC_FIFO_TYPE_MAG;
    tx_data->data_len = sizeof(int);
    tx_data->data = heading_int;

    k_fifo_put(result_fifo, tx_data);

    return 0;
}

int magneto_init() {
    int ret;

    // Make device ready
    if (!device_is_ready(dev_i2c)) {
        printk("Device %s is not ready; cannot connect\n", dev_i2c->name);
        return -1;
    }
    
    // Read WhoAmI register of the ICM-20948
    selectBank(0);

    uint8_t whoami_reg = 0x00;
    uint8_t whoami_data = 0x00;
    ret = i2c_write_read(dev_i2c, 0x69, &whoami_reg, 1, &whoami_data, 1);
    if (ret) {
        LOG_ERR("Error %d: Failed to read WhoAmI register\n", ret);
        return -1;
    }

    // Check if ICM-20948 is already bypassed, this is just heuristics
    if (whoami_data == 0x00) {
        LOG_INF("%s", "ICM-20948 is already bypassed\n");
    } else {
        // Check if WhoAmI register is correct
        if (whoami_data != 0xEA) {
            LOG_ERR("Error: Incorrect WhoAmI ICM-20948 register value: %d\n", whoami_data);
            return -1;
        }

        // Reset device, this is crucial for the magnetometer to work
        ret = i2c_reg_write_byte(dev_i2c, 0x69, 0x06, 0x80); // PWR_MGMT_1 (DEVICE_RESET = 1)
        if (ret) {
            LOG_ERR("Error %d: Failed to reset ICM-20948\n", ret);
            return -1;
        }

        // Wait for device to reset
        k_msleep(1);

        // Wakeup device and enable I2C bypass so we can directly access the magnetometer
        ret = i2c_reg_write_byte(dev_i2c, 0x69, 0x06, 0x01); // PWR_MGMT_1 (CLKSEL = 1, SLEEP = 0)
        if (ret) {
            LOG_ERR("Error %d: Failed to wake up ICM-20948\n", ret);
            return -1;
        }

        ret = i2c_reg_write_byte(dev_i2c, 0x69, 0x0F, 0x02); // INT_PIN_CFG (BYPASS_EN = 1)
        if (ret) {
            LOG_ERR("Error %d: Failed to enable bypass of ICM-20948\n", ret);
            return -1;
        }
    }

    // Read magnetometer WhoAmI register
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

    // Set magnetometer to continuous measurement mode 1 (10Hz)
    ret = i2c_reg_write_byte(dev_i2c, 0x0c, 0x31, 0x02); // CNTL2 (MODE = 1)
    if (ret) {
        LOG_ERR("Error %d: Failed to set magnetometer to continuous measurement mode 1\n", ret);
        return -1;
    }

    // Wait for magnetometer to start up
    k_msleep(50);
    
    return 0;
}

// Thread for magnetometer reading. This could have been also
// implemented using a timer and a work queue.

// 500 byte stack area
K_THREAD_STACK_DEFINE(mag_thread_stack_area, 500);
struct k_thread mag_thread_data;

static void mag_thread_entry_point(void *p1, void *, void *) {
    // Get FIFO
    struct k_fifo *result_fifo = (struct k_fifo *)p1;

    while (1) {
        magneto_read(result_fifo);
        k_msleep(300);
    }
}

// magneto_start starts the magnetometer thread
void magneto_start(struct k_fifo *result_fifo) {
    k_tid_t mag_thread = k_thread_create(&mag_thread_data, mag_thread_stack_area,
        K_THREAD_STACK_SIZEOF(mag_thread_stack_area),
        mag_thread_entry_point,
        result_fifo, NULL, NULL, 0, 0, K_NO_WAIT);
    
    k_thread_name_set(mag_thread, "magneto");
}