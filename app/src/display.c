#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/devicetree/spi.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(unibe_mcu, CONFIG_LOG_DEFAULT_LEVEL);

// Defines
#define LCD_CMD_PAGE_0 0xB0
#define LCD_CMD_COL_0 0x10

// Periphery
static const struct device *const gpioa_dev = DEVICE_DT_GET(DT_NODELABEL(gpioa));
static const struct device *const lcd_dev = DEVICE_DT_GET(DT_NODELABEL(arduino_spi));
static struct spi_cs_control spi_cs_ctrl = SPI_CS_CONTROL_INIT(DT_NODELABEL(arduino_spi), 2);
static struct spi_config spi_cfg;

// Manages A0 pin on display
static void setRegisterMode(bool data) {
	if (data) {
		gpio_pin_set(gpioa_dev, 8, 1);
	} else {
		gpio_pin_set(gpioa_dev, 8, 0);
	}
}

// Manages RST pin on display
static void resetDisplay() {
	gpio_pin_set(gpioa_dev, 6, 0);
	k_msleep(1);
	gpio_pin_set(gpioa_dev, 6, 1);
}

static int sendInstruction(uint8_t inst) {
	setRegisterMode(false); // Set to instruction mode

	// Buffer for instruction
	struct spi_buf txb;
	txb.buf = &inst;
	txb.len = 1U;

	// Buffer set for instruction
	struct spi_buf_set txbufs;
	txbufs.buffers = &txb;
	txbufs.count = 1U;
	
	// Send instruction
	int ret = spi_write(lcd_dev, &spi_cfg, &txbufs);
	if (ret) {
		LOG_ERR("Error %d: Failed to write to SPI device\n", ret);
		return ret;
	}

	return 0;
}

static void setRandomDisplay() {
	// Set random display data
	uint8_t data[128];
	for (int i = 0; i < 64; i++) {
		data[i] = 0xff;
	}

	// Send it to screen
	setRegisterMode(true); // Set to data mode

	// Buffer for data
	struct spi_buf txb;
	txb.buf = data;
	txb.len = 128U;

	// Buffer set for data
	struct spi_buf_set txbufs;
	txbufs.buffers = &txb;
	txbufs.count = 1U;

	// Send data
	int ret = spi_write(lcd_dev, &spi_cfg, &txbufs);
	if (ret) {
		LOG_ERR("Error %d: Failed to write to SPI device\n", ret);
	}
}

int InitDisplay(void) {
	// Configure GPIOs
	gpio_pin_configure(gpioa_dev, 6, GPIO_OUTPUT_LOW | DT_GPIO_FLAGS(DT_NODELABEL(gpio0), gpios));
	gpio_pin_configure(gpioa_dev, 8, GPIO_OUTPUT_LOW | DT_GPIO_FLAGS(DT_NODELABEL(gpio0), gpios));

	// Configure SPI port for display from devicetree
	spi_cfg.frequency = 400000; // 400kHz
	spi_cfg.slave = 0;
	spi_cfg.operation = SPI_OP_MODE_MASTER | SPI_HALF_DUPLEX  | SPI_TRANSFER_MSB | SPI_WORD_SET(8);
	spi_cfg.cs = spi_cs_ctrl;

	// Prepare display
	resetDisplay();
	setRegisterMode(false);

	// Initialize display
	sendInstruction(0xA0); // ADC_NORMAL
	sendInstruction(0xAE); // DISPLAY_OFF
	sendInstruction(0xC8); // COMMON_OUTPUT_MODE_REVERSE
	sendInstruction(0xA2); // BIAS_ONE_NINTH
	sendInstruction(0x2F); // POWER_CONTROL_SET_7
	sendInstruction(0x21); // INTERNAL_RESISTOR_RATIO_1
	sendInstruction(0xAF); // DISPLAY_ON

	// Set display to page 0, column 0 (this shouldn't be necessary)
	sendInstruction(LCD_CMD_PAGE_0); // Page 0
	sendInstruction(LCD_CMD_COL_0); // Column 0
	setRandomDisplay();
	
	return 0;
}