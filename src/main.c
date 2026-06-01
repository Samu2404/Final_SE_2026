#include "driver/spi_master.h"
#include "stdint.h"
#include "driver/gpio.h"
#include"driver/timer.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "driver/uart.h"


adc_oneshot_unit_handle_t adc1_handle;
spi_host_device_t host_id = SPI2_HOST;
spi_device_handle_t mcp4132_handle ;

volatile int adc_value = 0;

void spi_bus_init(void){
    spi_bus_config_t bus = {
    .mosi_io_num= GPIO_NUM_23,
    .miso_io_num= GPIO_NUM_19,
    .sclk_io_num= GPIO_NUM_18,
    .quadwp_io_num= -1,
    .quadhd_io_num=-1,
    };
    spi_bus_initialize (host_id, &bus);

    spi_device_interface_config_t devcfg ={
        .clock_speed_hz= 100000000,
        .mode= 3,
    };
}

void mcp4132_write_register (uint8_t reg, uint8_t const *data){
    uint8_t buffer[2];
    buffer[0] = reg|0x00;
    buffer[1] = *data;
    spi_transaction_t config = {
        .length= 16,
        .tx_buffer= buffer,
    };
    spi_device_transmit(mcp4132_handle, &config);
}

void mcp4132_read_register(uint8_t reg, uint8_t *data){
    uint8_t buffer[2];
    buffer[0] = reg | 0x0C ;
    buffer[1] = 0x00; // byte de relleno para recibir el dato          
    spi_transaction_t config = {
        .length= 16,
        .rx_buffer= data};
    spi_device_transmit(mcp4132_handle, &config);
}

void mcp4132_set_wiper (uint32_t value){
    if (value > 128) {
        value = 128;
    }
    else if (value < 0) {
        value = 0;
    }
    
    uint8_t hex_value = (uint8_t)(value * 255 / 128);  // Convertir a hex
    mcp4132_write_register(0x00, &hex_value);
}

void mcp4132_set_cutoff_frequency (float frequency){
    uint32_t value =(100000000/(frequency*2*3.14)); 
    int N= (value-75)*128/(10000);
    mcp4132_set_wiper(N);
}


void configurar_adc (void){
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t adc1_config = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    adc_oneshot_new_unit(&adc1_config, &adc1_handle);

    adc_oneshot_chan_cfg_t adc1_channel_config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_0, &adc1_channel_config);
}



IRAM_ATTR bool timer_isr(void *arg){
    adc_oneshot_read(adc1_handle, ADC_CHANNEL_0, &adc_value);
    return false;
}

void configurar_timer (void){
    timer_config_t timer_config = {
        .divider = 80, // Prescaler para 1 MHz
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_EN,
        .auto_reload = true,
    };
    timer_init(TIMER_GROUP_0, TIMER_0, &timer_config);
    timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, 1000); // 1000 Hz
    timer_enable_intr(TIMER_GROUP_0, TIMER_0);
    timer_start(TIMER_GROUP_0, TIMER_0);
}

configurar_uart(void){
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM_0, &uart_config);
    uart_driver_install(UART_NUM_0, 1024, 0, 0, NULL, 0);
}

void app_main() {
    spi_bus_init();
    configurar_adc();
    configurar_timer();
    configurar_uart();
    mcp4132_set_cutoff_frequency(1000);

    while (1) {
    if (adc_value > 1737) {
        *char cmd_1="Valor cambiado N=42";
        uart_write_bytes(0,cmd_1);
        mcp4132_set_wiper(95);
    } else if (adc_value < 1117)
        mcp4132_set_wiper(42);
        *char cmd_2 ="Valor cambiado N=42";
        uart_write_bytes(0,cmd_2)
    }      
}
