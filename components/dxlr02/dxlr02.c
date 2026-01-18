#include "dxlr02.h"
#include "driver/uart.h"
#include "string.h"

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

static dxlr02_status_t dxlr02_uart_read(dxlr02_t *module, char *data, size_t len, size_t * read){
    if(!module || !module -> initialized)
        return DXLR02_ERR_NOT_INITIALIZED;

    *read = uart_read_bytes((uart_port_t) module -> uart_port, data, len, pdMS_TO_TICKS(500));

    if(*read == 0){
        return DXLR02_ERR_TIMEOUT;  // revisar
    } else if (*read < 0) {
        return DXLR02_ERR_UART;
    } else if (*read != len){
        return DXLR02_ERR_INVALID_RESPONSE; // revisar
    }

    return DXLR02_OK;
}

static dxlr02_status_t dxlr02_read_exact(dxlr02_t *module, const char *expected, size_t len, uint32_t timeout_ms){
    if(!module || !module->initialized)
        return DXLR02_ERR_NOT_INITIALIZED;

    char data[len];
    int read = uart_read_bytes((uart_port_t) module -> uart_port, data, len, pdMS_TO_TICKS(timeout_ms));

    if(read != len)
        return DXLR02_ERR_INVALID_RESPONSE;

    size_t i = 0;
    while (i < len && data[i] == expected[i]) 
        i++;


    if(i != len)
        return DXLR02_ERR_INVALID_RESPONSE;

    return DXLR02_OK;
}

dxlr02_status_t dxlr02_send_cmd(dxlr02_t * module, const char * cmd, const char * response){
    if(!module || !module->initialized){
        return DXLR02_ERR_NOT_INITIALIZED;
    }
    
    uart_flush_input((uart_port_t)module->uart_port);
    
    dxlr02_status_t st = dxlr02_uart_send(module, cmd, strlen(cmd));
    if(st != DXLR02_OK)
        return st;

    return dxlr02_read_exact(module, response, strlen(response), 500);
 
}

/****************************************** AUXILIAR FUNCTIONS ******************************************/
static dxlr02_status_t dxlr02_ensure_at(dxlr02_t* module){
    dxlr02_status_t st;
    st = dxlr02_AT_mode(module);
    if(st != DXLR02_OK)
        return st;

    if(!module->mode_AT){
        st = dxlr02_AT_mode(module);
        if(st != DXLR02_OK)
            return st;
    }

    return DXLR02_OK;
}

