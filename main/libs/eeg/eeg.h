#pragma once 

#include <stdio.h>
#include "system_attr.h"



enum __EGG_READING_TYPE__
{
  READING_TYPE_NULL,
  READING_EEG_ONLY,
  READING_EEG_WITH_IMP,
};

/// sample data len 
#define EEG_DATA_SAMPLE_LEN 12

/// @brief initialise the eeg driver 
/// @param  void 
void eeg_driver_init(void);

/// @brief deinit the eeg ic 
/// @param  void 
void eeg_driver_deinit(void);

/// @brief start the EEG reading  with below param  
/// @param  rate
/// @param  reading_type
/// @param  eeg_Taskhandle
/// @return Quehandle 
void * eeg_start_reading(uint8_t rate, uint8_t reading_type,void * taskahndle  );

/// @brief stop the ic reading and maybe powerdown the ic 
/// @param  void 
/// @return 
void eeg_stop_reading(void);

/// @brief read the EEG data from the IC, must be called only after start_reading
/// @param buff 
/// @param len 
void eeg_read_data(uint8_t *buff, uint16_t len);

/// @brief this is to verify that ads ic is present and working 
/// @param  void 
/// @return true /false 
uint32_t eeg_verify_component(void);

/// @brief reset the EEG ic 
/// @param reset_type 
void eeg_reset_ic(uint8_t reset_type);

/// @brief read all the IC register and print it 
/// @param  void 
void eeg_read_Allreg(void);

/// @brief convert input data to float 
/// @param  input data 
/// @return float value 
float eeg_data_to_float(uint8_t * );

/// @brief this function is used to plot one sample data 
/// @param arr 
 void eeg_plot_data(uint8_t *arr);

/// @brief convert input data to int 
/// @param buff 
void  eeg_data_to_int(uint8_t *buff);


typedef struct __EEG_CMD_STRUCTURE__
{
  uint8_t rate;
  uint32_t timetill_run;
}PACKED eeg_cmd_struct_t;


#define GET_NO_OF_SAMPLES(time,rate) (time/rate)

#define EEG_DATA_SENDING_TIME 40 // 40 millisecond 


// wait for 2 seconds if no data comes , then error 
#define EEG_NOTIF_WAIT_TIME 2000 