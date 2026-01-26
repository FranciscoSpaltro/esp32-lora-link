#include "dxlr02.h"
#include "driver/uart.h"
#include "string.h"
#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void debug(int a) {
    for(int i = 0; i < a; i++){
        gpio_set_level(21, 1); // Prende LED (Significa "Llegué a AT")
        vTaskDelay(pdMS_TO_TICKS(100));
        gpio_set_level(21, 0); // Apaga
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    vTaskDelay(pdMS_TO_TICKS(500));
}



/****************************************** AUXILIAR UART FUNCTIONS ******************************************/
static dxlr02_status_t dxlr02_uart_send(dxlr02_t *module, const char *data, size_t len) {
    if(!module || !module->initialized){
        return DXLR02_ERR_NOT_INITIALIZED;
    }

    int ret = uart_write_bytes((uart_port_t)module->uart_port, data, len);

    if (ret < 0 || ret != len) {
        return DXLR02_ERR_UART;
    }

    return DXLR02_OK;
}

static dxlr02_status_t dxlr02_read_until(dxlr02_t *module, char * response, size_t response_len, char delim, size_t times, uint32_t timeout_ms){
    if(!module || !module->initialized)
        return DXLR02_ERR_NOT_INITIALIZED;

    size_t i = 0, j = 0;
    response[0] = '\0';
    int64_t init_time = esp_timer_get_time();
    int64_t timeout_us = (int64_t)timeout_ms * 1000;
    bool delim_found = false;
    for(j = 0; j < times; j++){
        delim_found = false;
        
        while(i < response_len - 1){
            if(esp_timer_get_time() - init_time > timeout_us){
                return DXLR02_ERR_TIMEOUT;
            }

            int read = uart_read_bytes((uart_port_t) module -> uart_port, response + i, 1, pdMS_TO_TICKS(TIMEOUT_ONE_BYTE_MS));

            if(read < 0){
                return DXLR02_ERR_UART;
            }
            else if(read == 0){
                continue;
            }
            if(response[i] == delim){
                
                i++;
                delim_found = true;
                break;
            } 

            i++;
        }

        if(!delim_found){
            response[i] = '\0';
            return DXLR02_ERR_INVALID_RESPONSE;
        }
    }

    if(response[0] == '\0'){
        return DXLR02_ERR_INVALID_RESPONSE;
    }

    response[i] = '\0';
    if(j == times){
        return DXLR02_OK;
    }

    return DXLR02_ERR_INVALID_RESPONSE;
}

dxlr02_status_t dxlr02_send_cmd(dxlr02_t * module, const char * cmd){
    if(!module || !module->initialized){
        return DXLR02_ERR_NOT_INITIALIZED;
    }
    
    uart_flush_input((uart_port_t)module->uart_port);
    
    return dxlr02_uart_send(module, cmd, strlen(cmd)); 
}

/****************************************** AUXILIAR FUNCTIONS ******************************************/

static dxlr02_status_t dxlr02_AT_mode(dxlr02_t * module){ 
    dxlr02_status_t st; 
    char response[25]; 
    st = dxlr02_send_cmd(module, "+++\r\n");
    if(st != DXLR02_OK) 
        return st; 

    st = dxlr02_read_until(module, response, sizeof(response), '\n', 1, pdMS_TO_TICKS(500)); 
    
    if(st != DXLR02_OK) 
        return st; 
    
    if(strcmp(response, "Entry AT\r\n") == 0){ 
        module -> mode_AT = true; 
        return DXLR02_OK; 
    } else if(strcmp(response, "Exit AT\r\n") == 0){ 
        st = dxlr02_read_until(module, response, sizeof(response), '\n', 1, pdMS_TO_TICKS(500)); 
        if(st != DXLR02_OK) 
        return st; 
        
        if(strcmp(response, "Power On\r\n") == 0 || strcmp(response, "Power on\r\n") == 0){
            module -> mode_AT = false; 
            return DXLR02_OK; 
        } 
    } 
    
    return DXLR02_ERR_INVALID_RESPONSE; 
}

dxlr02_status_t dxlr02_ensure_at(dxlr02_t* module){ 
    dxlr02_status_t st; 
    st = dxlr02_AT_mode(module); 
    if(st != DXLR02_OK){
        return st; 
    }
    if(!module->mode_AT){ 
        st = dxlr02_AT_mode(module); 
        if(st != DXLR02_OK) {
            return st; 
        }  
    } 
    return DXLR02_OK; 
} 

