#pragma once 


//////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

#define PI 3.14285714

#define ang_freq (PI / 180)

#define set   1
#define Reset 0

#define onebyte 8

#define TDC_CLK_FREQ   (1 * 1000 * 1000) /// 1mhz
#define INPUT_DELAY_NS ((1000 * 1000 * 1000 / TDC_CLK_FREQ) / 2 + 20)

#define SPI_Mode0 0

/////////// define the tdcs max transfer size
#define tdcs_max_xfr_size 32

/////////// tdcs regarding spi configuration
#define tdc_cmd_bits         8
#define tdc_addr_bits        8
#define tdc_dummy_bits       8
#define tdc_mode             SPI_Mode0
#define tdc_duty_cycle_pos   0
#define tdc_ena_prettrrans   0
#define tdc_Sclk             TDC_CLK_FREQ
#define tdc_cs_ena_posttrans 0
#define tdc_input_delay_ns   0
#define tdc_CS_soft          -1
#define tdc_flags            SPI_DEVICE_HALFDUPLEX
#define tdc_queue_size       1

/////////////////// Tdcs register defination ///////////

//////// write Address functions
#define NOP                  0x00
#define DAC_DATA             0x01
#define reg_read             0x02
#define WRITE_CONTROL_REG    0x55
#define WRITE_RESET_REG      0x56
#define WRITE_CONF_REG       0x57
#define WRITE_DAC_GAIN_CALIB 0x58
#define WRITE_DAC_ZERO_CALIB 0x59
#define WATCHDOG_TIMER_RESET 0x95
#define CRC_ERROR_RESET      0x96

///////////// read address register

#define STATUS_reg     0x00
#define DAC_data_reg   0x01
#define CONTROl_reg    0x02
#define CONF_reg       0x0B
#define DAC_GAIN_CALIB 0x13
#define DAC_ZERO_CALIB 0x17
///////////////////////////////////////////////////////////////

//////////////// tdcs parameters that are used when running tdcs

#define sampl_delay(t, a) ((t * 10) / a)

////////////////////// time req for spi to transmit the complete data to tdcs ic
#define time_taken_spi 50

///// @brief this is the current value that should be compared for impedance tracking
#define TDCS_IMPEDANCE_CURRENT_THRESHOLD_VALUE 400

#define TDCS_ADC_OVERCURRENT_SAMPLING_TIME 150

#define TDCS_ELECTRODE_OPEN_CIRCUIT_VLAUE 300

