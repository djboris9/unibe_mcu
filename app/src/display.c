#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/devicetree/spi.h>
#include <zephyr/logging/log.h>
#include <stdlib.h>

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
	//setRandomDisplay();
	
	return 0;
}

static void c12832a1z_display_get_capabilities(const struct device *dev,
		struct display_capabilities *capabilities)
{
	const struct c12832a1z_display_config *config = dev->config;
	struct c12832a1z_display_data *disp_data = dev->data;

	memset(capabilities, 0, sizeof(struct display_capabilities));
	capabilities->x_resolution = 128;
	capabilities->y_resolution = 64;
	capabilities->supported_pixel_formats = PIXEL_FORMAT_MONO01 | PIXEL_FORMAT_MONO10;
	capabilities->screen_info = SCREEN_INFO_MONO_VTILED | SCREEN_INFO_MONO_MSB_FIRST;
}

static void NIY(void) {
	LOG_ERR("%s\n", "Not implemented yet");
}

static int c12832a1z_display_blanking_off(const struct device *dev)
{
	return 0; // TODO
}

static int c12832a1z_display_blanking_on(const struct device *dev)
{
	return 0; // TODO
}

static int c12832a1z_display_read(const struct device *dev, const uint16_t x,
			      const uint16_t y,
			      const struct display_buffer_descriptor *desc,
			      void *buf)
{
	return -ENOTSUP;
}

static void *c12832a1z_display_get_framebuffer(const struct device *dev)
{
	return NULL;
}

static int c12832a1z_NIY2(const struct device *dev, const uint8_t arg)
{
	return -ENOTSUP;
}

static int c12832a1z_display_set_pixel_format(const struct device *dev,
		const enum display_pixel_format pixel_format)
{

	return -ENOTSUP;
}

static int c12832a1z_display_write(const struct device *dev, const uint16_t x,
			       const uint16_t y,
			       const struct display_buffer_descriptor *desc,
			       const void *buf)
{
	const struct c12832a1z_display_config *config = dev->config;

	// print position
	printf("Writing %dx%d (w,h) @ %dx%d (x,y)\n", desc->width, desc->height, x, y);

	// Buffer for data
	for (int i = 0; i < 4; i++) {
		printf("page: %d\n", i);
		setRegisterMode(false); // Set to instruction mode
		sendInstruction(LCD_CMD_PAGE_0 + 3 - i); // Switch to page i, inverted
		sendInstruction(LCD_CMD_COL_0); // Column 0

		setRegisterMode(true); // Set to data mode

		// Shift address of buf by whole columns
		uint8_t data = 0xff;
		struct spi_buf txb;
		txb.buf = (uint8_t*)buf + (i*128);
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

	printf("done\n");

	return 0;
}

static const struct display_driver_api c12832a1z_display_api = {
	.blanking_on = c12832a1z_display_blanking_on,
	.blanking_off = c12832a1z_display_blanking_off,
	.write = c12832a1z_display_write,
	.read = c12832a1z_display_read,
	.get_framebuffer = c12832a1z_display_get_framebuffer,
	.set_brightness = c12832a1z_NIY2, //c12832a1z_display_set_brightness,
	.set_contrast = c12832a1z_NIY2, //c12832a1z_display_set_contrast,
	.get_capabilities = c12832a1z_display_get_capabilities,
	.set_pixel_format = c12832a1z_display_set_pixel_format,
};


// GetDevice returns the display device for use
// with the character framebuffer
struct device *c12832a1z_device(void) {
	struct device *dev = malloc(sizeof(*dev));
	dev->name = "C12832A1Z";
	dev->data = malloc((128/8) * (64/8)); // 1 Bit per pixel (PPT=1 here)
	dev->api = &c12832a1z_display_api;

	InitDisplay();
	
	return dev;
}