dxlr02_status_t dxlr02_ensure_data_mode(dxlr02_t* module){ 
    dxlr02_status_t st; 
    st = dxlr02_AT_mode(module); 
    if(st != DXLR02_OK) 
        return st; 
    
    if(module->mode_AT){ 
        st = dxlr02_AT_mode(module); 
        if(st != DXLR02_OK) 
            return st; 
    } 
    
    return DXLR02_OK; 
} 




dxlr02_status_t dxlr02_set_baudrate(dxlr02_t * module, int baudrate){
    dxlr02_status_t st = dxlr02_ensure_at(module);
    if(st != DXLR02_OK)
        return st;

    switch(baudrate){
        case 1200:
        st = dxlr02_send_cmd(module, "AT+BAUD1\r\n");
        break;

        case 2400:
            st = dxlr02_send_cmd(module, "AT+BAUD2\r\n");
            break;

        case 4800:
            st = dxlr02_send_cmd(module, "AT+BAUD3\r\n");
            break;

        case 9600:
            st = dxlr02_send_cmd(module, "AT+BAUD4\r\n");
            break;

        case 19200:
            st = dxlr02_send_cmd(module, "AT+BAUD5\r\n");
            break;

        case 38400:
            st = dxlr02_send_cmd(module, "AT+BAUD6\r\n");
            break;

        case 57600:
            st = dxlr02_send_cmd(module, "AT+BAUD7\r\n");
            break;

        case 115200:
            st = dxlr02_send_cmd(module, "AT+BAUD8\r\n");
            break;

        case 128000:
            st = dxlr02_send_cmd(module, "AT+BAUD9\r\n");
            break;

        default:
            st = DXLR02_ERR_INVALID_PARAMETER;
            break;
    }

    if(st != DXLR02_OK)
        return st;

    char response[5];
    st = dxlr02_read_until(module, response, sizeof(response), '\n', 1, 500);
    if(st != DXLR02_OK)
        return st;

    if(strcmp(response, "OK\r\n") != 0)
        return DXLR02_ERR_INVALID_RESPONSE;

    st = dxlr02_ensure_data_mode(module);
    if(st != DXLR02_OK)
        return st;
        
    module->config.baudrate = baudrate;
    return DXLR02_OK;
}

dxlr02_status_t dxlr02_set_mode(dxlr02_t * module, uint8_t mode){
    if(mode > 2)
        return DXLR02_ERR_INVALID_PARAMETER;

    dxlr02_status_t st = dxlr02_ensure_at(module);
    if(st != DXLR02_OK)
        return st;

    char cmd[12];
    snprintf(cmd, sizeof(cmd), "AT+MODE%u\r\n", mode);

    char expected_response[14];
    snprintf(expected_response, sizeof(expected_response), "+MODE=%u\r\nOK\r\n", mode);

    char response[14];

    st = dxlr02_send_cmd(module, cmd);
    if(st != DXLR02_OK)
        return st;

    st = dxlr02_read_until(module, response, sizeof(response), '\n', 2, 500);
    if(st != DXLR02_OK)
        return st;

    if(strcmp(response, expected_response) != 0)
        return DXLR02_ERR_INVALID_RESPONSE;

    module->config.working_mode = mode;

    st = dxlr02_ensure_data_mode(module);
    if(st != DXLR02_OK)
        return st;
    
    return DXLR02_OK;
}

dxlr02_status_t dxlr02_set_energy_mode(dxlr02_t * module, uint8_t mode){
    // 1. Armo el string "AT+MODEx" con x: mode
    if(mode > 2)
        return DXLR02_ERR_INVALID_PARAMETER;

    dxlr02_status_t st = dxlr02_ensure_at(module);
    if(st != DXLR02_OK)
        return st;

    char cmd[13];
    snprintf(cmd, sizeof(cmd), "AT+SLEEP%u\r\n", mode);

    st = dxlr02_send_cmd(module, cmd);
    if(st != DXLR02_OK)
        return st;

    char response[5];
    st = dxlr02_read_until(module, response, sizeof(response), '\n', 1, 500);
    if(st != DXLR02_OK)
        return st;

    if(strcmp(response, "OK\r\n") != 0)
        return DXLR02_ERR_INVALID_RESPONSE;

    module->config.energy_mode = mode;

    st = dxlr02_ensure_data_mode(module);
    if(st != DXLR02_OK)
        return st;

    return DXLR02_OK;
}

