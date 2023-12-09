#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include <string.h>
#include "comm.h"

const struct device *const dev_usart = DEVICE_DT_GET(DT_NODELABEL(usart3));

#define BUFLEN 128
uint8_t buf[2][BUFLEN];
int curbuf = 0;

#define LINEBUFLEN 128
uint8_t linebuf[LINEBUFLEN];
int linebuf_idx = 0;

static void uart_callback(const struct device *dev, struct uart_event *evt, void *user_data) {
    switch (evt->type) {
        case UART_RX_RDY:
            printf("UART RX ready. offset=%d len=%d\n", evt->data.rx.offset, evt->data.rx.len);
            for (int i = 0; i < evt->data.rx.len; i++) {
                // Get current char
                char chr = evt->data.rx.buf[evt->data.rx.offset + i];
                
                // Handle line endings
                if (chr == '\r' || chr == '\n') {
                    linebuf[linebuf_idx] = chr;

                    // Process only non-zero lines
                    if (linebuf_idx > 0) {
                        char* line = k_malloc(linebuf_idx);
                        memcpy(line, linebuf, linebuf_idx);
                        struct locsvc_fifo_t *tx_data = k_malloc(sizeof(struct locsvc_fifo_t));
                        tx_data->type = LOCSVC_FIFO_TYPE_GPS;
                        tx_data->data_len = linebuf_idx;
                        tx_data->data = line;

                        k_fifo_put((struct k_fifo*) user_data, tx_data);
                        printf("sent\n");
                    }

                    linebuf_idx = 0;
                } else {
                    linebuf[linebuf_idx] = chr;
                    linebuf_idx++;
                }

                if (linebuf_idx >= LINEBUFLEN) {
                    linebuf_idx = 0;
                }
            }
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
            //printf("Received: %s\n", buf[curbuf]);
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
}

int gps_init(struct k_fifo *result_fifo) {
    int ret;

    // Make device ready
    if (!device_is_ready(dev_usart)) {
        printk("Device %s is not ready; cannot connect\n", dev_usart->name);
        return -1;
    }

    // Set USART callback
    uart_callback_set(dev_usart, uart_callback, result_fifo);

    // Enable RX
    ret = uart_rx_enable(dev_usart, buf[curbuf], BUFLEN, SYS_FOREVER_US);
    if (ret) {
        printk("Cannot enable RX: %d\n", ret);
        return -1;
    }


    return 0;
}