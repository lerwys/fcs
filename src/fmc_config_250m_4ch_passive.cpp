//============================================================================
// Author      : Andrzej Wojenski
// Version     : 1.0
// Description : Software driver for configuration FMC ADC 250M 4CH card (PASSIVE version)
//               Also checks and tests avaiable chips on the board
//         Includes:
//               - firmware identification
//         - LED configuration
//               - trigger configuration
//               - AMC7823 configuration and data acquisition (temperature monitor)
//               - EEPROM 24A64 check
//               - ISLA216P configuration (4 ADC chips)
//               - Clock and data lines calibation (IDELAY)
//               - most of the functions provides assertions to check if data is written to the chip (checks readback value)
//============================================================================
#include "reg_map/fmc_config_250m_4ch.h"

#include <iostream>
#include <unistd.h>  /* getopt */

#include "data.h"
#include "commLink.h"
#include "wishbone/rs232_syscon.h"
#include "interface/i2c.h"
#include "interface/spi.h"
#include "interface/gpio.h"
#include "chip/si570.h"
#include "chip/ad9510.h"
#include "chip/isla216p.h"
#include "chip/amc7823.h"
#include "chip/eeprom_24a64.h"
#include "platform/fmc250m_plat.h"

using namespace std;

int main(int argc, const char **argv) {

  cout << "FMC configuration software for FMC ADC 250M 4CH card (PASSIVE version)" << endl <<
      "Author: Andrzej Wojenski" << endl;

  WBInt_drv* int_drv;
  wb_data data;
  vector<uint16_t> amc_temp;
  vector<uint16_t> test_pattern;
  commLink* _commLink = new commLink();
  uint32_t data_temp;
  int opt, error;

  /* Default command-line arguments */
  program = argv[0];
  quiet = 0;
  verbose = 0;
  error = 0;
  platform = PLATFORM_NOT_SET;

  /* Process the command-line arguments */
  while ((opt = getopt(argc, (char **)argv, "p:vqh")) != -1) {
    switch (opt) {
    case 'p':
      if (strlen(optarg) > MAX_PLATFORM_SIZE_ID) {
        fprintf(stderr, "%s: platform id length too big -- '%s'\n", program, optarg);
        return 1;
      }

      // Simple lookup table for checking the platform name (case insensitive)
      switch (platform = lookupstring_i(optarg)) {
        case ML605:
          delay_l = fmc_250m_ml605_delay_l;
          platform_name = ML605_STRING;
          break;
        case KC705:
          delay_l = fmc_250m_kc705_delay_l;
          platform_name = KC705_STRING;
          break;
        case AFC:
          delay_l = fmc_250m_afc_delay_l;
          platform_name = AFC_STRING;
          break;
        case BAD_PLATFORM:
          fprintf(stderr, "%s: invalid platform -- '%s'\n", program, optarg);
          platform_name = NOPLAT_STRING;  // why care?
          return 1;
      }
      break;
    case 'v':
      verbose = 1;
      break;
    case 'q':
      quiet = 1;
      break;
    case 'h':
      help();
      return 1;
    case ':':
    case '?':
      error = 1;
      break;
    default:
      fprintf(stderr, "%s: bad option\n", program);
      help();
      return 1;
    }
  }

  if (error) return 1;

  if (platform == PLATFORM_NOT_SET) {
    fprintf(stderr, "%s: platform not set!\n", program);
    return 1;
  }

  fprintf(stdout, "%s: platform (%s) set!\n", program, platform_name);

  data.data_send.resize(10);
  data.extra.resize(2);

  // CommLink configuration
  // Adding communication interfaces
  _commLink->regWBMaster(new rs232_syscon_driver());

  int_drv = _commLink->regIntDrv(ISLA_SPI_DRV, FPGA_ISLA_SPI, new spi_int());
  ((spi_int*)int_drv)->spi_init(FPGA_SYS_FREQ, 1000000, 0x2400); // 10MHZ, ASS = 1,
  //TX_NEG = 1 (data changed on falling edge), RX_NEG = 0 (data latched on rising edge)

  int_drv = _commLink->regIntDrv(SI571_I2C_DRV, FPGA_SI571_I2C, new i2c_int());
  ((i2c_int*)int_drv)->i2c_init(FPGA_SYS_FREQ, 400000); // 400kHz

  int_drv = _commLink->regIntDrv(AD9510_SPI_DRV, FPGA_AD9510_SPI, new spi_int());
  ((spi_int*)int_drv)->spi_init(FPGA_SYS_FREQ, 1000000, 0x2400); // 1MHZ, ASS = 1,
  //TX_NEG = 1 (data changed on falling edge), RX_NEG = 0 (data latched on rising edge)

  int_drv = _commLink->regIntDrv(AMC7823_SPI_DRV, FPGA_AMC7823_SPI, new spi_int());
  //((spi_int*)int_drv)->spi_init(FPGA_SYS_FREQ, 20000000, 0x2200); // 20MHZ, ASS = 1,
  ((spi_int*)int_drv)->spi_init(FPGA_SYS_FREQ, 1000000, 0x2200); // 20MHZ, ASS = 1,
  //TX_NEG = 0 (data changed on rising edge), RX_NEG = 1 (data latched on falling edge)

  int_drv = _commLink->regIntDrv(EEPROM_I2C_DRV, FPGA_EEPROM_I2C, new i2c_int());
  ((i2c_int*)int_drv)->i2c_init(FPGA_SYS_FREQ, 400000); // 400kHz

  _commLink->regIntDrv(GENERAL_GPIO_DRV, FPGA_CTRL_REGS, new gpio_int());

  // ======================================================
  //                Firmware identification
  // ======================================================

  cout << "============================================" << endl <<
      "            Firmware identification         " << endl <<
      "============================================" << endl;

  data.wb_addr = FPGA_CTRL_REGS | WB_FMC_STATUS; // FMC status register (HW address)

  // check id data
  _commLink->fmc_config_read(&data);

  cout << "Reg: " << hex << (data.data_read[0]) << endl;
  cout << "Firmware ID: " << hex << (data.data_read[0] >> 3) <<
      "(" << dec << (data.data_read[0] >> 3)<< ")" << endl; // should be 0x01332A11 (20130321)

  // Check communication
  assert(0x01332A11 == (data.data_read[0] >> 3) );

  // ======================================================
  //                  LEDs configuration
  // ======================================================

  cout << "============================================" << endl <<
      "            LEDs configuration              " << endl <<
      "============================================" << endl;

  data.wb_addr = FPGA_CTRL_REGS | WB_MONITOR_CTRL; // monitor register (HW address)

  data.data_send[0] = 0x02;
  _commLink->fmc_config_send(&data);
  //sleep(1);

  data.data_send[0] = 0x04;
  _commLink->fmc_config_send(&data);
  //sleep(1);

  data.data_send[0] = 0x08;
  _commLink->fmc_config_send(&data);
  //sleep(1);

  // Check if data properly written
  _commLink->fmc_config_read(&data);
  assert( (data.data_read[0] & 0x0E) == 0x08); // ignore DAV pin

  data.data_send[0] = 0x0E;
  _commLink->fmc_config_send(&data);
  //sleep(1);

  // Set status config (blue LED)
  data.data_send[0] = 0x02;
  _commLink->fmc_config_send(&data);

  data.data_send[0] = 0x00;
  _commLink->fmc_config_send(&data);

  // ======================================================
  //                  Trigger configuration
  // ======================================================
  cout << "============================================" << endl <<
      "            Trigger configuration           " << endl <<
      "============================================" << endl;

  data.wb_addr = FPGA_CTRL_REGS | WB_TRG_CTRL; // trigger control, input mode, no termination
  data.data_send[0] = 0x01;
  _commLink->fmc_config_send(&data);

  /*
  cout << "============================================" << endl <<
      "                EEPROM check           " << endl <<
      "============================================" << endl;
  // check eeprom
  EEPROM_drv::EEPROM_setCommLink(_commLink, EEPROM_I2C_DRV);
  EEPROM_drv::EEPROM_switch(0x02); // according to documentation, switches to i2c fmc lines
  //EEPROM_drv::EEPROM_sendData(0x00); // wrong address
  EEPROM_drv::EEPROM_sendData(0x50); // good address
  */
//
//  // ======================================================
//  //      AMC7823 configuration (temperature monitor)
//  // ======================================================
  cout << "============================================" << endl <<
      " AMC7823 configuration (temperature monitor)" << endl <<
      "============================================" << endl;
  AMC7823_drv::AMC7823_setCommLink(_commLink, AMC7823_SPI_DRV, GENERAL_GPIO_DRV);
  AMC7823_drv::AMC7823_checkReset(AMC7823_ADDR);

  AMC7823_drv::AMC7823_config(AMC7823_ADDR);
  AMC7823_drv::AMC7823_powerUp(AMC7823_ADDR);

  amc_temp = AMC7823_drv::AMC7823_getADCData(FPGA_CTRL_REGS | WB_MONITOR_CTRL, AMC7823_ADDR);

  cout << "Temperature monitor (on-chip): " << AMC7823_drv::AMC7823_tempConvert(amc_temp[4]) << endl;

  //exit(1);

  // ======================================================
  //                 ISLA216P25 (ADC chips)
  // ======================================================
  cout << "============================================" << endl <<
      "       ISLA216P25 (ADC chips) config        " << endl <<
      "============================================" << endl;

  ISLA216P_drv::ISLA216P_setCommLink(_commLink, ISLA_SPI_DRV, GENERAL_GPIO_DRV);

  // power-on calibration (500ms, reset pin)
  ISLA216P_drv::ISLA216P_sleep(FPGA_CTRL_REGS | WB_ADC_ISLA_CTRL, 0x00); // turn off sleep

  // Resetting /autocalbiration procedure
  ISLA216P_drv::ISLA216P_AutoCalibration(FPGA_CTRL_REGS | WB_ADC_ISLA_CTRL);

  // first configure ISLA to enable four-wire mode (enable SDO output)
  ISLA216P_drv::ISLA216P_config(ISLA_ADC0_ADDR);
  ISLA216P_drv::ISLA216P_config(ISLA_ADC1_ADDR);
  ISLA216P_drv::ISLA216P_config(ISLA_ADC2_ADDR);
  ISLA216P_drv::ISLA216P_config(ISLA_ADC3_ADDR);

  //ISLA216P_drv::ISLA216P_spi_write(ISLA_ADC0_ADDR, 0x00, 0x80); // turn on four wire mode

  // check if autocalibration is done by SPI (could be also done by ADC output regs)
  ISLA216P_drv::ISLA216P_checkCalibration(ISLA_ADC0_ADDR);
  ISLA216P_drv::ISLA216P_checkCalibration(ISLA_ADC1_ADDR);
  ISLA216P_drv::ISLA216P_checkCalibration(ISLA_ADC2_ADDR);
  ISLA216P_drv::ISLA216P_checkCalibration(ISLA_ADC3_ADDR);

  ISLA216P_drv::ISLA216P_sync(FPGA_CTRL_REGS | WB_ADC_ISLA_CTRL);

  // Check communication with all ADC
  cout << "ISLA216P25 chip ID: " << ISLA216P_drv::ISLA216P_getChipID(ISLA_ADC0_ADDR) << " version: "
      << ISLA216P_drv::ISLA216P_getChipVersion(ISLA_ADC0_ADDR) << endl;
  cout << "ISLA216P25 temp: " << ISLA216P_drv::ISLA216P_getTemp(ISLA_ADC0_ADDR) << endl;

  cout << "ISLA216P25 chip ID: " << ISLA216P_drv::ISLA216P_getChipID(ISLA_ADC1_ADDR) << " version: "
      << ISLA216P_drv::ISLA216P_getChipVersion(ISLA_ADC1_ADDR) << endl;
  cout << "ISLA216P25 temp: " << ISLA216P_drv::ISLA216P_getTemp(ISLA_ADC1_ADDR) << endl;

  cout << "ISLA216P25 chip ID: " << ISLA216P_drv::ISLA216P_getChipID(ISLA_ADC2_ADDR) << " version: "
      << ISLA216P_drv::ISLA216P_getChipVersion(ISLA_ADC2_ADDR) << endl;
  cout << "ISLA216P25 temp: " << ISLA216P_drv::ISLA216P_getTemp(ISLA_ADC2_ADDR) << endl;

  cout << "ISLA216P25 chip ID: " << ISLA216P_drv::ISLA216P_getChipID(ISLA_ADC3_ADDR) << " version: "
      << ISLA216P_drv::ISLA216P_getChipVersion(ISLA_ADC3_ADDR) << endl;

  cout << "ISLA216P25 temp: " << ISLA216P_drv::ISLA216P_getTemp(ISLA_ADC3_ADDR) << endl;

  // set test pattern
  // mode:
  // user pattern, user pattern 0 only
  cout << "Setting test pattern" << endl;
  test_pattern.resize(4);

  // config for 200MHz output!!!

  //test_pattern[0] = 0xAAAA; // 9999
  test_pattern[0] = 0x1234;
  test_pattern[1] = 0x5678;
  test_pattern[2] = 0x9ABC;
  test_pattern[3] = 0xDEF1;
  ISLA216P_drv::ISLA216P_setTestPattern(ISLA_ADC0_ADDR, 0x83, test_pattern);
  //ISLA216P_drv::ISLA216P_setTestPattern(ISLA_ADC0_ADDR, 0x20, test_pattern);
  //sleep(7);
  test_pattern[0] = 0x1111;
  test_pattern[1] = 0x2222;
  test_pattern[2] = 0x3333;
  test_pattern[3] = 0x4444;
  ISLA216P_drv::ISLA216P_setTestPattern(ISLA_ADC1_ADDR, 0x83, test_pattern);

  test_pattern[0] = 0xEDF0;
  test_pattern[1] = 0x4567;
  test_pattern[2] = 0x1234;
  test_pattern[3] = 0x9876;
  ISLA216P_drv::ISLA216P_setTestPattern(ISLA_ADC2_ADDR, 0x83, test_pattern);

  test_pattern[0] = 0x5555;
  test_pattern[1] = 0x6666;
  test_pattern[2] = 0x7777;
  test_pattern[3] = 0x8888;
  ISLA216P_drv::ISLA216P_setTestPattern(ISLA_ADC3_ADDR, 0x83, test_pattern);

  // check test pattern configuration
/*
  ISLA216P_drv::ISLA216P_assert(ISLA_ADC0_ADDR, 0xC1, 0x34);
  ISLA216P_drv::ISLA216P_assert(ISLA_ADC0_ADDR, 0xC2, 0x12);

  ISLA216P_drv::ISLA216P_assert(ISLA_ADC1_ADDR, 0xC1, 0x78);
  ISLA216P_drv::ISLA216P_assert(ISLA_ADC1_ADDR, 0xC2, 0x56);

  ISLA216P_drv::ISLA216P_assert(ISLA_ADC2_ADDR, 0xC1, 0xBC);
  ISLA216P_drv::ISLA216P_assert(ISLA_ADC2_ADDR, 0xC2, 0x9A);

  ISLA216P_drv::ISLA216P_assert(ISLA_ADC3_ADDR, 0xC1, 0xF1);
  ISLA216P_drv::ISLA216P_assert(ISLA_ADC3_ADDR, 0xC2, 0xDE);

  ISLA216P_drv::ISLA216P_assert(ISLA_ADC0_ADDR, 0xC0, 0x80);
  ISLA216P_drv::ISLA216P_assert(ISLA_ADC1_ADDR, 0xC0, 0x80);
  ISLA216P_drv::ISLA216P_assert(ISLA_ADC2_ADDR, 0xC0, 0x80);
  ISLA216P_drv::ISLA216P_assert(ISLA_ADC3_ADDR, 0xC0, 0x80);*/

  // config done, reset IDELAYCTRL blocks

  // Calibration
  // reset IDELAYCTRLs in FPGA
  data.wb_addr = FPGA_CTRL_REGS | WB_FPGA_CTRL;
  data.data_send[0] = 0x01;
  _commLink->fmc_config_send(&data);
  sleep(1);
  data.data_send[0] = 0x00;
  _commLink->fmc_config_send(&data);

  // check if ready
  _commLink->fmc_config_read(&data);
  printf("data: %08x\n", data.data_read[0]);

  if ( (data.data_read[0] >> 2) & 0x07 ) // now checks adc0 adc1 adc2
    cout << "All IDELAY controllers has been reset and are ready to work!" << endl;
  else {
    cout << "Error while resetting IDELAY controllers, some of them are not ready!" << endl;
    exit(1);
  }

  // train communication links
  // upload new tap value
  // for fmc adc 250m
  // adc0 -1.364
  // adc1 -0.705
  // adc2 -0.996
  // tap resolution 78ps

  set_fpga_delay_s(_commLink, FPGA_CTRL_REGS | WB_IDELAY0_CAL, &delay_l[0]);

  //data.wb_addr = FPGA_CTRL_REGS | WB_IDELAY0_CAL;
  //data.data_send[0] = IDELAY_ALL_LINES | IDELAY_TAP(18) | IDELAY_UPDATE; // should be 0x0050003f
  //_commLink->fmc_config_send(&data);
  //// check data
  //_commLink->fmc_config_read(&data);
  ////assert(data.data_read[0] == (IDELAY_ALL_LINES | IDELAY_TAP(18) | IDELAY_UPDATE));
  //usleep(1000);
  //data.data_send[0] = (IDELAY_ALL_LINES | IDELAY_TAP(18)) & 0xFFFFFFFE; // should be 0x0050003f
  //_commLink->fmc_config_send(&data);

  set_fpga_delay_s(_commLink, FPGA_CTRL_REGS | WB_IDELAY1_CAL, &delay_l[1]);

  //data.wb_addr = FPGA_CTRL_REGS | WB_IDELAY1_CAL;
  //data.data_send[0] = IDELAY_ALL_LINES | IDELAY_TAP(18) | IDELAY_UPDATE; // should be 0x0050003f
  //_commLink->fmc_config_send(&data);
  //// check data
  //_commLink->fmc_config_read(&data);
  ////assert(data.data_read[0] == (IDELAY_ALL_LINES | IDELAY_TAP(9) | IDELAY_UPDATE));
  //usleep(1000);
  //data.data_send[0] = (IDELAY_ALL_LINES | IDELAY_TAP(18)) & 0xFFFFFFFE; // should be 0x0050003f
  //_commLink->fmc_config_send(&data);

  set_fpga_delay_s(_commLink, FPGA_CTRL_REGS | WB_IDELAY2_CAL, &delay_l[2]);

  //data.wb_addr = FPGA_CTRL_REGS | WB_IDELAY2_CAL;
  //data.data_send[0] = IDELAY_ALL_LINES | IDELAY_TAP(18) | IDELAY_UPDATE; // should be 0x0050003f
  //_commLink->fmc_config_send(&data);
  //// check data
  //_commLink->fmc_config_read(&data);
  ////assert(data.data_read[0] == (IDELAY_ALL_LINES | IDELAY_TAP(13) | IDELAY_UPDATE));
  //usleep(1000);
  //data.data_send[0] = (IDELAY_ALL_LINES | IDELAY_TAP(18)) & 0xFFFFFFFE; // should be 0x0050003f
  //_commLink->fmc_config_send(&data);

  set_fpga_delay_s(_commLink, FPGA_CTRL_REGS | WB_IDELAY3_CAL, &delay_l[3]);

  //data.wb_addr = FPGA_CTRL_REGS | WB_IDELAY3_CAL;
  //data.data_send[0] = IDELAY_ALL_LINES | IDELAY_TAP(18) | IDELAY_UPDATE; // should be 0x0050003f
  //_commLink->fmc_config_send(&data);
  //// check data
  //_commLink->fmc_config_read(&data);
  ////assert(data.data_read[0] == (IDELAY_ALL_LINES | IDELAY_TAP(13) | IDELAY_UPDATE));
  //usleep(1000);
  //data.data_send[0] = (IDELAY_ALL_LINES | IDELAY_TAP(18)) & 0xFFFFFFFE; // should be 0x0050003f
  //_commLink->fmc_config_send(&data);


  // turn off test pattern
  cout << "Check test pattern" << endl;
  //sleep(90);


  cout << "Test pattern off" << endl;

  ISLA216P_drv::ISLA216P_setTestPattern(ISLA_ADC0_ADDR, 0x00, test_pattern);
  ISLA216P_drv::ISLA216P_setTestPattern(ISLA_ADC1_ADDR, 0x00, test_pattern);
  ISLA216P_drv::ISLA216P_setTestPattern(ISLA_ADC2_ADDR, 0x00, test_pattern);
  ISLA216P_drv::ISLA216P_setTestPattern(ISLA_ADC3_ADDR, 0x00, test_pattern);


  cout << "All done! All components on the FMC card had been configured and tested!" << endl <<
      "FMC card is ready to work!" << endl;

  return 0;
}
