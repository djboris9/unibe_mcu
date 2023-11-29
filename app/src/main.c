#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "display.h"

LOG_MODULE_REGISTER(unibe_mcu, CONFIG_LOG_DEFAULT_LEVEL);

int main(void) {
	LOG_INF("Hello World! %s\n", CONFIG_BOARD);

	// Configure periphery
	InitDisplay();

	return 0;
}