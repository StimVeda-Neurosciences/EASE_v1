#pragma once 

/////////// contains the system level API that is used by both the test file and main file 


/////////standard lib
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "sdkconfig.h"
////////////////////////////////////////////////
////////////// Helper macros /////////////////


#define delay(x) (vTaskDelay(x/portTICK_PERIOD_MS))


// packed attribute
#define PACKED __packed

#define UNUSED __unused

/// @brief get the size of the array, only pass the array name
#define SIZEOF_ARR(x) ((sizeof(x) / sizeof(x[0])))

// calculate the offset of a memeber in a structure
#define OFFSET_OF(type, member) ((size_t) & ((type*) 0)->member)

// calculate the pointer to the main enclosing strucutre from a pointer member
#define CONTAINER_OF(ptr, type, member)                                                                                                    \
    ({                                                                                                                                     \
        const typeof(((type*) 0)->member)* __mptr = (ptr);                                                                                 \
        (type*) ((char*) __mptr - OFFSET_OF(type, member));                                                                                \
    })

// can be done in this way
// #define CONTAINER_OF(ptr, type, member) ({
//     (type *)( (char *)(ptr) - OFFSET_OF(type, member) ); })

// basic err checking macro
#define ESP_ERR_CHECK(x)                                                                                                                   \
    if (x)                                                                                                                                 \
    {                                                                                                                                      \
        ESP_LOGE(" ", "file %s line %d err %x", __FILE__, __LINE__, x);                                                                    \
        return x;                                                                                                                          \
    }

/// @brief check the error and skip the process
#define EXIT_IF_ERR(x)                                                                                                                       \
    if (x != ESP_OK)                                                                                                                       \
    {                                                                                                                                      \
        goto err;                                                                                                                          \
    }



#define u8_ptr(x) ((uint8_t *)&x)
#define u16_ptr(x) ((uint16_t *)&x)


#define _2bytes 2
#define _1byte 1
#define _3byte 3
#define _10bytes 10


#define MIN_OF(a,b) ((a)>(b)?(b):(a))
#define MAX_OF(a,b) ((a)>(b)?(a):(b))

#define MOD_OF(x) (((x)>0)?((x)):(-(x)))



///// CPU ID 
#define APP_CPU APP_CPU_NUM
#define PRO_CPU PRO_CPU_NUM

/////////// priorities of the task
#define PRIORITY_5 5
#define PRIORITY_4 4
#define PRIORITY_3 3
#define PRIORITY_2 2
#define PRIORITY_1 1


////////// all the poossible error codes for the devie
//////////////////////////////////////// in enum format 
///////// enum to store error code data 

enum
{
    ERR_NONE =0,
    ERR_SYS_BASE = 0x01,
    
    // invalid things errors 
    ERR_SYS_INVALID_STATE,
    ERR_SYS_INVALID_PARAM,
    ERR_SYS_INVALID_DATA,
    
    // empty things errors 
    ERR_SYS_NO_RESOURCES,
    ERR_SYS_EMPTY_MEM,
    ERR_SYS_EMPTY_DATA,
    ERR_SYS_NO_DATA,
    
    // fail things error
    ERR_SYS_OP_FAILED,
    ERR_SYS_BOOT_FAIL,
    ERR_SYS_API_ERR,
    ERR_SYS_APP_CRASH,
    ERR_SYS_FACTRY_RESET,
    ERR_SYS_TIMEOUT,
    
    ERR_EEG_BASE = 100,
	ERR_EEG_HARDWARE_FAULT,
    ERR_EEG_SYSTEM_FAULT,
	
    ERR_TDCS_BASE = 150,
	ERR_TDCS_HARDWARE_FAULT,
	ERR_TDCS_HRD_OVERCURRENT,
	ERR_TDCS_SOFT_OVERCURRENT,
    ERR_TDCS_ELECTRODES_OPEN,
	
    ERR_BATT_BASE = 200,
    ERR_BATT_HARDWARE_FAULT,
	ERR_BATT_CHG_IN_PROTOCOL,
    ERR_BATT_CRITICAL_LOW,

    ERR_ACCEL_BASE = 250,
	ERR_ACCEL_HARDWARE_FAULT,

};

#define EEG_ERR_POSITION  8 
#define TDCS_ERR_POSITION 16
#define FUEL_GAUGE_ERR_POSITION 0

////////// enum to store status of the system 
typedef enum __DEVICE_STATUS__
{
    STATUS_NONE = 0x01,
    STATUS_IDLE,
    STATUS_EEG_RUN,
    STATUS_TDCS,
    STATUS_OTA,
    STATUS_PWR_OFF,
    
}device_state_enum_t;

////////////////////////////////////////////////////////////////////////////
/////////////////// enum to store  general task state

