#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_pdm.h"
#include "driver/uart.h"

#define AUDIO_FS 16000
#define UART_BAUD_RATE 921600
#define MIC_CLK_GPIO GPIO_NUM_5
#define MIC_DIN_GPIO GPIO_NUM_6
#define STEREO_SAMPLES_PER_READ 240

static i2s_chan_handle_t rx_handle;

static void initMic(void)
{
    i2s_chan_config_t chan_cfg = {
        .id = I2S_NUM_0,
        .role = I2S_ROLE_MASTER,
        .dma_desc_num = 6,
        .dma_frame_num = 240,
        .auto_clear = false,
        .intr_priority = 0,
    };

    i2s_pdm_rx_config_t pdm_rx_cfg = {
        .clk_cfg = I2S_PDM_RX_CLK_DEFAULT_CONFIG(AUDIO_FS),
        .gpio_cfg = {
            .clk = MIC_CLK_GPIO,
            .din = MIC_DIN_GPIO,
            .invert_flags = {
                .clk_inv = false,
            },
        },
        .slot_cfg = I2S_PDM_RX_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
    };

    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &rx_handle));
    ESP_ERROR_CHECK(i2s_channel_init_pdm_rx_mode(rx_handle, &pdm_rx_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(rx_handle));
}

static void initUart(void)
{
    const uart_config_t uart_cfg = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };

    ESP_ERROR_CHECK(uart_param_config(UART_NUM_0, &uart_cfg));
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, 1024, 1024, 0, NULL, 0));
}

static void uart2py_task(void *arg)
{
    int16_t stereo_buf[STEREO_SAMPLES_PER_READ * 2];
    size_t bytes_read = 0;

    (void)arg;

    while (1) {
        esp_err_t ret = i2s_channel_read(rx_handle, stereo_buf, sizeof(stereo_buf), &bytes_read, portMAX_DELAY);
        if (ret != ESP_OK || bytes_read == 0) {
            continue;
        }

        uart_write_bytes(UART_NUM_0, (const char *)stereo_buf, bytes_read);
    }
}

void app_main(void)
{
    BaseType_t task_created = pdFAIL;

    initMic();
    initUart();

    task_created = xTaskCreate(uart2py_task, "uart2py_task", 4096, NULL, 5, NULL);
    ESP_ERROR_CHECK(task_created == pdPASS ? ESP_OK : ESP_FAIL);
}