dxlr02_status_t dxlr02_set_stop_bit(dxlr02_t * module, uint8_t stop){       // stop es cantidad de bits de stop (0, 1 o 2)
    if(stop > 2){
        return DXLR02_ERR_INVALID_PARAMETER;
    }

    dxlr02_status_t st = dxlr02_ensure_at(module);
    if(st != DXLR02_OK)
        return st;

    char cmd[12];
    snprintf(cmd, sizeof(cmd), "AT+STOP%u\r\n", stop);

    st = dxlr02_send_cmd(module, cmd);
    if(st != DXLR02_OK)
        return st;

    char response[5];
    st = dxlr02_read_until(module, response, sizeof(response), '\n', 1, 500);
    if(st != DXLR02_OK)
        return st;

    if(strcmp(response, "OK\r\n") != 0)
        return DXLR02_ERR_INVALID_RESPONSE;

    module->config.stop_bit = stop;

    st = dxlr02_ensure_data_mode(module);
    if(st != DXLR02_OK)
        return st;

    return DXLR02_OK;
}

dxlr02_status_t dxlr02_set_parity(dxlr02_t * module, uint8_t parity){
    if(parity > 2){
        return DXLR02_ERR_INVALID_PARAMETER;
    }

    dxlr02_status_t st = dxlr02_ensure_at(module);
    if(st != DXLR02_OK)
        return st;

    char cmd[12];
    snprintf(cmd, sizeof(cmd), "AT+PARI%u\r\n", parity);

    st = dxlr02_send_cmd(module, cmd);
    if(st != DXLR02_OK)
        return st;

    char response[5];
    st = dxlr02_read_until(module, response, sizeof(response), '\n', 1, 500);
    if(st != DXLR02_OK)
        return st;

    if(strcmp(response, "OK\r\n") != 0)
        return DXLR02_ERR_INVALID_RESPONSE;

    module->config.parity = parity;

    st = dxlr02_ensure_data_mode(module);
    if(st != DXLR02_OK)
        return st;

    return DXLR02_OK;
}

dxlr02_status_t dxlr02_reset(dxlr02_t * module){
    dxlr02_status_t st = dxlr02_ensure_at(module);
    if(st != DXLR02_OK)
        return st;

    st = dxlr02_send_cmd(module, "AT+RESET\r\n");
    if(st != DXLR02_OK)
        return st;

    char response[15];
    st = dxlr02_read_until(module, response, sizeof(response), '\n', 2, 500);
    if(st != DXLR02_OK)
        return st;

    if(strcmp(response, "OK\r\nPower On\r\n") != 0 && strcmp(response, "OK\r\nPower on\r\n") != 0)
        return DXLR02_ERR_INVALID_RESPONSE;

    st = dxlr02_ensure_data_mode(module);
    if(st != DXLR02_OK)
        return st;

    return DXLR02_OK;
}

dxlr02_status_t dxlr02_set_default(dxlr02_t * module){
    dxlr02_status_t st = dxlr02_ensure_at(module);
    if(st != DXLR02_OK){
        return st;
    }

    st = dxlr02_send_cmd(module, "AT+DEFAULT\r\n");
    if(st != DXLR02_OK)
        return st;

    char response[15];
    st = dxlr02_read_until(module, response, sizeof(response), '\n', 2, 500);
    if(st != DXLR02_OK)
        return st;

    if(strcmp(response, "OK\r\nPower On\r\n") != 0 && strcmp(response, "OK\r\nPower on\r\n") != 0)
        return DXLR02_ERR_INVALID_RESPONSE;

    module->config.address = 0xff;
    module->config.baudrate = 9600;
    module->config.channel = 0;
    module->config.crc = false;
    module->config.energy_mode = 2;
    module->config.iq_signal_flip = false;
    module->config.parity = 0;
    module->config.rate_level = 0;
    module->config.rf_coding_rate = 2;
    module->config.spread_factor = 12;
    module->config.stop_bit = 0;
    module->config.transmit_power = 22;
    module->config.working_mode = 0;

    st = dxlr02_ensure_data_mode(module);
    if(st != DXLR02_OK)
        return st;

    return DXLR02_OK;
}

