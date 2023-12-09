#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/display/cfb.h>
#include "c12832a1z_display.h"
#include "cfb_font_gcathin.h"
#include "comm.h"
#include "gps.h"

LOG_MODULE_REGISTER(unibe_mcu, CONFIG_LOG_DEFAULT_LEVEL);

K_FIFO_DEFINE(locsvc_fifo);

// Variables for display
struct device *display_dev;
char disp_gps[20];

void updateScreen(void) {
	// Print data to screen
	int ret = cfb_print(display_dev, disp_gps, 0, 0);
	if (ret) {
		LOG_ERR("Error %d: Failed to print to display\n", ret);
		return;
	}

	// Finalize
	ret = cfb_framebuffer_finalize(display_dev);
	if (ret) {
		LOG_ERR("Error %d: Failed to finalize CFB\n", ret);
		return;
	}
}

int main(void) {
	int ret;
	LOG_INF("Hello World! %s\n", CONFIG_BOARD);

	// Initialize GPS
	gps_init(&locsvc_fifo);

	// Initialize display
	display_dev = c12832a1z_device(); 
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

	// Read from locsvc_fifo
	struct locsvc_fifo_t *rx_data;
	while (1) {
		rx_data = k_fifo_get(&locsvc_fifo, K_FOREVER);

		// Filter data that starts with '$GNGGA'
		if (rx_data->data_len >= 6 && strncmp(rx_data->data, "$GNGGA", 6) == 0) {
			printf("Received for GPS: '%.*s'\n", rx_data->data_len, (char*)rx_data->data);
			// Example: $GNGGA,114529.000,4656.2592,N,00725.8373,E,2,09,1.23,563.4,M,48.0,M,,*78
			//                 time       lat         lon          Q #s  HDOP  alt   sep  age  ref

			float time, lat, lng;
			char lat_c, lng_c;
			int q, sat;
			ret = sscanf(rx_data->data, "$GNGGA,%f,%f,%c,%f,%c,%d,%d", &time, &lat, &lat_c, &lng, &lng_c, &q, &sat);
			if (ret != 7) {
				// Set disp_gps value to "unknown"
				snprintf(disp_gps, 20, "unknown");
				continue;
			}
			printf("Ret: %d\n", ret);
			printf("Time: %f\n", time);
			printf("Lat: %f %c\n", lat, lat_c);
			printf("Lng: %f %c\n", lng, lng_c);
			printf("Q: %d\n", q);
			printf("Sat: %d\n", sat);

			// Set display string
			snprintf(disp_gps, 20, "%.2f %c %.2f %c", lat, lat_c, lng, lng_c);
		}

		k_free(rx_data->data);
		k_free(rx_data);

		updateScreen();
	}

	return 0;
}