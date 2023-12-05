#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>

const struct device *const dev_usart = DEVICE_DT_GET(DT_NODELABEL(usart3));

#define BUFLEN 128
uint8_t buf[2][BUFLEN];
int curbuf = 0;

static void uart_callback(const struct device *dev, struct uart_event *evt, void *user_data) {
    switch (evt->type) {
        case UART_RX_RDY:
            printf("UART RX ready\n");
            break;
        case UART_TX_DONE:
            printf("UART TX done\n");
            break;
        case UART_RX_BUF_REQUEST:
            printf("UART RX buffer request\n");
            // Rollover buffer
            curbuf = (curbuf + 1) % 2;

            // Respond to buffer request
            int ret = uart_rx_buf_rsp(dev, buf[curbuf], BUFLEN);
            if (ret) {
                printf("Cannot respond to buffer request: %d\n", ret);
            }
            break;
        case UART_RX_BUF_RELEASED:
            printf("UART RX buffer released\n");
            printf("Received: %s\n", buf[curbuf]);
            break;
        case UART_RX_STOPPED:
            printf("UART RX stopped\n");
            break;
        case UART_TX_ABORTED:
            printf("UART TX aborted\n");
            break;
        default:
            printf("UART unknown event\n");
            break;
    }
    printf("UART callback\n");
}

int gps_init(void) {
    int ret;

    // Make device ready
    if (!device_is_ready(dev_usart)) {
        printk("Device %s is not ready; cannot connect\n", dev_usart->name);
        return -1;
    }

    // Set USART callback
    uart_callback_set(dev_usart, uart_callback, NULL);

    // Enable RX
    ret = uart_rx_enable(dev_usart, buf[curbuf], BUFLEN, SYS_FOREVER_US);
    if (ret) {
        printk("Cannot enable RX: %d\n", ret);
        return -1;
    }


    return 0;
}