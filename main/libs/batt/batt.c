#include<string.h>

#include "batt.h"
#include "batt_regs.h"

#include "system_attr.h"
#include "ble.h"

#include"freertos/FreeRTOS.h"
#include"freertos/task.h"

#include "driver/i2c.h"

#include "esp_log.h"


static volatile batt_data_struct_t batt_data;

static const char * TAG = "BATT";

// ========================= task realted param 
#define BATT_TASK_STACK_DEPTH 2048
#define BATT_TASK_PARAM NULL
#define BATT_TASK_CPU PRO_CPU
#define BATT_TASK_NAME "BATT_TASK"
#define BATT_TASK_PRIORITY PRIORITY_5

void batt_function_task(void* param);
static TaskHandle_t batt_taskhandle;

static StackType_t batt_Task_stack_mem[BATT_TASK_STACK_DEPTH];
static StaticTask_t batt_task_tcb;

#define battery_state_change_update_time 500



static void send_fuelgauge(uint8_t addr, uint16_t data) {
  uint8_t writebuf[3] = {addr, (data & 0xFF), ((data >> 8) & 0xFF)};

  i2c_master_write_to_device(BATT_I2C_PORT, Sens_addr, writebuf, 3, i2c_wait_time);
}

static uint16_t get_fuelgauge(uint8_t addr) {
  uint8_t ret_data[2] = {0};
  if (i2c_master_write_read_device(BATT_I2C_PORT, Sens_addr, &addr, 1, ret_data, 2, i2c_wait_time) != ESP_OK) {
    return 0;
  }

  return (uint16_t) (ret_data[0] | (ret_data[1] << 8));
}

static void verify_write_fuel_gauge(uint8_t addr, uint16_t data) {

  uint8_t ret_data[2] = {0};
 
  if (i2c_master_write_read_device(BATT_I2C_PORT, Sens_addr, &addr, 1, ret_data, 2, i2c_wait_time) != ESP_OK) {
    return;
  }

  uint16_t rcvd_data = (uint16_t) (ret_data[0] | (ret_data[1] << 8));

  if (data != rcvd_data) {
    printf("wr %x\r\n", addr);

    uint8_t writebuf[3] = {addr, (data & 0xFF), ((data >> 8) & 0xFF)};

    i2c_master_write_to_device(BATT_I2C_PORT, Sens_addr, writebuf, 3, i2c_wait_time);
  }
}



/// @brief initialise the battery driver 
/// @param  void
void batt_driver_init(void)
{

  i2c_config_t m5_config = {.mode = I2C_MODE_MASTER,
                            .sda_io_num = PIN_BATT_SDA,
                            .scl_io_num = PIN_BATT_SCL,
                            .sda_pullup_en = 1,                  //// using the external pullup resistor
                            .scl_pullup_en = GPIO_PULLUP_ENABLE, //// using the external pullup resistor
                            .master.clk_speed = scl_freq,
                            .clk_flags = 0

  };

  i2c_param_config(BATT_I2C_PORT, &m5_config);

  if (i2c_driver_install(BATT_I2C_PORT, I2C_MODE_MASTER, 0, 0, 0) != ESP_OK) {
    printf("error cannot init fuel gauge \r\n");
    assert(0);
  }

  //// get the device fuel gauge id
  if (get_fuelgauge(DevName) != FUEL_GAUGE_DEV_ID) {
    printf("no fuel gauge \r\n");
    goto return_mech;
  }
  /////// wait for the IC to power up Fstat.DNR should be 0
  while (get_fuelgauge(FStat) & 0x01) {
    printf("wait\r\n");
  }

  //// first wakeup the IC with proper method describe in datasheet
  verify_write_fuel_gauge(SOFTWAKEUP, 0x0090);
  verify_write_fuel_gauge(HibCFG, 0x0000);
  verify_write_fuel_gauge(SOFTWAKEUP, 0x0000);

  /////////////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////////////////////
  ////////////// application specific configuration

  verify_write_fuel_gauge(SOCHold, (0x02 | (0x05 << 5)));

  verify_write_fuel_gauge(DesignCap,
                          (uint16_t) (2 * Batt_Mah)); // Designed capacity of LiPo in mAh 0X07D0=1000mAh, least count=0.5mAh @10mOhm res

  verify_write_fuel_gauge(ICFGTerm, Batt_Mah / 10); /////// corresponds to 100 ma  ///// // 1LSB = 1.5625uV/Rsense, IChgTerm < C/10 = 100mA
                                                    ///in our case. We will choose 50mA as charging IC is set to 45mA Ichg.
                                                    // 320step = 320*156.25uA/step = 50000uA = 50mA

  verify_write_fuel_gauge(Config1, 0x0000); // default values. Bit15=0 for internal temp measurement
  verify_write_fuel_gauge(Config2, 0x3658); // default values

  /////////////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////////////////////
  ////////////// m5 algorithm configuration
  verify_write_fuel_gauge(RelaxCFG, (1 << 8) | (8 << 4) | (10)); // load current -> 5ma , dv = 10mv , dt = 3 minutes
  //// lerancfg --> makes the volatge dominate or columb counter dominate
  // verify_write_fuel_gauge(ModelCFG, 0x8004); /// do not write the register as learn model get reset
  /////////////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////////////////////
  ////////////// cell characterization configuration
  verify_write_fuel_gauge(FullSOCThr, 0x5005); // default is 0x5F05=95%, but use 80%=0x5005 for EZ performance.

  verify_write_fuel_gauge(VEmpty, (BATT_VEMPTY << 7) | (BATT_VRECOVERY)); // Keep VR same, change VE to 3V. /////// default VE is 3.3v

  // if POR bit in status is 1, clear it
  verify_write_fuel_gauge(STATUS, 0x0000);

return_mech:
  ///////// reserve 1kb space for the fuel gauge task
  batt_taskhandle =  xTaskCreateStaticPinnedToCore(batt_function_task,BATT_TASK_NAME,BATT_TASK_STACK_DEPTH,BATT_TASK_PARAM,BATT_TASK_PRIORITY,batt_Task_stack_mem,&batt_task_tcb,BATT_TASK_CPU);
  assert(batt_taskhandle != NULL);

}

