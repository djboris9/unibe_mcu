#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include "comm.h"

LOG_MODULE_DECLARE(unibe_mcu, CONFIG_LOG_DEFAULT_LEVEL);

const struct device *const dev_usart = DEVICE_DT_GET(DT_NODELABEL(usart3));

// DMA buffer length
#define BUFLEN 128

// Line scanner buffer length
#define LINEBUFLEN 128
uint8_t linebuf[LINEBUFLEN];
int linebuf_idx = 0;

static void uart_callback(const struct device *dev, struct uart_event *evt, void *user_data) {
    switch (evt->type) {
        case UART_RX_RDY:
            LOG_DBG("UART RX ready. offset=%d len=%d", evt->data.rx.offset, evt->data.rx.len);

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
            LOG_DBG("UART TX done");
            break;
        case UART_RX_BUF_REQUEST:
            LOG_DBG("UART RX buffer request");
            // Respond to buffer request
            char* buf = k_malloc(BUFLEN);
            int ret = uart_rx_buf_rsp(dev, buf, BUFLEN);
            if (ret) {
                LOG_ERR("Cannot respond to buffer request: %d", ret);
            }
            break;
        case UART_RX_BUF_RELEASED:
            LOG_DBG("UART RX buffer released");
            k_free(evt->data.rx_buf.buf);
            break;
        case UART_RX_STOPPED:
            LOG_ERR("%s", "UART RX stopped");
            break;
        case UART_TX_ABORTED:
            LOG_ERR("%s", "UART TX aborted");
            break;
        default:
            LOG_ERR("%s", "UART unknown event");
            break;
    }
}

int gps_init(struct k_fifo *result_fifo) {
    int ret;

    // Make device ready
    if (!device_is_ready(dev_usart)) {
        LOG_ERR("Device %s is not ready; cannot connect\n", dev_usart->name);
        return -1;
    }

    // Set USART callback
    uart_callback_set(dev_usart, uart_callback, result_fifo);

    // Enable RX
    char* buf = k_malloc(BUFLEN);
    ret = uart_rx_enable(dev_usart, buf, BUFLEN, SYS_FOREVER_US);
    if (ret) {
        LOG_ERR("Cannot enable RX: %d\n", ret);
        return -1;
    }

    return 0;
}