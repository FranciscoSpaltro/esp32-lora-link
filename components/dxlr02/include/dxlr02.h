#ifndef DXLR02_H
#define DXLR02_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "esp_timer.h"


// --- CONFIGURACIÓN DEBUG (UART 1 REMAPEADA) ---
// NO USAR PINES 9 o 10 (Crashea la flash)
// NO USAR PINES 15 (Tu sensor)
#define DEBUG_PORT      UART_NUM_1  
#define DEBUG_TX_PIN    4   // <--- Conectá el Analizador/RX acá
#define DEBUG_RX_PIN    5   // <--- No es necesario si solo enviás logs
#define DEBUG_BAUD      115200

void debug_print(const char* str);

#define MAX_BUFFER_LEN 50
#define TIMEOUT_ONE_BYTE_MS 500
#define TIMEOUT_READ_US 10000


typedef enum {
    DXLR02_OK = 0,
    DXLR02_ERR_NOT_INITIALIZED,
    DXLR02_ERR_TIMEOUT,
    DXLR02_ERR_INVALID_RESPONSE,
    DXLR02_ERR_UART,
    DXLR02_ERR_MODULE_NOT_RESPONDING,
    DXLR02_ERR_INVALID_PARAMETER,
    DXLR02_ERR_ALREADY_INIT,
    DXLR02_ERR_OUT_OF_SPACE,
    DXLR02_ERR_COUNT                        // do not use
} dxlr02_status_t;

typedef struct {
    uint8_t working_mode;   // 0 (transparent), 1 (fixed-point), 2 (broadcast)
    uint8_t energy_mode;    // 0 (sleep), 1 (over-the-air wake-up), 2 (high-efficiency)
    int baudrate;       // 1 (1200), 2 (2400), 3 (4800), 4 (9600), 5 (19200), 6 (38400), 7 (57600), 8 (115200), 9 (128000)
    uint8_t rate_level;     // 0-7
    uint8_t stop_bit;        // 0 (1 SB), 1 (2 SB)
    uint8_t parity;         // 0 (no validation), 1 (odd check), 2 (even check) 
    uint8_t channel;        // frequency band (0-19, 0A-1E)
    uint8_t address;        // 1 byte
    uint8_t transmit_power;  // 0-22 dB (integers)
    uint8_t rf_coding_rate; // 1 (4/5), 2 (4/6), 3 (4/7), 4 (4/8)
    uint8_t spread_factor;  // 5-12
    bool crc;               // on-off
    bool iq_signal_flip;    // on-off
} dxlr02_config_t;

typedef struct {
    uint8_t uart_port;
    bool initialized;
    dxlr02_config_t config;
    bool mode_AT;
} dxlr02_t;



dxlr02_status_t dxlr02_init(dxlr02_t * module, uint8_t port, int baudrate);
dxlr02_status_t dxlr02_ensure_data_mode(dxlr02_t* module);
dxlr02_status_t dxlr02_set_config(dxlr02_t * module, const dxlr02_config_t * conf);
dxlr02_status_t dxlr02_get_config(dxlr02_t * module, dxlr02_config_t * conf);

dxlr02_status_t dxlr02_send_data(dxlr02_t * module, const char * data, size_t size);
dxlr02_status_t dxlr02_receive_data(dxlr02_t * module, char * data, size_t max_size, size_t * eff_len);  // bytes leidos


#endif