/// @brief deinit the i2c driver 
/// @param  void
void batt_driver_deinit(void)
{
  ESP_LOGW(TAG, "deiniting batt task");
  vTaskSuspend(batt_taskhandle);
  vTaskDelete(batt_taskhandle);
}

void batt_function_task(void* param) {

  for (;;) {
    // get the battery device id
    batt_data.device_id = get_fuelgauge(DevName);


    
    // return ((get_fuelgauge(AvgVcell))* 78.125f )/1000000;
    batt_data.V_cell = ((get_fuelgauge(AvgVcell)) * 78.125f) / 1000;
    ///// return the soc of the battery
    batt_data.soc = get_fuelgauge(RepSOC) / 256;

    batt_data.rem_cap = get_fuelgauge(ReCap) * 0.5;
    batt_data.time_till_empty = get_fuelgauge(TTE) * 5.625;

    //// calculate the current
    int16_t value = get_fuelgauge(battcurrent);

    // batt_data.curr_flow = ((value & 0x8000)) ? (-(((~(value - 1)) * 155.25) / 1000)) : ((value * 155.25) / 1000);
    batt_data.curr_flow = (value * 155.25) / 1000;
    /////////////// get the temerature of the fuel gauge
    value = get_fuelgauge(DieTemp);
    // if (value & 0x8000)
    //     value = ~(value - 1);
    batt_data.temp = value / 256;

    /////////// ic status
    batt_data.ic_status = get_fuelgauge(STATUS);

    //// to know abouut the chaarging status
    batt_data.charging_status = (batt_data.curr_flow > BATT_MIN_CHARGING_CURRENT) ? (BATT_CHARGING) : BATT_DISCHARGING;
 

    // also send battery data in this task
    batt_send_data();
    
    delay(battery_state_change_update_time);
    
  }
  //// never reachs here
  vTaskDelete(NULL);
}


void batt_send_data(void)
{
    /// @brief this is to get the batttery soc monitor and charging status monitor
  static volatile uint8_t batt_soc = 0;
  static volatile uint8_t batt_chg_status = 0;

    bool send_data = false;

    if (batt_chg_status != batt_data.charging_status) {
      //// update the global status
      batt_chg_status = batt_data.charging_status;
      send_data = true;
    }

    /////////// get the soc change status
    if (batt_soc != batt_data.soc) {
      /// update the global soc
      batt_soc = batt_data.soc;
      send_data = true;
    }

     /////////// check that is we have to send te data
    if (send_data == true) {
      uint8_t buff[2] = {batt_soc, batt_chg_status};
      ////// send the data through ble
      esp_ble_send_battery_data(buff, 2);
    }

}


/// @brief get the battery voltage 
/// @param  void 
/// @return batt voltage in millivolt 
uint16_t batt_get_voltage(void)
{
  return batt_data.V_cell;
}

/// @brief get the state of charge from the battery 
/// @param  void 
/// @return soc 
uint8_t batt_get_soc(void)
{
  return batt_data.soc;
}

/// @brief get the current from the battery 
/// @param  void 
/// @return battery current 
int16_t batt_get_current(void)
{
  return batt_data.curr_flow;
}

/// @brief get the charging status of the battery 
/// @param  void 
/// @return chg/discharging 
uint16_t batt_get_chg_status(void)
{
  return batt_data.charging_status;
}


/// @brief this is to verify that fuel gauge is present and working
/// @param  void
/// @return succ/errcode
uint32_t batt_verify_component(void)
{
  //// get the device fuel gauge id
  // read the device id from the 
  if (batt_data.device_id != FUEL_GAUGE_DEV_ID) {
    return ERR_BATT_HARDWARE_FAULT;
  }
  return ESP_OK;
}

/// @brief get the battery custom data 
/// @param  batt_data_struct_t pointer 
/// @return err if driver is not init 
esp_err_t batt_get_data(batt_data_struct_t * in_buff)
{
    if (batt_data.device_id != FUEL_GAUGE_DEV_ID) {
      memset(in_buff,0,BATT_DATA_STRUCT_SIZE);
    return ERR_BATT_HARDWARE_FAULT;
  }
  memcpy(in_buff, &batt_data, BATT_DATA_STRUCT_SIZE);
  return ESP_OK;
}



