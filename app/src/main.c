#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/display/cfb.h>
#include "c12832a1z_display.h"
#include "cfb_font_gcathin.h"
#include "comm.h"
#include "gps.h"

LOG_MODULE_REGISTER(unibe_mcu, CONFIG_LOG_DEFAULT_LEVEL);

K_FIFO_DEFINE(locsvc_fifo);

int main(void) {
	int ret;
	LOG_INF("Hello World! %s\n", CONFIG_BOARD);

	// Initialize GPS
	gps_init(&locsvc_fifo);

	// Read from locsvc_fifo
	struct locsvc_fifo_t *rx_data;
	while (1) {
		rx_data = k_fifo_get(&locsvc_fifo, K_FOREVER);
		//printf("Received for UART: '%.*s'\n", rx_data->data_len, rx_data->data);

		// Filter data that starts with '$GNGGA'
		if (rx_data->data_len >= 6 && strncmp(rx_data->data, "$GNGGA", 6) == 0) {
			printf("Received for GPS: '%.*s'\n", rx_data->data_len, rx_data->data);
		}

		k_free(rx_data->data);
		k_free(rx_data);
	}

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