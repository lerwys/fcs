//============================================================================
// Author      : Andrzej Wojenski
// Version     : 1.0
// Description : Software driver for configuration FMC ADC 130M 4CH card (ACTIVE version)
//               Also checks and tests avaiable chips on the board
//         Includes:
//               - firmware identification
//         - LED configuration
//               - trigger configuration
//               - LM75A configuration and data acquisition (temperature sensor)
//               - EEPROM 24A64 check
//               - Si571 clock distribution chip configuration (runs with 125MHz clock)
//               - AD9510 configuration (clock distribution)
//               - LTC2208 configuration (4 ADC chips)
//               - Clock and data lines calibation (IDELAY)
//               - most of the functions provides assertions to check if data is written to the chip (checks readback value)
//============================================================================
#include "reg_map/fmc_config_130m_4ch.h"

#include <iostream>
#include <unistd.h>  /* getopt */

#include "data.h"
#include "commlink/commLink.h"
#include "wishbone/rs232_syscon.h"
#include "interface/i2c.h"
#include "interface/spi.h"
#include "interface/gpio.h"
#include "chip/si570.h"
#include "chip/ad9510.h"
#include "chip/isla216p.h"
#include "chip/amc7823.h"
#include "chip/eeprom_24a64.h"
#include "chip/lm75a.h"
#include "platform/fmc130m_plat.h"

#include "config.h"

using namespace std;

