#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/display/cfb.h>
#include "c12832a1z_display.h"
#include "cfb_font_gcathin.h"
#include "comm.h"
#include "gps.h"
#include "magneto.h"

LOG_MODULE_REGISTER(unibe_mcu, CONFIG_LOG_DEFAULT_LEVEL);

// FIFO for location services data
K_FIFO_DEFINE(locsvc_fifo);

// Variables for display
struct device *display_dev;
char disp_gps_lat[20];
char disp_gps_lng[20];
char disp_mag[20];

void updateScreen(void) {
	int ret;
	// Clear screen
	ret = cfb_framebuffer_clear(display_dev, false);
	if (ret) {
		LOG_ERR("Error %d: Failed to clear display\n", ret);
		return;
	}

	// Print latitude to screen
	ret = cfb_print(display_dev, disp_gps_lat, 0, 0);
	if (ret) {
		LOG_ERR("Error %d: Failed to print to display\n", ret);
		return;
	}

	// Print longitude to screen
	ret = cfb_print(display_dev, disp_gps_lng, 0, 10);
	if (ret) {
		LOG_ERR("Error %d: Failed to print to display\n", ret);
		return;
	}

	// Print heading to screen
	ret = cfb_print(display_dev, disp_mag, 0, 20);
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

// process_gps_data processes the GPS data and sets the display strings.
// Returns true if the display strings have changed.
static bool process_gps_data(struct locsvc_fifo_t *rx_data) {
	// Filter data that starts with '$GNGGA'
	if (rx_data->data_len >= 6 && strncmp(rx_data->data, "$GNGGA", 6) == 0) {
		LOG_DBG("Received for GPS: '%.*s'\n", rx_data->data_len, (char*)rx_data->data);
		// Example: $GNGGA,114529.000,4656.2592,N,00725.8373,E,2,09,1.23,563.4,M,48.0,M,,*78
		//                 time       lat         lon          Q #s  HDOP  alt   sep  age  ref

		float time, lat, lng;
		char lat_c, lng_c;
		int q, sat;
		int ret = sscanf(rx_data->data, "$GNGGA,%f,%f,%c,%f,%c,%d,%d", &time, &lat, &lat_c, &lng, &lng_c, &q, &sat);
		if (ret != 7) {
			// Set disp_gps value to "unknown"
			snprintf(disp_gps_lat, 20, "unknown lat");
			snprintf(disp_gps_lng, 20, "unknown lng");

			return true;
		}

		// Reformat coordinates and set display string
		// We use only degrees and minutes, no DMS format
		int lat_deg = lat/100;
		lat -= lat_deg*100;
		int lng_deg = lng/100;
		lng -= lng_deg*100;

		snprintf(disp_gps_lat, 20, "%2d %.3f' %c", lat_deg, lat, lat_c);
		snprintf(disp_gps_lng, 20, "%2d %.3f' %c", lng_deg, lng, lng_c);

		return true;
	}

	return false;
}

// process_mag_data processes the magnetometer data and sets the display strings.
// Returns true if the display strings have changed.
static bool process_mag_data(struct locsvc_fifo_t *rx_data) {
	// Read heading as integer from FIFO
	int heading = *(int*)rx_data->data;
	LOG_DBG("Received for magnetometer: %d\n", heading);

	// Set display string
	snprintf(disp_mag, 20, "heading %d deg", heading);

	return true;
}


int main(void) {
	int ret;
	LOG_INF("Hello BME on %s\n", CONFIG_BOARD);

	// Initialize GPS
	gps_init(&locsvc_fifo);

	// Initialize magnetometer
	magneto_init();
	
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

	// Set font for CFB
	ret = cfb_framebuffer_set_font(display_dev, 0);
	if (ret) {
		LOG_ERR("Error %d: Failed to set font\n", ret);
		return ret;
	}

	// Initial screen
	updateScreen();

	// Start magnetometer
	magneto_start(&locsvc_fifo);

	// Read from locsvc_fifo
	struct locsvc_fifo_t *rx_data;
	while (1) {
		rx_data = k_fifo_get(&locsvc_fifo, K_FOREVER);
		bool changedDisplay = false;

		switch (rx_data->type) {
			case LOCSVC_FIFO_TYPE_MAG:
				changedDisplay |= process_mag_data(rx_data);
				break;
			case LOCSVC_FIFO_TYPE_GPS:
				changedDisplay |= process_gps_data(rx_data);
				break;
			default:
				LOG_ERR("Error: Unknown FIFO type: %d\n", rx_data->type);
				break;
		}

		// Free memory
		k_free(rx_data->data);
		k_free(rx_data);

		// Update screen
		if (changedDisplay) {
			updateScreen();
		}
	}

	return 0;
}