static dxlr02_status_t dxlr02_ensure_data_mode(dxlr02_t* module){
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

dxlr02_status_t dxlr02_AT_mode(dxlr02_t * module){
    const char *msg = "+++\r\n";
    dxlr02_status_t st;
    char response[25];
    bool out;

    st = dxlr02_uart_send(module, msg, strlen(msg));
    if(st != DXLR02_OK)
        return st;

    size_t read;
    st = dxlr02_uart_read(module, response, 24, &read);
    if(st != DXLR02_OK)
        return st;

    response[read] = '\0';

    if(strcmp(response, "Entry AT\r\n") == 0){
        out = true;
    }
    else if(strcmp(response, "Exit AT\r\nPower on\r\n") == 0){
        out = false;
    }
    else{
        return DXLR02_ERR_INVALID_RESPONSE;
    } 

    module -> mode_AT = out;
    return DXLR02_OK;
}

dxlr02_status_t dxlr02_set_baudrate(dxlr02_t * module, int baudrate){
    dxlr02_status_t st = dxlr02_ensure_at(module);
    if(st != DXLR02_OK)
        return st;

    switch(baudrate){
        case 1200:
            st = dxlr02_send_cmd(module, "AT+BAUD1\r\n", "OK\r\n");
            break;

        case 2400:
            st = dxlr02_send_cmd(module, "AT+BAUD2\r\n", "OK\r\n");
            break;

        case 4800:
            st = dxlr02_send_cmd(module, "AT+BAUD3\r\n", "OK\r\n");
            break;

        case 9600:
            st = dxlr02_send_cmd(module, "AT+BAUD4\r\n", "OK\r\n");
            break;

        case 19200:
            st = dxlr02_send_cmd(module, "AT+BAUD5\r\n", "OK\r\n");
            break;

        case 38400:
            st = dxlr02_send_cmd(module, "AT+BAUD6\r\n", "OK\r\n");
            break;

        case 57600:
            st = dxlr02_send_cmd(module, "AT+BAUD7\r\n", "OK\r\n");
            break;

        case 115200:
            st = dxlr02_send_cmd(module, "AT+BAUD8\r\n", "OK\r\n");
            break;

        case 128000:
            st = dxlr02_send_cmd(module, "AT+BAUD9\r\n", "OK\r\n");
            break;

        default:
            st = DXLR02_ERR_INVALID_PARAMETER;
            break;
    }

    
    if(st != DXLR02_OK)
        return st;

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

    char response[14];
    snprintf(response, sizeof(response), "+MODE=%u\r\nOK\r\n", mode);

    st = dxlr02_send_cmd(module, cmd, response);
    if(st != DXLR02_OK)
        return st;

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

    st = dxlr02_send_cmd(module, cmd, "OK\r\n");
    if(st != DXLR02_OK)
        return st;

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

    st = dxlr02_send_cmd(module, cmd, "OK\r\n");
    if(st != DXLR02_OK)
        return st;

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

    st = dxlr02_send_cmd(module, cmd, "OK\r\n");
    if(st != DXLR02_OK)
        return st;

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

    st = dxlr02_send_cmd(module, "AT+RESET", "OK\r\nPower On\r\n");
    if(st != DXLR02_OK)
        return st;

    st = dxlr02_ensure_data_mode(module);
    if(st != DXLR02_OK)
        return st;

    return DXLR02_OK;
}

dxlr02_status_t dxlr02_set_default(dxlr02_t * module){
    dxlr02_status_t st = dxlr02_ensure_at(module);
    if(st != DXLR02_OK)
        return st;

    st = dxlr02_send_cmd(module, "AT+DEFAULT", "OK\r\nPower On\r\n");
    if(st != DXLR02_OK)
        return st;

    module->config.address = 0xffff;
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

    st = dxlr02_send_cmd(module, cmd, "OK\r\n");
    if(st != DXLR02_OK)
        return st;

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

    char cmd[16];
    snprintf(cmd, sizeof(cmd), "AT+CHANNEL%02X\r\n", ch);

    char response[19];
    snprintf(response, sizeof(response), "+CHANNEL=%02X\r\nOK\r\n", ch);
 
    st = dxlr02_send_cmd(module, cmd, response);
    if(st != DXLR02_OK)
        return st;

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

    char cmd[14];
    snprintf(cmd, sizeof(cmd), "AT+MAC%s,%s\r\n", high, low);
    char response[16];
    snprintf(response, sizeof(response), "+MAC=%s%s\r\nOK\r\n", high, low);

    st = dxlr02_send_cmd(module, cmd, response);
    if(st != DXLR02_OK)
        return st;

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

    char cmd[12];
    snprintf(cmd, sizeof(cmd), "AT+POWE%u\r\n", pow);

    char response[15];
    snprintf(response, sizeof(response), "+POWE=%u\r\nOK\r\n", pow);

    st = dxlr02_send_cmd(module, cmd, response);
    if(st != DXLR02_OK)
        return st;

    module->config.transmit_power = pow;

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

    char response[12];
    snprintf(response, sizeof(response), "+CR=%u\r\nOK\r\n", four_of_x - 4);

    st = dxlr02_send_cmd(module, cmd, response);
    if(st != DXLR02_OK)
        return st;

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

    char response[13];
    snprintf(response, sizeof(response), "+SF=%u\r\nOK\r\n", sf);

    st = dxlr02_send_cmd(module, cmd, response);
    if(st != DXLR02_OK)
        return st;

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
        st = dxlr02_send_cmd(module, "AT+CRC1\r\n", "OK\r\n");
    } else {
        st = dxlr02_send_cmd(module, "AT+CRC0\r\n", "OK\r\n");
    }

    if(st != DXLR02_OK)
        return st;

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
        st = dxlr02_send_cmd(module, "AT+IQ1\r\n", "OK\r\n");
    } else {
        st = dxlr02_send_cmd(module, "AT+IQ0\r\n", "OK\r\n");
    }

    if(st != DXLR02_OK)
        return st;

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

    dxlr02_status_t st = dxlr02_set_default(module); 
    if(st != DXLR02_OK)
        return st;

    st = dxlr02_set_baudrate(module, baudrate);
    if(st != DXLR02_OK)
        return st;
        
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
}

dxlr02_status_t dxlr02_send_data(dxlr02_t * module, const char * data, size_t size){
    // TO DO
}

dxlr02_status_t dxlr02_receive_data(dxlr02_t * module, char * data, size_t max_size, size_t * eff_len){
    // TO DO
}