int main(int argc, const char **argv) {

  cout << "FMC configuration software for FMC ADC 130M 4CH card (ACTIVE version)" << endl <<
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
          delay_l = fmc_130m_ml605_delay_l;
          platform_name = ML605_STRING;
          break;
        case KC705:
          delay_l = fmc_130m_kc705_delay_l;
          platform_name = KC705_STRING;
          break;
        case AFC:
          delay_l = fmc_130m_afc_delay_l;
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

  int_drv = _commLink->regIntDrv(SI571_I2C_DRV, FPGA_SI571_I2C, new i2c_int());
  ((i2c_int*)int_drv)->i2c_init(FPGA_SYS_FREQ, 100000); // 100kHz

  int_drv = _commLink->regIntDrv(AD9510_SPI_DRV, FPGA_AD9510_SPI, new spi_int());
  ((spi_int*)int_drv)->spi_init(FPGA_SYS_FREQ, 1000000, 0x2400); // 1MHZ, ASS = 1,
  //TX_NEG = 1 (data changed on falling edge), RX_NEG = 0 (data latched on rising edge)

  int_drv = _commLink->regIntDrv(EEPROM_I2C_DRV, FPGA_EEPROM_I2C, new i2c_int());
  ((i2c_int*)int_drv)->i2c_init(FPGA_SYS_FREQ, 400000); // 400kHz

  int_drv = _commLink->regIntDrv(LM75A_I2C_DRV, FPGA_LM75A_I2C, new i2c_int());
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
  //sleep(4);

  data.data_send[0] = 0x04;
  _commLink->fmc_config_send(&data);
  //sleep(4);

  data.data_send[0] = 0x08;
  _commLink->fmc_config_send(&data);
  //sleep(4);

  // Check if data properly written
  _commLink->fmc_config_read(&data);
  assert( (data.data_read[0] & 0x0E) == 0x08); // ignore TEMP_ALARM pin

  data.data_send[0] = 0x0E;
  _commLink->fmc_config_send(&data);
  //sleep(4);

  // Set status config (blue LED)
  data.data_send[0] = 0x02;
  _commLink->fmc_config_send(&data);

  // for trigger test
  data.data_send[0] = 0x00;
  _commLink->fmc_config_send(&data);

  // ======================================================
  //                  Trigger configuration
  // ======================================================
  cout << "============================================" << endl <<
      "            Trigger configuration           " << endl <<
      "============================================" << endl;

  data.wb_addr = FPGA_CTRL_REGS | WB_TRG_CTRL; // trigger control
  data.data_send[0] = 0x01;
  _commLink->fmc_config_send(&data);

  // ======================================================
  //        LM75A configuration (temperature monitor)
  // ======================================================
  cout << "============================================" << endl <<
      "             LM75A check data    " << endl <<
      "============================================" << endl;
  // lm75 i2c have timeout

  LM75A_drv::LM75A_setCommLink(_commLink, LM75A_I2C_DRV);
  printf("LM75A chip number 1, chip ID: 0x%02x (should be 0xA1)\n", LM75A_drv::LM75A_readID(LM75A_ADDR_1));
  printf("LM75A chip number 1, temperature: %f *C\n", LM75A_drv::LM75A_readTemp(LM75A_ADDR_1));
  printf("LM75A chip number 2, chip ID: 0x%02x (should be 0xA1)\n", LM75A_drv::LM75A_readID(LM75A_ADDR_2));
  printf("LM75A chip number 2, temperature: %f *C\n", LM75A_drv::LM75A_readTemp(LM75A_ADDR_2));

  // ======================================================
  //                  EEPROM configuration
  // ======================================================
  cout << "============================================" << endl <<
      "              EEPROM check    " << endl <<
      "============================================" << endl;

  EEPROM_drv::EEPROM_setCommLink(_commLink, EEPROM_I2C_DRV);
  EEPROM_drv::EEPROM_switch(0x02); // according to documentation, switches to i2c fmc lines
  //EEPROM_drv::EEPROM_sendData(0x00); // wrong address
  EEPROM_drv::EEPROM_sendData(EEPROM_ADDR); // good address

  data.data_send.clear();
  data.data_read.clear();

  // ======================================================
  //      AD9510 configuration (clock distribution)
  // ======================================================
  cout << "============================================" << endl <<
      "     AD9510 config (clock distribution)     " << endl <<
      "============================================" << endl;

  // reset chip
  data.wb_addr = FPGA_CTRL_REGS | WB_CLK_CTRL; // clock control
  _commLink->fmc_config_read(&data);

  data.wb_addr = FPGA_CTRL_REGS | WB_CLK_CTRL;
  data_temp = data.data_read[0] | 0x02; // pull high
  data.data_send[0] = data_temp;
  _commLink->fmc_config_send(&data);
  sleep(1);

  data_temp = data.data_read[0] & 0xFFFFFFFD; // pull low
  data.data_send[0] = data_temp;
  _commLink->fmc_config_send(&data);
  sleep(1);

  // turn off reset
  data.wb_addr = FPGA_CTRL_REGS | WB_CLK_CTRL;
  data_temp = data.data_read[0] | 0x02; // pull high
  data.data_send[0] = data_temp;
  _commLink->fmc_config_send(&data);
  sleep(1);

  AD9510_drv::AD9510_setCommLink(_commLink, AD9510_SPI_DRV);

  // FPGA working with clock copy for ADC (FMC ADC 130M 4CH rev.1)
  //AD9510_drv::AD9510_config_si570_fmc_adc_130m_4ch(AD9510_ADDR); // with config check included
  AD9510_drv::AD9510_config_si570_pll_fmc_adc_130m_4ch(AD9510_ADDR); // with config check included

    // Check PLL lock
  
  data.wb_addr = FPGA_CTRL_REGS | WB_CLK_CTRL; // clock control
  _commLink->fmc_config_read(&data);
  printf("Clock PLL Lock: %d\n", data.data_read[0]);

  //exit(1);

  // ======================================================
  //                  LTC ADC configuration
  // ======================================================
  cout << "============================================" << endl <<
      "     LTC2208 config (4 ADC chips)     " << endl <<
      "============================================" << endl;

  data.wb_addr = FPGA_CTRL_REGS | WB_ADC_LTC_CTRL; // trigger control
  //data.data_send[0] = 0x02; // dither on, power on, random off, pga off (input 2.25 Vpp)
  data.data_send[0] = 0x00; // dither off, power on, random off, pga off (input 2.25 Vpp)
  _commLink->fmc_config_send(&data);

  // ======================================================
  //            Clock and data lines calibration
  // ======================================================
  cout << "============================================" << endl <<
      "      Clock and data lines calibration     " << endl <<
      "============================================" << endl;

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

  if ( (data.data_read[0] >> 2) & 0x0F ) // check adc0 adc1 adc2 adc3 idelayctrl
    cout << "All IDELAY controllers had been reset and are ready to work!" << endl;
  else {
    cout << "Error while resetting IDELAY controllers, some of them are not ready!" << endl;
    exit(1);
  }

  cout << "IDELAY lines calibration" << endl;

  // upload new tap value (130m)
  // for fmc adc 130m
  // adc0
  // adc1
  // adc2
  // adc3
  // tap resolution 78ps

  set_fpga_delay_s(_commLink, FPGA_CTRL_REGS | WB_IDELAY0_CAL, &delay_l[0]);

  //data.wb_addr = FPGA_CTRL_REGS | WB_IDELAY0_CAL;
  //data.data_send[0] = IDELAY_ALL_LINES | IDELAY_TAP(07) | IDELAY_UPDATE;
  //_commLink->fmc_config_send(&data);
  //usleep(1000);
  //data.data_send[0] = (IDELAY_ALL_LINES | IDELAY_TAP(07)) & 0xFFFFFFFE;
  //_commLink->fmc_config_send(&data);

  set_fpga_delay_s(_commLink, FPGA_CTRL_REGS | WB_IDELAY1_CAL, &delay_l[1]);

  //data.wb_addr = FPGA_CTRL_REGS | WB_IDELAY1_CAL;
  //data.data_send[0] = IDELAY_ALL_LINES | IDELAY_TAP(07) | IDELAY_UPDATE;
  //_commLink->fmc_config_send(&data);
  //usleep(1000);
  //data.data_send[0] = (IDELAY_ALL_LINES | IDELAY_TAP(07)) & 0xFFFFFFFE;
  //_commLink->fmc_config_send(&data);

  set_fpga_delay_s(_commLink, FPGA_CTRL_REGS | WB_IDELAY2_CAL, &delay_l[2]);

  //data.wb_addr = FPGA_CTRL_REGS | WB_IDELAY2_CAL;
  //data.data_send[0] = IDELAY_ALL_LINES | IDELAY_TAP(07) | IDELAY_UPDATE;
  //_commLink->fmc_config_send(&data);
  //usleep(1000);
  //data.data_send[0] = (IDELAY_ALL_LINES | IDELAY_TAP(07)) & 0xFFFFFFFE;
  //_commLink->fmc_config_send(&data);

  set_fpga_delay_s(_commLink, FPGA_CTRL_REGS | WB_IDELAY3_CAL, &delay_l[3]);

  //data.wb_addr = FPGA_CTRL_REGS | WB_IDELAY3_CAL;
  //data.data_send[0] = IDELAY_ALL_LINES | IDELAY_TAP(07) | IDELAY_UPDATE;
  //_commLink->fmc_config_send(&data);
  //usleep(1000);
  //data.data_send[0] = (IDELAY_ALL_LINES | IDELAY_TAP(07)) & 0xFFFFFFFE;
  //_commLink->fmc_config_send(&data);

  // train communication links

  cout << "All done! All components on the FMC card had been configured and tested!" << endl <<
      "FMC card is ready to work!" << endl;

  return 0;
}
