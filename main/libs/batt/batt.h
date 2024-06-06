#pragma once 

#include <stdio.h>
#include <stdint.h>

#include "sys_attr.h"



typedef struct  __BATT_DATA__ 
{
  /// @brief this will send by notif from headband
  uint8_t soc;
  uint8_t charging_status;

  /// @brief  rest are the data that it will supply in diff char
  uint16_t V_cell;
  uint16_t rem_cap;
  uint32_t time_till_empty;
  int16_t curr_flow;
  int16_t temp;
  uint16_t ic_status;
  
  /// @brief the device id of the fuel gauge
  uint16_t device_id;
}PACKED batt_data_struct_t;

#define BATT_DATA_STRUCT_SIZE (sizeof(batt_data_struct_t))

enum __BATT_CHARGING_STATUS__
{
  BATT_CHARGING = 10,
  BATT_DISCHARGING = 20,
  BATT_CHG_UNKOWN = 30
};


#define FUEL_GAUGE_PRESENT 0x20
#define FUEL_GAUGE_ABSENT  0x30

#define BATT_MIN_CHARGING_CURRENT 5


/// @brief initialise the battery driver 
/// @param  void
void batt_driver_init(void);

/// @brief deinit the i2c driver 
/// @param  void
void batt_driver_deinit(void);

/// @brief get the state of charge from the battery 
/// @param  void 
/// @return soc 
uint8_t batt_get_soc(void);

/// @brief get the battery voltage 
/// @param  void 
/// @return batt voltage in millivolt 
uint16_t batt_get_voltage(void);

/// @brief get the current from the battery 
/// @param  void 
/// @return battery current 
int16_t batt_get_current(void);

/// @brief get the charging status of the battery 
/// @param  void 
/// @return chg/discharging 
uint16_t batt_get_chg_status(void);


/// @brief this is to verify that fuel gauge is present and working
/// @param  void
/// @return succ/errcode
uint32_t batt_verify_component(void);

/// @brief get the battery custom data 
/// @param  batt_data_struct_t pointer 
/// @return err if driver is not init 
int batt_get_data(batt_data_struct_t *);


// used to send the Battery soc and charging status 
void batt_send_data(void);