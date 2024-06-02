
#include<stdio.h>
#include<stdint.h>
#include "sys_attr.h"

#include "tdcs.h"

#include "esp_log.h"
#include "esp_err.h"
#include "esp_attr.h"

#include "driver/gpio.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "hal/adc_types.h"


static const char * TAG = "TDCS_ADC";

#define DEFAULT_VREF    1121        //Use adc2_vref_to_gpio() to obtain a better estimate


#define ADC_SAMPLING_POINTS 30

#define Resistance 0.56 // in ohm


/// @brief Get the ADC voltage from the 
/// @param  void 
/// @return return the adc voltage in micro volts units  
static int IRAM_ATTR tdcs_adc_get_vol(void);




static adc_oneshot_unit_handle_t adc1_handle;
static adc_cali_handle_t adc_cali_handle;



static void calibrate_adc_driver(void )
{
    adc_cali_line_fitting_config_t adc_calib =
    {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
        .default_vref = DEFAULT_VREF,
        .unit_id = ADC_UNIT_1
    };
    esp_err_t err =  adc_cali_create_scheme_line_fitting(&adc_calib,&adc_cali_handle);
    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "adc driver calib unit failed");
        system_raw_restart();
    }

}


static int IRAM_ATTR  tdcs_adc_get_vol(void)
{
    // printf("Raw: reading is %d , voltage is mv %f \r\n", adc_reading, (adc_reading *0.598f));
    int raw=0;
    esp_err_t err =  adc_oneshot_read(adc1_handle, ADC_CHANNEL_7,&raw );
    if(err != ESP_OK)
    {
        return 0;
    }
    int voltage =0;
    err = adc_cali_raw_to_voltage(adc_cali_handle,raw, &voltage);
    if(err != ESP_OK)
    {
        return 0;
    }
    return voltage;
}


IRAM_ATTR int tdcs_get_current_flowing(void)
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

    // Configure ADC
    static const adc_oneshot_unit_init_cfg_t adc2_config = 
    {
        .ulp_mode = ADC_ULP_MODE_DISABLE,
        .unit_id = ADC_UNIT_1, 
    };
    esp_err_t err = adc_oneshot_new_unit(&adc2_config, &adc1_handle);

    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "adc driver unit failed");
        system_raw_restart();
    }

    adc_channel_t adc_channel;
    adc_unit_t adc_unit = ADC_UNIT_1;
    adc_oneshot_io_to_channel(PIN_TDCS_CURRENT_MONITOR,&adc_unit,&adc_channel);


    adc_oneshot_chan_cfg_t adc_chanel_config =
    {
        .bitwidth = ADC_BITWIDTH_12,
        .atten = ADC_ATTEN_DB_12,
    };
    adc_oneshot_config_channel(adc1_handle, adc_channel, &adc_chanel_config);
}

void adc_driver_deinit(void)
{
    // delete the calibration scheme and delete the driver 
    adc_cali_delete_scheme_line_fitting(adc_cali_handle);
    adc_oneshot_del_unit(adc1_handle);
}

