#ifndef _NVS_H_
#define _NVS_H_

#include "stdint.h"

#include "nvs_flash.h"
#include "nvs.h"
#include "sdkconfig.h" 


extern void error_hand(void);

#define tdcs_time "tdcs_Time"
#define EEG_time "eeg_Time"
#define error_code "error_code"

///////////////////////// these value are for fuel gauge 
#define RC_COMP0          "rcomp0"
#define TEMP_CO           "tempco"
#define FULL_CAP_REP      "fullcaprep"
#define Saved_cycle       "savedcycle"
#define FUL_CAP_NOM       "fulcapnom"

#define ESP_OK 0
typedef int esp_err_t;

#define STORAGE_NAMESPACE "storage"

typedef uint32_t nvs_handle_t;

///// function prototype

///////// to get save the TDCS time to the flash when app is diconnected
void save_tdcs_time(uint64_t );

///// save the eeg time till the APP is disconnected 

void save_eeg_time(uint64_t );

///// get back the EEG time when esp32 restart it get the time from the flash
uint64_t get_eeg_time(void);

//// get the tdcs time store in the flash and send it to smartphone 
uint64_t get_tdcs_time(void);

//////// set the error code when the phone is disconnected from the esp32

void set_err_code(int error);

///// get back the error code when the esp32 restart
int get_err_code(void);

/////////must call this function to initiaise the nvs storage 
void nvs_initialize(void);

/////save the log info into the device 
void save_log_info(uint64_t , uint64_t , int);

/**
 * saving and reading hierarchy will be this 
 * RCOMP0
 * TEMPCO
 * FULLCAPREP
 * SAVED cYCLE
 * FULLCAPNOM
 */

////////////save the info like from the fuel gauge to flash
void save_fuel_gauge_nvs(uint16_t []);

//////////// get the fuel gauge readings from the flash
void get_fuel_gauge_nvs(uint16_t[]);







#endif