dxlr02_status_t dxlr02_set_level(dxlr02_t * module, uint8_t level){
    if(level > 7){
        return DXLR02_ERR_INVALID_PARAMETER;
    }

    dxlr02_status_t st = dxlr02_ensure_at(module);
    if(st != DXLR02_OK)
        return st;

    char cmd[13];
    snprintf(cmd, sizeof(cmd), "AT+LEVEL%u\r\n", level);

    st = dxlr02_send_cmd(module, cmd);
    if(st != DXLR02_OK)
        return st;

    char response[5];
    st = dxlr02_read_until(module, response, sizeof(response), '\n', 1, 500);
    if(st != DXLR02_OK)
        return st;

    if(strcmp(response, "OK\r\n") != 0)
        return DXLR02_ERR_INVALID_RESPONSE;

    module->config.rate_level = level;

    st = dxlr02_ensure_data_mode(module);
    if(st != DXLR02_OK)
        return st;

    return DXLR02_OK;
}

dxlr02_status_t dxlr02_set_channel(dxlr02_t * module, uint8_t ch){
    if(ch > 0x1E){
        return DXLR02_ERR_INVALID_PARAMETER;
    }

    dxlr02_status_t st = dxlr02_ensure_at(module);
    if(st != DXLR02_OK)
        return st;

    char cmd[20];
    snprintf(cmd, sizeof(cmd), "AT+CHANNEL%02X\r\n", ch);

    char expected_response[20];
    snprintf(expected_response, sizeof(expected_response), "+CHANNEL=%02X\r\nOK\r\n", ch);
 
    st = dxlr02_send_cmd(module, cmd);
    if(st != DXLR02_OK)
        return st;

    char response[20];
    st = dxlr02_read_until(module, response, sizeof(response), '\n', 2, 500);
    if(st != DXLR02_OK)
        return st;

    if(strcmp(response, expected_response) != 0)
        return DXLR02_ERR_INVALID_RESPONSE;
        
    module->config.channel = ch;

    st = dxlr02_ensure_data_mode(module);
    if(st != DXLR02_OK)
        return st;

    return DXLR02_OK;
}

dxlr02_status_t dxlr02_set_mac(dxlr02_t * module, uint8_t mac){
    dxlr02_status_t st = dxlr02_ensure_at(module);
    if(st != DXLR02_OK)
        return st;
    
    char high[3];
    char low[3];

    snprintf(high, sizeof(high), "%02X", (mac >> 4) & 0xFF);
    snprintf(low, sizeof(low), "%02X", mac & 0xFF);

    char cmd[20];
    snprintf(cmd, sizeof(cmd), "AT+MAC%s,%s\r\n", high, low);
    char expected_response[20];
    snprintf(expected_response, sizeof(expected_response), "+MAC=%s%s\r\nOK\r\n", high, low);

    st = dxlr02_send_cmd(module, cmd);
    if(st != DXLR02_OK)
        return st;

    char response[20];
    st = dxlr02_read_until(module, response, sizeof(response), '\n', 2, 500);
    if(st != DXLR02_OK)
        return st;

    if(strcmp(response, expected_response) != 0)
        return DXLR02_ERR_INVALID_RESPONSE;

    module->config.address = mac;

    st = dxlr02_ensure_data_mode(module);
    if(st != DXLR02_OK)
        return st;

    return DXLR02_OK;
}


dxlr02_status_t dxlr02_set_transmit_power(dxlr02_t * module, uint8_t pow){
    if(pow > 22){
        return DXLR02_ERR_INVALID_PARAMETER;
    }

    dxlr02_status_t st = dxlr02_ensure_at(module);
    if(st != DXLR02_OK)
        return st;

    char cmd[20];
    snprintf(cmd, sizeof(cmd), "AT+POWE%u\r\n", pow);

    char expected_response[20];
    snprintf(expected_response, sizeof(expected_response), "+POWE=%u\r\nOK\r\n", pow);

    st = dxlr02_send_cmd(module, cmd);
    if(st != DXLR02_OK)
        return st;

    module->config.transmit_power = pow;

    char response[20];
    st = dxlr02_read_until(module, response, sizeof(response), '\n', 2, 500);
    if(st != DXLR02_OK)
        return st;

    if(strcmp(response, expected_response) != 0)
        return DXLR02_ERR_INVALID_RESPONSE;

    st = dxlr02_ensure_data_mode(module);
    if(st != DXLR02_OK)
        return st;

    return DXLR02_OK;
}

