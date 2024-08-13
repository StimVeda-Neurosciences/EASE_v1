## System File

There are many ietms in the system file listed below 
- [Macro decelration](#macro-decelration)
- [Error enum and device state declaration](#error-enum-and-device-state-declaration)
- [pinout macro definations](#pinout-macro-definations)
- [Function decelration](#function-decelration)
- [Structure decelration](#structure-decelration)


### Macro decelration 

--- 
the macro decelration would look like this 

```c

#define delay(x) (vTaskDelay(x/portTICK_PERIOD_MS))


#define u8(x) ((uint8_t *)&x)
#define u16(x) ((uint16_t *)&x)


#define _2bytes 2
#define _1byte 1
#define _3byte 3
#define _10bytes 10


#define min(a,b) (a>b?b:a)
#define max(a,b) (a>b?a:b)

#define MOD(x) (((x)>0)?((x)):(-(x)))

#define s(x) sizeof(x)

///////////// wait tick period for queue and messages
#define wait_for_q_msg 10

#define wait_for_msg_buffer 10 /////// 10 ticks period 

#define idle_state_wait_time  300000


/////// length of the queue
#define error_queue_length  1
#define error_q_item_Size 4 ///// sizeof(uint32_t)
#define status_queue_length 1
#define status_q_item_size 1 /////sizeof(uint8_t)


///// CPU ID 
#define app_cpu 1
#define pro_cpu 0

/////////// priorities of the task
#define PRIORTY_5 5
#define PRIORTY_4 4
#define PRIORTY_3 3
#define PRIORTY_2 2
#define PRIORTY_1 1

```

### Error enum and device state declaration 

```c

enum
{
    error_none = 0x00,
    error_timeout,
    
	err_eeg_hardware_not_wroking =100,
  err_eeg_hardware_system_fault,
	
	err_tDCS_hardware_not_wroking = 110,
	err_tDCS_overcurrent_hardware_trigerred =111,
	err_tDCS_overcurrent_software_trigerred = 112,
  err_tDCS_electrodes_are_open = 113,
	
	err_Fuel_gauge_hardware_not_wroking = 120,
	err_battery_charging_while_protocol_running = 121,
  err_battery_critically_low = 122,

  
	err_accelro_hardware_not_wroking = 130

};

#define EEG_ERR_POSITION  8 
#define TDCS_ERR_POSITION 16
#define FUEL_GAUGE_ERR_POSITION 0

////////// enum to store status of the system 
enum
{
    status_none = 0x01,
    status_idle,
    status_eeg_runs,
    status_tdcs_runs, 
    status_pwr_save,
    
};

////////////////////////////////////////////////////////////////////////////
/////////////////// enum to store  general task state

enum _DEVICE_STATE_
{
    IDLE,
    RUN_EEG,
    RUN_TDCS,
    STOP,
    BLE_CONNECTED,
    BLE_DISCONNECTED

};
```

### pinout macro definations 

```c

#define red_led_pin    GPIO_NUM_4
#define green_led_pin  GPIO_NUM_2
#define blue_led_pin   GPIO_NUM_15

#define lights_pin (GPIO_SEL_4 | GPIO_SEL_2 | GPIO_SEL_15)

////==================== TDCS pins ========================
#define tdcs_mosi_pin GPIO_NUM_13
#define tdcs_miso_pin GPIO_NUM_12
#define tdcs_sclk_pin GPIO_NUM_14
#define tdcs_ss_pin GPIO_NUM_27

#define tdcs_boost_en_pin GPIO_NUM_25
#define tdcs_over_curr_intr_pin  GPIO_NUM_34
#define tdcs_adc_monitor_pin GPIO_NUM_35  ///// ADC channel 35 

#define tdcs_d1_pin GPIO_NUM_33
#define tdcs_d2_pin GPIO_NUM_32 // reverse of schematic 

////======================= EEG pins ===========================

#define ads_mosi_pin   GPIO_NUM_23
#define ads_miso_pin   GPIO_NUM_19
#define ads_sclk_pin   GPIO_NUM_18
#define ads_ss_pin     GPIO_NUM_5

#define ads_ddry_pin    GPIO_NUM_39 ///// connected to gpio39 in newer board 
#define ads_enable_pin  GPIO_NUM_21 /////// connected to esp32 gpio 21 in newer board 
#define ads_reset_pin   GPIO_NUM_22   /// differnt number on new board

// #define ads_ss_pinsel     GPIO_SEL_4
// #define ads_ddry_pinsel   GPIO_SEL_39
#define ads_enable_pinsel GPIO_SEL_21
// #define ads_reset_pinsel  GPIO_SEL_22
/////////// define the ads max transfer size 
#define ads_max_xfr_size   32 //////// in bytes 

////======================= Fuel gauge  pins ===========================


//////////// define the I2C pins config 
#define fuel_sda_pin GPIO_NUM_10
#define fuel_scl_pin GPIO_NUM_9
```

### Function decelration 

```c
 * @name send_err_code
 * @param  errorcodes
 * @brief send the error code  to the queue
 * @return void 
*/
void send_err_code(uint32_t);

/// @brief send the status code to the status queue
/// @param  status code 
void send_stats_code(uint8_t);


/// @brief get the current no of items in the error q 
/// @param  void 
/// @return no of items
uint8_t get_no_item_err_q(void);

/// @brief get the number of items in the status queue 
/// @param  void
/// @return no of items 
uint8_t get_no_item_stat_q(void);

/// @brief get the first item from the error queue
/// @param  void 
/// @return first item from the err q
uint32_t pop_err_q(void);

/// @brief get the first item from the status queue
/// @param  void
/// @return item from the stat queue
uint8_t pop_stat_q(void);

/// @brief reset the error queue 
/// @param  void
void reset_err_q(void);

/// @brief reset the status queue 
/// @param  void 
void reset_status_q(void);


////////////////////////////////////////////////////////
////////////////////// message buffer ///////////////////

/// @brief push the message to the message buffer 
/// @param buff 
/// @param size 
void push_msg_buff(uint8_t * buff, uint16_t size);

/// @brief reset the message buffer 
/// @param  void
void reset_msg_buffer(void);

/// @brief pop the first message from the message buffer 
/// @param  buff
/// @param  size
void pop_msg_buff(uint8_t *, uint16_t);

/// @brief check for emptiness of the message buffer 
/// @param  void 
/// @return true/false
bool is_msgbuff_empty(void);

/// @brief put a specific color to the led 
/// @param  color
void put_color(uint8_t);

/// @brief init the system 
/// @param  void
void system_init(void);


/// @brief shutdown the system 
/// @param  void
void system_shutdown(void);

/// @brief delete a task and also nullify the input param task 
/// @param  taskHandle
void taskdelete(TaskHandle_t *);
```



### Structure decelration

```c
typedef struct __packed _TDCS_TASK_
{
  /* data */
  uint8_t opcode;
  uint16_t amplitude;
  uint32_t frequency;
  uint32_t time_till_run;
  
  /// @brief check the stop type of the TDCS 
  uint8_t stop_type;

}funct_tdcs_data;

typedef struct __packed _EEG_TASK_
{
  uint8_t rate;
  uint32_t timetill_run;
}func_eeg_task;

```

