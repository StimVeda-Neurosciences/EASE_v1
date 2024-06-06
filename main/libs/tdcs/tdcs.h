#pragma once 

#include "sys_attr.h"

////////////////////////////////////////////////////////////////////////////
/////////////////// enum to store the tdcs ble opcode
enum _TAC_OPCODE_
{
    tac_none = 0x00,
    tac_sine_wave,
    tac_ramp_fun_uni,
    tac_ramp_fun_bi,
    tac_tdcs_prot,
    tac_square_uni,
    tac_square_bi,
    tac_set_current,
    tac_abort,
    tac_stop,

};

////////////// anonymus enum 
enum __TAC_TDCS_WAVE_STATE__
{
    tdcs_wave_none,
    tdcs_wave_ramp_up,
    tdcs_wave_constant,
    tdcs_wave_ramp_down,
    tdcs_wave_complete,

};

//////// enum for set current and measured current 
typedef  struct __CURRENT_STRUCT__
{
    uint16_t set_curent;
    uint32_t measured_current;

}PACKED current_struct_t;

typedef struct __TDCS_TASK__
{
  /* data */
  uint8_t opcode;
  uint16_t amplitude;
  uint32_t frequency;
  uint32_t time_till_run;
  
  /// @brief check the stop type of the TDCS 
  uint8_t stop_type;

}PACKED tdcs_cmd_struct_t;


// =========== functions 

/// @brief initialise the TDCS driver 
/// @param  void 
void tdcs_driver_init(void);

/// @brief deinit the TDCS driver 
/// @param  void 
void tdcs_driver_deinit(void);

/// @brief run the TDCS protocol accordint to the param
/// @param waveform_type 
/// @param amplitude 
/// @param frequency 
/// @param time_till_run 
void tdcs_start_prot(uint8_t waveform_type, uint16_t amplitude , uint32_t frequency , uint32_t time_till_run);

/// @brief verify the TDCS component 
/// @param  void 
/// @return err code
uint32_t tdcs_verify_component(void);

/// @brief stop the TDCS protocol 
/// @param  void 
void tdcs_stop_prot(void);

/// @brief read the TDCS register
/// @param  void
void read_tdc_reg(void);

/// @brief get the delay time for te tdcs process 
/// @param  void
/// @return delay time 
uint32_t tdcs_get_delay_Time(void);

/////// incerment and decrement 0.1 ma
void incremt(void);

/////////////// decrement  by 0.1 ma
void decrement(void);

//////////// configure the sine function give time in seconds
void sine_wave(void);

////////////////// configure the ramp function give time in seconds
void ramp_fun_uni(void);

/////////////// ramp function with bi directional voltage 
void ramp_fun_bi(void);

/////////////////////////////////// confgiure square wave//////////////////////
////////////////// bidirectional square wave
void square_bi(void);

////////////////////////////////////////////////////////////////////////////////
////////////////////////////// uindirectional square wave ///////////
void square_uni(void);

/// @brief run the TDCS protocol
/// @param  void 
void run_tdcs(void);

/// @brief abort the TDCS procedure 
/// @param  
void abort_tdcs(void);

/// @brief is the tdcs procedure complete or not 
/// @return true /false   
uint8_t is_tdcs_complete(void);

/// @brief check the tdcs protection mechanisms here 
/// @param  void
/// @return err/ok
uint32_t check_tdcs_protection(void);

/// @brief get the current flowing through electrodes 
/// @param  void 
/// @return curernt flowing through 
 int tdcs_get_current_flowing(void);


#define TDCS_ELECTRODES_OVERCURRENT_LIMIT 2300 //////refer to as 2.1v 


/////////////////////// to display help while debugging
void tdcs_help(void);
