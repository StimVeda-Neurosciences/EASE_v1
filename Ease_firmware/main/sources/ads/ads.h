#ifndef _ADS_
#define _ADS_

#include"system.h"

////////////////////////// driver file for spi and gpio
#include "driver/spi_master.h"
#include "driver/spi_common.h"




///////// define the queue paramters 

#define ads_queue_len 120
#define ads_item_size 12

#define samples_to_send_time  40

#define items_to_club(x) (samples_to_send_time/x)


#define onebyte 8

#define time_to_wait_for_ads_data 0

#define ads_CLK_FREQ         (1*1000*1000)  ///1mhz

#define SPI_Mode1 1


/////////// ads regarding spi configuration
#define ads_cmd_bits              8
#define ads_addr_bits             8
#define ads_dummy_bits            0
#define ads_mode                  SPI_Mode1
#define ads_duty_cycle_pos        0
#define ads_ena_prettrrans        0
#define ads_Sclk                  ads_CLK_FREQ
#define ads_cs_ena_posttrans      0
#define ads_input_delay_ns        0
#define ads_CS_soft              -1
#define ads_flags                 0 //SPI_DEVICE_HALFDUPLEX
#define ads_spi_queue_size            1



//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
/////////////////////////ad sreg defination 
  //Commands
  #define _WAKEUP  0x02 // Wake-up from standby mode
  #define _STANDBY 0x04 // Enter Standby mode
  #define _RESET   0x06 // Reset the device
  #define _START   0x08 // Start and restart (synchronize) conversions
  #define _STOP    0x0A // Stop conversion
  #define _RDATAC  0x10 // Enable Read Data Continuous mode (default mode at power-up)
  #define _SDATAC  0x11 // Stop Read Data Continuous mode
  #define _RDATA   0x12 // Read data by command; supports multiple read back
  
  //Offsets
  #define _RREG 0x20 // (also = 00100000) is the first opcode that the address must be added to for RREG communication
  #define _WREG 0x40 // 01000000 in binary (Datasheet, pg. 35)
  
  //Register Addresses
  #define ID 0x00
  #define CONFIG1 0x01
  #define CONFIG2 0x02
  #define CONFIG3 0x03
  #define LOFF 0x04
  #define CH1SET 0x05
  #define CH2SET 0x06
  #define CH3SET 0x07
  #define CH4SET 0x08
  #define CH5SET 0x09
  #define CH6SET 0x0A
  #define CH7SET 0x0B
  #define CH8SET 0x0C
  #define BIAS_SENSP 0x0D
  #define BIAS_SENSN 0x0E
  #define LOFF_SENSP 0x0F
  #define LOFF_SENSN 0x10
  #define LOFF_FLIP 0x11
  #define LOFF_STATP 0x12
  #define LOFF_STATN 0x13
  #define GPIO 0x14
  #define MISC1 0x15
  #define MISC2 0x16
  #define CONFIG4 0x17


/////////////// function description 
#define soft_reset 0
#define hard_reset 1


/////////////////// ads gain
#define gain 12 // this is 12 for eeg only and 1 for impedance 
#define scale_factor (0.5364418/gain)


#define eeg_only 1
#define eeg_with_imp 2
//////////////////////////// init the spi bus and communication
void ads_spi_bus_init(void);

/////// init the ads ic , after specifgying the rate the ic start and generate interrupt 
uint8_t ads_init(uint8_t, uint8_t  );

/// @brief this is to verify that ads ic is present and working 
/// @param  void 
/// @return true /false 
uint8_t ads_verify_process(void);

/////////////// to deinit and abort the commuication 
void ads_deint(void);

////////////////// to reset the ads 
void ads_reset(uint8_t reset_type);


///////////// get the device id 
uint8_t get_DeviceID(void);
////////////////////////////////////////
void read_Allreg(void);

////////// get the status 
uint32_t get_dat(void);

//////////////////// convert the data in voltage value
float data_to_float(uint8_t * );

/// @brief this function is used to plot one sample data 
/// @param arr 
inline void eeg_plot_data(uint8_t *arr);


///////////////////////convert data to int 
void inline  data_to_int(uint8_t *buff);

#define device_id 0x3C
///////////////// extern function
extern void error_hand(void);

//////// the ads queue handle 
extern xQueueHandle ads_queue;



#endif