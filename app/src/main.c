#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/display/cfb.h>
#include "display.h"
#include "cfb_font_gcathin.h"

LOG_MODULE_REGISTER(unibe_mcu, CONFIG_LOG_DEFAULT_LEVEL);

int main(void) {
	int ret;
	LOG_INF("Hello World! %s\n", CONFIG_BOARD);

	// Configure periphery
	InitDisplay();

	struct device *display_dev = c12832a1z_device(); 
	if (display_dev == NULL) {
		LOG_ERR("%s\n", "Error: Failed to get display device");
		return -1;
	}

	// Initialize CFB
	ret = cfb_framebuffer_init(display_dev);
	if (ret) {
		LOG_ERR("Error %d: Failed to initialize CFB\n", ret);
		return ret;
	}

	// Set font
	ret = cfb_framebuffer_set_font(display_dev, 0);
	if (ret) {
		LOG_ERR("Error %d: Failed to set font\n", ret);
		return ret;
	}

	// Clear screen
	ret = cfb_framebuffer_clear(display_dev, true);
	if (ret) {
		LOG_ERR("Error %d: Failed to clear display\n", ret);
		return ret;
	}

	// Print hello world
	ret = cfb_print(display_dev, "Hello World!", 0, 0);
	if (ret) {
		LOG_ERR("Error %d: Failed to print to display\n", ret);
		return ret;
	}

	// Finalize
	ret = cfb_framebuffer_finalize(display_dev);
	if (ret) {
		LOG_ERR("Error %d: Failed to finalize CFB\n", ret);
		return ret;
	}

	return 0;
}