dxlr02_status_t dxlr02_set_coding_rate(dxlr02_t * module, uint8_t four_of_x){ // 4/5, 4/6, 4/7, 4/8
    if(four_of_x < 5 || four_of_x > 8)
        return DXLR02_ERR_INVALID_PARAMETER;

    dxlr02_status_t st = dxlr02_ensure_at(module);
    if(st != DXLR02_OK)
        return st;

    char cmd[10];
    snprintf(cmd, sizeof(cmd), "AT+CR%u\r\n", four_of_x - 4);

    char expected_response[20];
    snprintf(expected_response, sizeof(expected_response), "+CR=%u\r\nOK\r\n", four_of_x - 4);

    st = dxlr02_send_cmd(module, cmd);
    if(st != DXLR02_OK)
        return st;

    char response[20];
    st = dxlr02_read_until(module, response, sizeof(response), '\n', 2, 500);
    if(st != DXLR02_OK)
        return st;

    if(strcmp(response, expected_response) != 0)
        return DXLR02_ERR_INVALID_RESPONSE;

    module->config.rf_coding_rate = four_of_x;

    st = dxlr02_ensure_data_mode(module);
    if(st != DXLR02_OK)
        return st;

    return DXLR02_OK;
}


dxlr02_status_t dxlr02_set_spread_factor(dxlr02_t * module, uint8_t sf){
    if(sf < 5 || sf > 12){
        return DXLR02_ERR_INVALID_PARAMETER;
    }

    dxlr02_status_t st = dxlr02_ensure_at(module);
    if(st != DXLR02_OK)
        return st;

    char cmd[10];
    snprintf(cmd, sizeof(cmd), "AT+SF%u\r\n", sf);

    char expected_response[20];
    snprintf(expected_response, sizeof(expected_response), "+SF=%u\r\nOK\r\n", sf);

    st = dxlr02_send_cmd(module, cmd);
    if(st != DXLR02_OK)
        return st;

    char response[20];
    st = dxlr02_read_until(module, response, sizeof(response), '\n', 2, 500);
    if(st != DXLR02_OK)
        return st;

    if(strcmp(response, expected_response) != 0)
        return DXLR02_ERR_INVALID_RESPONSE;

    module->config.spread_factor = sf;

    st = dxlr02_ensure_data_mode(module);
    if(st != DXLR02_OK)
        return st;

    return DXLR02_OK;
}

dxlr02_status_t dxlr02_set_crc(dxlr02_t * module, bool crc){
    dxlr02_status_t st = dxlr02_ensure_at(module);
    if(st != DXLR02_OK)
        return st;

    if(crc){
        st = dxlr02_send_cmd(module, "AT+CRC1\r\n");
    } else {
        st = dxlr02_send_cmd(module, "AT+CRC0\r\n");
    }

    if(st != DXLR02_OK)
        return st;

    char response[5];
    st = dxlr02_read_until(module, response, sizeof(response), '\n', 1, 500);
    if(st != DXLR02_OK)
        return st;

    if(strcmp(response, "OK\r\n") != 0)
        return DXLR02_ERR_INVALID_RESPONSE;

    module->config.crc = crc;

    st = dxlr02_ensure_data_mode(module);
    if(st != DXLR02_OK)
        return st;

    return DXLR02_OK;
}

dxlr02_status_t dxlr02_set_iq_flip(dxlr02_t * module, bool flip){
    dxlr02_status_t st = dxlr02_ensure_at(module);
    if(st != DXLR02_OK)
        return st;

    if(flip){
        st = dxlr02_send_cmd(module, "AT+IQ1\r\n");
    } else {
        st = dxlr02_send_cmd(module, "AT+IQ0\r\n");
    }

    if(st != DXLR02_OK)
        return st;

    char response[5];
    st = dxlr02_read_until(module, response, sizeof(response), '\n', 1, 500);
    if(st != DXLR02_OK)
        return st;

    if(strcmp(response, "OK\r\n") != 0)
        return DXLR02_ERR_INVALID_RESPONSE;

    module->config.iq_signal_flip = flip;

    st = dxlr02_ensure_data_mode(module);
    if(st != DXLR02_OK)
        return st;

    return DXLR02_OK;
}

/**********************************/

dxlr02_status_t dxlr02_init(dxlr02_t * module, uint8_t port, int baudrate){
    if(!module)
        return DXLR02_ERR_INVALID_PARAMETER;
    if(module -> initialized)
        return DXLR02_ERR_ALREADY_INIT;
    module -> uart_port = port;
    module -> mode_AT = false;

    module -> initialized = true;

    dxlr02_status_t st = dxlr02_set_default(module); 
    if(st != DXLR02_OK){
        module -> initialized = false;
        return st;
    }

    st = dxlr02_set_baudrate(module, baudrate);
    if(st != DXLR02_OK){
        module -> initialized = false;
        return st;
    }

    module -> config.baudrate = baudrate;
    
    return DXLR02_OK;
}