enum __DEVICE_COMMANDS__
{
    DEV_STATE_NONE =1,
    DEV_STATE_IDLE,
    DEV_STATE_RUN_EEG,
    DEV_STATE_RUN_TDCS,
    DEV_STATE_STOP,
    DEV_STATE_SWITCH_OTA,
    DEV_STATE_BLE_CONNECTED,
    DEV_STATE_BLE_DISCONNECTED

};

/////////////////////////////////////////////////////////////////////////////////
/////////////////////pin defination //////////////////////////////////////////////

#define PIN_RED_LED    GPIO_NUM_4
#define PIN_GREEN_LED  GPIO_NUM_2
#define PIN_BLUE_LED   GPIO_NUM_15

#define PINS_LEDS (GPIO_SEL_4 | GPIO_SEL_2 | GPIO_SEL_15)

////==================== TDCS pins ========================
#define PIN_TDCS_MOSI GPIO_NUM_13
#define PIN_TDCS_MISO GPIO_NUM_12
#define PIN_TDCS_SCK GPIO_NUM_14
#define PIN_TDCS_CS GPIO_NUM_27

#define PIN_TDCS_BOOST_EN GPIO_NUM_25
#define PIN_TDCS_OVRCURRENT_INTR  GPIO_NUM_34
#define PIN_TDCS_CURRENT_MONITOR GPIO_NUM_35  ///// ADC channel 35 

#define PIN_TDCS_D1 GPIO_NUM_33
#define PIN_TDCS_D2 GPIO_NUM_32 // reverse of schematic 

#define TDCS_SPI_PORT 

////======================= EEG pins ===========================

#define PIN_EEG_MOSI   GPIO_NUM_23
#define PIN_EEG_MISO   GPIO_NUM_19
#define PIN_EEG_SCK   GPIO_NUM_18
#define PIN_EEG_CS     GPIO_NUM_5

#define PIN_EEG_DRDY_INTR    GPIO_NUM_39 ///// connected to gpio39 in newer board 
#define PIN_EEG_IC_EN  GPIO_NUM_21 /////// connected to esp32 gpio 21 in newer board 
#define PIN_EEG_IC_RESET   GPIO_NUM_22   /// differnt number on new board

// #define ads_ss_pinsel     GPIO_SEL_4
// #define ads_ddry_pinsel   GPIO_SEL_39
#define PINSEL_EEG_IC_EN GPIO_SEL_21
// #define ads_reset_pinsel  GPIO_SEL_22
/////////// define the ads max transfer size 
#define PIN_EEG_MAX_BUFF_SIZE   32 //////// in bytes 

#define EEG_SPI_PORT 
////======================= Fuel gauge  pins ===========================


//////////// define the I2C pins config 
#define PIN_BATT_SDA GPIO_NUM_10
#define PIN_BATT_SCL GPIO_NUM_9
#define BATT_I2C_PORT I2C_NUM_0


//////////////////////////////////////////////////////////////////////////
///////////////// Function definations /////////////////////////////////////

/***********
 * @name send_err_code
 * @param  errorcodes
 * @brief send the error code  to the queue
 * @return void 
*/
void sys_send_err_code(uint32_t code );

/// @brief send the status code to the status queue
/// @param  status code 
void sys_send_stats_code(uint8_t code );


/// @brief get the current no of items in the error q 
/// @param  void 
/// @return no of items
uint8_t sys_get_no_item_err_q(void);

/// @brief get the number of items in the status queue 
/// @param  void
/// @return no of items 
uint8_t sys_get_no_item_stat_q(void);

/// @brief get the first item from the error queue
/// @param  void 
/// @return first item from the err q
uint32_t sys_pop_err_q(void);

/// @brief get the first item from the status queue
/// @param  void
/// @return item from the stat queue
uint8_t sys_pop_stat_q(void);

/// @brief reset the error queue 
/// @param  void
void sys_reset_err_q(void);

/// @brief reset the status queue 
/// @param  void 
void sys_reset_status_q(void);


////////////////////////////////////////////////////////
////////////////////// message buffer ///////////////////

/// @brief push the message to the message buffer 
/// @param buff 
/// @param size 
void sys_push_msg_buff(uint8_t * buff, uint16_t size);

/// @brief reset the message buffer 
/// @param  void
void sys_reset_msg_buffer(void);

/// @brief pop the first message from the message buffer 
/// @param  buff
/// @param  size
void sys_pop_msg_buff(uint8_t *, uint16_t);

/// @brief check for emptiness of the message buffer 
/// @param  void 
/// @return true/false
bool sys_is_msgbuff_empty(void);


/// @brief init the system 
/// @param  void
void system_init(void);


// in case of restart and shutdown the system_deinit would be called by below funtions 
/// @brief shutdown the system 
/// @param  void
void system_shutdown(void);

/// @brief restart the system 
/// @param  void 
void system_restart(void);

/// @brief doesn't call any deinit api, just call restart 
/// @param  void 
void system_raw_restart(void);