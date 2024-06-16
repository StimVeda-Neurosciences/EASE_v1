
#include<stdio.h>
#include<stdint.h>
#include "sys_attr.h"

#include "tdcs.h"

#include "esp_log.h"
#include "esp_err.h"
#include "esp_attr.h"

#include "driver/gpio.h"

#include "driver/adc.h"
#include "driver/adc_types_legacy.h"

#include "esp_adc_cal.h"
#include "esp_adc_cal_types_legacy.h"

static esp_adc_cal_characteristics_t adc_chars = {0};

static const char * TAG = "TDCS_ADC";

#define ADC_BITWITDTH ADC_WIDTH_BIT_12
#define ADC_ATTEN_FACTOR ADC_ATTEN_DB_6

#define ADC_PIN_CH ADC1_CHANNEL_7

#define DEFAULT_VREF    1121    
// #define DEFAULT_VREF    1065        //Use adc2_vref_to_gpio() to obtain a better estimate


#define ADC_SAMPLING_POINTS 20

#define Resistance 0.56 /// 560 ohm resistor 


/// @brief Get the ADC voltage from the 
/// @param  void 
/// @return return the adc voltage in micro volts units  
static uint32_t  tdcs_adc_get_vol(void);


/// @brief calibrate the adc driver 
/// @param  void
static void calibrate_adc_driver(void )
{
    
    esp_adc_cal_value_t cal_Val =  esp_adc_cal_characterize(ADC_UNIT_1,ADC_ATTEN_FACTOR,ADC_BITWITDTH,DEFAULT_VREF,&adc_chars);
      if (cal_Val == ESP_ADC_CAL_VAL_EFUSE_TP)
    {
        printf("Characterized using Two Point Value\n");
    }
    else if (cal_Val == ESP_ADC_CAL_VAL_EFUSE_VREF)
    {
        printf("Characterized using eFuse Vref\n");
    }
    else
    {
        printf("Characterized using Default Vref\n");
    }
}


static uint32_t IRAM_ATTR  tdcs_adc_get_vol(void)
{
    return esp_adc_cal_raw_to_voltage(adc1_get_raw(ADC_PIN_CH),&adc_chars);
}


IRAM_ATTR uint32_t tdcs_get_current_flowing(void)
{
    uint32_t i_value = 0;
    for(int i=0; i< ADC_SAMPLING_POINTS; i++)
    {
        i_value += tdcs_adc_get_vol()/Resistance;
    }
    return i_value/ADC_SAMPLING_POINTS;
}


void adc_driver_init(void)
{
    // configure the IO to be input 
    gpio_set_direction(PIN_TDCS_CURRENT_MONITOR, GPIO_MODE_INPUT);
    gpio_set_pull_mode(PIN_TDCS_CURRENT_MONITOR, GPIO_FLOATING);
    // Check if Two Point or Vref are burned into eFuse

    calibrate_adc_driver();

    adc_set_data_width(ADC_UNIT_1,ADC_BITWITDTH);
    adc1_config_channel_atten(ADC_PIN_CH,ADC_ATTEN_FACTOR);

}

void adc_driver_deinit(void)
{
    // delete the calibration scheme and delete the driver 
    //  no api function to deinit adc driver 
    // adc_digi_stop();
   
}