dxlr02_status_t dxlr02_set_config(dxlr02_t * module, const dxlr02_config_t * conf){
    dxlr02_status_t st = dxlr02_ensure_at(module);
    if(st != DXLR02_OK)
        return st;

    st = dxlr02_set_baudrate(module, conf -> baudrate);
    if(st != DXLR02_OK){
        return st;
    }

    st = dxlr02_set_mode(module, conf -> working_mode);
    if(st != DXLR02_OK){
        return st;
    }

    st = dxlr02_set_energy_mode(module, conf -> energy_mode);
    if(st != DXLR02_OK){
        return st;
    }


    st = dxlr02_set_stop_bit(module, conf -> stop_bit);
    if(st != DXLR02_OK){
        return st;
    }
    
    st = dxlr02_set_parity(module, conf -> parity);
    if(st != DXLR02_OK){
        return st;
    }
    
    st = dxlr02_set_level(module, conf -> rate_level);
    if(st != DXLR02_OK){
        return st;
    }
    
    st = dxlr02_set_channel(module, conf -> channel);
    if(st != DXLR02_OK){
        return st;
    }
    
    st = dxlr02_set_mac(module, conf -> address);
    if(st != DXLR02_OK){
        return st;
    }
    
    st = dxlr02_set_transmit_power(module, conf -> transmit_power);
    if(st != DXLR02_OK){
        return st;
    }
    
    st = dxlr02_set_coding_rate(module, conf -> rf_coding_rate);
    if(st != DXLR02_OK){
        return st;
    }
    
    st = dxlr02_set_spread_factor(module, conf -> spread_factor);
    if(st != DXLR02_OK){
        return st;
    }
    
    st = dxlr02_set_crc(module, conf -> crc);
    if(st != DXLR02_OK){
        return st;
    }
    
    st = dxlr02_set_iq_flip(module, conf -> iq_signal_flip);
    if(st != DXLR02_OK){
        return st;
    }

    st = dxlr02_ensure_data_mode(module);
    if(st != DXLR02_OK)
        return st;

    return DXLR02_OK;
}



dxlr02_status_t dxlr02_get_config(dxlr02_t * module, dxlr02_config_t * conf){
    // TO DO
    return DXLR02_OK;
}

dxlr02_status_t dxlr02_send_data(dxlr02_t *module, const char *data, size_t size){
    // En el flujo del programa se debe estar en data_mode, es responsabilidad de quien llama a esta función
    // Las cadenas se envian con un \0

    if(!module || !module->initialized)
        return DXLR02_ERR_NOT_INITIALIZED;
    if(!data || size == 0)
        return DXLR02_ERR_INVALID_PARAMETER;

    if (data[size - 1] == '\0')
        return dxlr02_uart_send(module, data, size);

    char aux[size + 1];
    memcpy(aux, data, size);
    aux[size] = '\0';
    return dxlr02_uart_send(module, aux, size + 1);
}


dxlr02_status_t dxlr02_receive_data(dxlr02_t * module, char * data, size_t max_size, size_t * eff_len){
    // En el flujo del programa se debe estar en data_mode, es responsabilidad de quien llama a esta función
    // Asume que las cadenas se envian con un \0

    if(!module || !module->initialized)
        return DXLR02_ERR_NOT_INITIALIZED;

    if(!data || max_size == 0)
        return DXLR02_ERR_INVALID_PARAMETER;

    size_t i = 0;
    data[0] = '\0';
    int64_t init_time = esp_timer_get_time();
    int64_t timeout_us = (int64_t)TIMEOUT_READ_US;

    while(i < max_size - 1){
        if(esp_timer_get_time() - init_time > timeout_us)
            return DXLR02_ERR_TIMEOUT;

        int read = uart_read_bytes((uart_port_t) module -> uart_port, data + i, 1, pdMS_TO_TICKS(TIMEOUT_ONE_BYTE_MS));

        if(read < 0)
            return DXLR02_ERR_UART;
        else if(read == 0)
            continue;

        if(data[i] == '\0'){
            return DXLR02_OK;
        } 

        i++;
    }

    data[max_size - 1] = '\0';
    
    return DXLR02_ERR_OUT_OF_SPACE;

}