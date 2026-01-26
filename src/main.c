#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "dxlr02.h"


// --- CONFIGURACIÓN LO-RA (UART 2) ---
#define LORA_PORT       UART_NUM_2
#define LORA_TX_PIN     17
#define LORA_RX_PIN     16
#define LORA_BAUD       9600

static void uart_init_lora(void)
{
    uart_config_t cfg = {
        .baud_rate = LORA_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    uart_driver_install(LORA_PORT, 1024, 0, 0, NULL, 0);
    uart_param_config(LORA_PORT, &cfg);
    uart_set_pin(LORA_PORT, LORA_TX_PIN, LORA_RX_PIN,
                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    uart_flush_input(LORA_PORT);
}

static void uart_init_debug(void)
{
    const uart_config_t cfg = {
        .baud_rate = DEBUG_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // Instalamos driver para UART1
    uart_driver_install(DEBUG_PORT, 1024, 0, 0, NULL, 0);
    uart_param_config(DEBUG_PORT, &cfg);
    
    // IMPORTANTE: Remapeamos los pines para no tocar la Flash
    uart_set_pin(DEBUG_PORT, DEBUG_TX_PIN, DEBUG_RX_PIN, 
                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    uart_flush_input(DEBUG_PORT);
}

void app_main(void)
{
    // 1. Iniciamos Debug en UART1 (GPIO 4)
    //uart_init_debug();
    //vTaskDelay(pdMS_TO_TICKS(1000));
    

// Al principio de app_main
    gpio_reset_pin(21);
    gpio_set_direction(21, GPIO_MODE_OUTPUT);
    vTaskDelay(pdMS_TO_TICKS(1000));
    // 2. Iniciamos LoRa en UART2
    uart_init_lora();

    dxlr02_t mod = {0};



    if (dxlr02_init(&mod, LORA_PORT, LORA_BAUD) != DXLR02_OK) {
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    
    
    // Cambiar a Data Mode explícitamente por si acaso
    dxlr02_ensure_data_mode(&mod);
    
    //int i = 0;
    //char buf[32];

    while (1) {
        // Enviar por LoRa
        dxlr02_send_data(&mod, "hola", 5); 
        
        // Reportar por Debug
        //snprintf(buf, sizeof(buf), "Loop %d: Enviado 'hola'", i++);
        //debug_print(buf);

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}