#ifndef _TDCS_
#define _TDCS_


////////////////////////// driver file for spi and gpio
#include "driver/spi_master.h"
#include "driver/spi_common.h"
#include "driver/gpio.h"

#include "system.h"

//////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

#define PI 3.14285714

#define ang_freq (PI / 180)

#define set 1
#define Reset 0

#define onebyte 8

#define TDC_CLK_FREQ (1 * 1000 * 1000) /// 1mhz
#define INPUT_DELAY_NS ((1000 * 1000 * 1000 / TDC_CLK_FREQ) / 2 + 20)

#define SPI_Mode0 0



/////////// define the tdcs max transfer size
#define tdcs_max_xfr_size 32

/////////// tdcs regarding spi configuration
#define tdc_cmd_bits 8
#define tdc_addr_bits 8
#define tdc_dummy_bits 8
#define tdc_mode SPI_Mode0
#define tdc_duty_cycle_pos 0
#define tdc_ena_prettrrans 0
#define tdc_Sclk TDC_CLK_FREQ
#define tdc_cs_ena_posttrans 0
#define tdc_input_delay_ns 0
#define tdc_CS_soft -1
#define tdc_flags SPI_DEVICE_HALFDUPLEX
#define tdc_queue_size 1

/////////////////// Tdcs register defination ///////////

//////// write Address functions
#define NOP 0x00
#define DAC_DATA 0x01
#define reg_read 0x02
#define WRITE_CONTROL_REG 0x55
#define WRITE_RESET_REG 0x56
#define WRITE_CONF_REG 0x57
#define WRITE_DAC_GAIN_CALIB 0x58
#define WRITE_DAC_ZERO_CALIB 0x59
#define WATCHDOG_TIMER_RESET 0x95
#define CRC_ERROR_RESET 0x96

///////////// read address register

#define STATUS_reg 0x00
#define DAC_data_reg 0x01
#define CONTROl_reg 0x02
#define CONF_reg 0x0B
#define DAC_GAIN_CALIB 0x13
#define DAC_ZERO_CALIB 0x17
///////////////////////////////////////////////////////////////

//////////////// tdcs parameters that are used when running tdcs 

#define sampl_delay(t,a) ((t*10)/a)

////////////////////// time req for spi to transmit the complete data to tdcs ic 
#define time_taken_spi 50

///// @brief this is the current value that should be compared for impedance tracking 
#define TDCS_IMPEDANCE_CURRENT_THRESHOLD_VALUE 400

#define TDCS_ADC_OVERCURRENT_SAMPLING_TIME  150

#define TDCS_ELECTRODE_OPEN_CIRCUIT_VLAUE 300

////////////////////////////////////////////////////////////////////////////
/////////////////// enum to store the tdcs ble opcode
typedef enum _TAC_OPCODE_
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

} _tac_opcode;

////////////// anonymus enum 
enum
{
    none,
    ramp_up,
    constant,
    ramp_down,
    complete,

};

//////// enum for set current and measured current 
typedef  struct __packed  _CURENT_FED_
{
    uint16_t set_curent;
    uint32_t measured_current;

}current_imp;
//////////function prototype //////////////////////////////////////

///////// init the tdcs ic spi communication

void tdcs_bus_init(void);

//////////// deinit the tdcs bus 
void tdcs_bus_deinit(void);

///////// init the tdcs register 
void tdcs_init(uint8_t, uint16_t , uint32_t , uint32_t);

/// @brief  this is to verify the tdcs process 
/// @param  void 
/// @return succ/failure 
uint8_t tdcs_verify_process(void);

/////////////////////////////////// to stop the tdcs operation instantly
void tdcs_deinit(void);

////////////////////////////////////////////////////////
/////////// read the registers of the tdcs ic
void read_tdc_reg(void);

/////////// set the desired current value
void set_Current(uint16_t current);

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

//////////////////////////////////////// run tdcs//////
void run_tdcs(void);

/// @brief abort the tdcs process 
/// @param  
void abort_tdcs(void);

/// @brief is the tdcs procedure complete or not 
/// @return true /false   
uint8_t is_tdcs_complete(void);

/// @brief check the tdcs protection mechanisms here 
/// @param  void
/// @return err/ok
uint32_t check_tdcs_protection(void);


/////////////////////// to display help while debugging
void help(void);

#endif