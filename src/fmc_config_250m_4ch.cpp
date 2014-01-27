//============================================================================
// Author      : Andrzej Wojenski
// Version     : 1.0
// Description : Software driver for configuration FMC ADC 250M 4CH card (ACTIVE version)
//               Also checks and tests avaiable chips on the board
//         Includes:
//               - firmware identification
//         - LED configuration
//               - trigger configuration
//               - AMC7823 configuration and data acquisition (temperature monitor)
//               - EEPROM 24A64 WITHOUT check (I2C from CPU)
//               - Si571 clock distribution chip configuration (runs with 125MHz clock)
//               - AD9510 configuration (clock distribution)
//               - ISLA216P configuration (4 ADC chips)
//               - Clock and data lines calibation (IDELAY)
//               - most of the functions provides assertions to check if data is written to the chip (checks readback value)
//============================================================================
#include "reg_map/fmc_config_250m_4ch.h"

#include <iostream>
#include <unistd.h>  /* getopt */

#include "config.h"
#include "plat_opts.h"
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

  cout << "FMC configuration software for FMC ADC 250M 4CH card (ACTIVE version) AMC Artix7 Version" << endl <<
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
          delay_data_l = fmc_250m_ml605_delay_l;
          platform_name = ML605_STRING;
          break;
        case KC705:
          delay_data_l = fmc_250m_kc705_delay_l;
          platform_name = KC705_STRING;
          break;
        case AFC:
          delay_data_l = fmc_250m_afc_delay_l;
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
  //          Si571 configuration (clock generation)
  // ======================================================
  // Set Si571 to f_out = 250MHz
  // data generated with Si570 software (from Si www)
  // part number:
  // 571
  // AJC000337G, rev. D
  // parameters:
  // OE - active high, 3.3V, LVPECL
  // 10-280MHz freq. range
  // startup freq:  155.4857 MHz (previously 155.49MHz, previously 155.52 MHz)
  // I2C addr: 0x49
  // Startup registers values:
  // reg 7 = 0x01      // 0000 0001 -> HS = 000 -> HS = 4d
  // reg 8 = 0xC2      // 1100 0010 -> N1 = 000 0111 -> N1 = 8d
  // reg 9 = 0xB9      // 1011 1001
  // reg 10 = 0x59     // 0101 1001
  // reg 11 = 0xBE     // 1011 1110
  // reg 12 = 0xA4     // 1010 0100
  // are those default values correct?

  // fDCO_current = 4975.5867616 GHz (previously 4.97568, previously 4.97664 GHz)
  // RFreq = 00 0010 1011 1001 0101 1001 1011 1110 1010 0100 = 11699601060d
  // RFreq = 11699601060d / 2^28 = 43.584410325
  // fxtal = 114.260813135916 MHz ( previously 114.262954 previously 114.285 MHz )

  // fxtal2 = 114.158763716164 MHz ( previously 114.262954 previously 114.285 MHz )

  cout << "============================================" << endl <<
      "   Si571 configuration (clock generation)   " << endl <<
      "============================================" << endl;

  data.extra[0] = SI571_ADDR;
  data.extra[1] = 6;
  //data.data_send[0] = 0x07; // starting register
  data.data_send.clear();
  data.data_read.clear();

  Si570_drv::si570_setCommLink(_commLink, SI571_I2C_DRV, GENERAL_GPIO_DRV);

  Si570_drv::si570_outputDisable(FPGA_CTRL_REGS | WB_CLK_CTRL);

  //Si570_drv::si570_read_freq(&data);
  //exit(1);

  // Configuration for 130MHz output
  /*
  data.data_send.clear();
  data.data_send.push_back(0x02); // reg 7
  data.data_send.push_back(0x42); // reg 8
  data.data_send.push_back(0xD8); // reg 9
  data.data_send.push_back(0x01); // reg 10
  data.data_send.push_back(0x2A); // reg 11
  data.data_send.push_back(0x30); // reg 12
  */
  // Configuration for 125MHz output

/*
  data.data_send.clear();
  data.data_send.push_back(0x21); // reg 7
  data.data_send.push_back(0xC2); // reg 8
  data.data_send.push_back(0xBC); // reg 9
  data.data_send.push_back(0x01); // reg 10
  data.data_send.push_back(0x1E); // reg 11
  data.data_send.push_back(0xB8); // reg 12
*/

  // Configuration for 100MHz output

/*
  //HS = 001 -> HS =  5d
  //N1 = 000 1001 -> N1 = 10d
  // RFreq = 43.750273 -> RFreq = 43.750273*2^28 = 11744124482 = 2bc011e42
  data.data_send.clear();
  data.data_send.push_back(0x22); // 0010 0010
  data.data_send.push_back(0x42); // 0100 0010
  data.data_send.push_back(0xBC);
  data.data_send.push_back(0x01);
  data.data_send.push_back(0x1E);
  //data.data_send.push_back(0xB8);
  data.data_send.push_back(0x42);
*/

  // Configuration for 117.963900MHz output
/*
  //HS = 011 -> HS = 7d
  //N1 = 000 0101 -> N1 = 6d
  // RFreq = 43.360368576 -> RFreq = 43.360368576*2^28 = 11639460311 = 2B5C411d7h
  data.data_send.clear();
  data.data_send.push_back(0x61); // 0110 0001
  data.data_send.push_back(0x42); // 0100 0010
  data.data_send.push_back(0xB5);
  data.data_send.push_back(0xC4);
  data.data_send.push_back(0x11);
  data.data_send.push_back(0xD7);
*/

  // Configuration for 122.682456 MHz output
/*
  //HS = 001 -> HS = 5d
  //N1 = 000 0111 -> N1 = 8d
  // RFreq = 42.947412685 -> RFreq = 42.947412685*2^28 = 11528608308 = 2AF289A34h
  data.data_send.clear();
  data.data_send.push_back(0x21); // 0010 0001
  data.data_send.push_back(0xC2); // 1100 0010
  data.data_send.push_back(0xAF);
  data.data_send.push_back(0x28);
  data.data_send.push_back(0x9A);
  data.data_send.push_back(0x34);
*/

  // Configuration for 112.583175675676 MHz MHz output
/*
  //HS = 111 -> HS = 11d
  //N1 = 000 0011 -> N1 = 4d
  // RFreq = 43.35315652464 -> RFreq = 43.35315652464*2^28 = 11637524341 = 2B5A68775h
  data.data_send.clear();
  data.data_send.push_back(0xE0); // 1110 0000
  data.data_send.push_back(0xC2); // 1100 0010
  data.data_send.push_back(0xB5);
  data.data_send.push_back(0xA6);
  data.data_send.push_back(0x87);
  data.data_send.push_back(0x75);
*/
  // Configuration for 124.997588 MHz output
/*
  //HS = 001 -> HS = 5d
  //N1 = 000 0111 -> N1 = 8d
  // RFreq = 43.7578702892628 -> RFreq = 43.7578702892628*2^28 = 11746163865 = 2BC203C99h
  data.data_send.clear();
  data.data_send.push_back(0x21); // 0010 0001
  data.data_send.push_back(0xC2); // 1100 0010
  data.data_send.push_back(0xBC);
  data.data_send.push_back(0x20);
  data.data_send.push_back(0x3C);
  data.data_send.push_back(0x99);
*/
/*
  // Configuration for 113.529121545 MHz output.
  //HS = 111 -> HS = 11d
  //N1 = 000 0011 -> N1 = 4d
  // RFreq = 43.7174182279586 -> RFreq = 43.7174182279586*2^28 = 11735305097 = 2BB7A8B89h
  data.data_send.clear();
  data.data_send.push_back(0xE0); // 1110 0000
  data.data_send.push_back(0xC2); // 1100 0010
  data.data_send.push_back(0xBB);
  data.data_send.push_back(0x7A);
  data.data_send.push_back(0x8B);
  data.data_send.push_back(0x89);
*/
  // Configuration for 113.750000 MHz output. NOT LOCKING
/*
  //HS = 010 -> HS = 6d
  //N1 = 000 0111 -> N1 = 8d
  // RFreq = 47.7845164059035 -> RFreq = 47.7845164059035*2^28 = 12827058451d = 2FC8D6113h
  data.data_send.clear();
  data.data_send.push_back(0x41); // 0100 0001
  data.data_send.push_back(0xC2); // 1100 0010
  data.data_send.push_back(0xFC);
  data.data_send.push_back(0x8D);
  data.data_send.push_back(0x61);
  data.data_send.push_back(0x13);
*/

  // Configuration for 112 MHz MHz output.
/*
  //HS = 111 -> HS = 11d
  //N1 = 000 0011 -> N1 = 4d
  // RFreq = 43.128589166354 -> RFreq = 43.128589166354*2^28 = 11577242500 = 2B20EB384h
  data.data_send.clear();
  data.data_send.push_back(0xE0); // 1110 0000
  data.data_send.push_back(0xC2); // 1100 0010
  data.data_send.push_back(0xB2);
  data.data_send.push_back(0x0E);
  data.data_send.push_back(0xB3);
  data.data_send.push_back(0x84);
*/

  // Configuration for 114.222973 MHz output
/*
  //HS = 010 -> HS = 6d
  //N1 = 000 0111 -> N1 = 8d
  // RFreq = 47.983204635 -> RFreq = 47.983204635*2^28 = 12880393416 = 2FFBB34C8h
  data.data_send.clear();
  data.data_send.push_back(0x41); // 0100 0001
  data.data_send.push_back(0xC2); // 1100 0010
  data.data_send.push_back(0xFF);
  data.data_send.push_back(0xBB);
  data.data_send.push_back(0x34);
  data.data_send.push_back(0xC8);
*/

  // Configuration for 113.376415 MHz output
/*
  //HS = 111 -> HS = 11d
  //N1 = 000 0011 -> N1 = 4d
  // RFreq = 43.6586144972237 -> RFreq = 43.6586144972237*2^28 = 11719520091 = 2BA89AF5Bh
  data.data_send.clear();
  data.data_send.push_back(0xE0); // 1110 0000
  data.data_send.push_back(0xC2); // 1100 0010
  data.data_send.push_back(0xBA);
  data.data_send.push_back(0x89);
  data.data_send.push_back(0xAF);
  data.data_send.push_back(0x5B);
*/

  // Configuration for 208.927174 MHz output

  //HS = 010 -> HS = 6d
  //N1 = 000 0011 -> N1 = 4d
  // RFreq = 43.9234975289945 -> RFreq = 43.9234975289945*2^28 = 11790624088 = 2BEC6A558h
  data.data_send.clear();
  data.data_send.push_back(0x40); // 0100 0000
  data.data_send.push_back(0xC2); // 1100 0010
  data.data_send.push_back(0xBE);
  data.data_send.push_back(0xC6);
  data.data_send.push_back(0xA5);
  data.data_send.push_back(0x58);

  // Configuration for 75MHz output
/*
  data.data_send.clear();
  data.data_send.push_back(0xE1); // reg 7
  data.data_send.push_back(0x42); // reg 8
  data.data_send.push_back(0xB5); // reg 9
  data.data_send.push_back(0x01); // reg 10
  data.data_send.push_back(0x1B); // reg 11
  data.data_send.push_back(0xDA); // reg 12
*/

/*
  // Configuration for 50MHz output
  data.data_send.clear();
  data.data_send.push_back(0x63); // reg 7
  data.data_send.push_back(0x42); // reg 8
  data.data_send.push_back(0xAE); // reg 9
  data.data_send.push_back(0x01); // reg 10
  data.data_send.push_back(0x18); // reg 11
  data.data_send.push_back(0xFC); // reg 12
*/

  data.extra[0] = SI571_ADDR;
  data.extra[1] = 6;

  Si570_drv::si570_set_freq(&data);

  sleep(1);

  data.data_send.clear();
  data.data_read.clear();
  //Si570_drv::si570_read_freq(&data); // check if data is the same, //not working

// 125MHz
/*
  Si570_drv::si570_assert(SI571_ADDR, 0x07, 0x21);
  Si570_drv::si570_assert(SI571_ADDR, 0x08, 0xC2);
  Si570_drv::si570_assert(SI571_ADDR, 0x09, 0xBC);
  Si570_drv::si570_assert(SI571_ADDR, 0x0A, 0x01);
  Si570_drv::si570_assert(SI571_ADDR, 0x0B, 0x1E);
  Si570_drv::si570_assert(SI571_ADDR, 0x0C, 0xB8);
*/

// 100MHz

/*
  Si570_drv::si570_assert(SI571_ADDR, 0x07, 0x22);
  Si570_drv::si570_assert(SI571_ADDR, 0x08, 0x42);
  Si570_drv::si570_assert(SI571_ADDR, 0x09, 0xBC);
  Si570_drv::si570_assert(SI571_ADDR, 0x0A, 0x01);
  Si570_drv::si570_assert(SI571_ADDR, 0x0B, 0x1E);
  //Si570_drv::si570_assert(SI571_ADDR, 0x0C, 0xB8);
  Si570_drv::si570_assert(SI571_ADDR, 0x0C, 0x42);
*/

// 117.963900MHz
/*
  Si570_drv::si570_assert(SI571_ADDR, 0x07, 0x61);
  Si570_drv::si570_assert(SI571_ADDR, 0x08, 0x42);
  Si570_drv::si570_assert(SI571_ADDR, 0x09, 0xB5);
  Si570_drv::si570_assert(SI571_ADDR, 0x0A, 0xC4);
  Si570_drv::si570_assert(SI571_ADDR, 0x0B, 0x11);
  Si570_drv::si570_assert(SI571_ADDR, 0x0C, 0xD7);
*/

// 122.682456 MHz
/*
  Si570_drv::si570_assert(SI571_ADDR, 0x07, 0x21);
  Si570_drv::si570_assert(SI571_ADDR, 0x08, 0xC2);
  Si570_drv::si570_assert(SI571_ADDR, 0x09, 0xAF);
  Si570_drv::si570_assert(SI571_ADDR, 0x0A, 0x28);
  Si570_drv::si570_assert(SI571_ADDR, 0x0B, 0x9A);
  Si570_drv::si570_assert(SI571_ADDR, 0x0C, 0x34);
*/

// 112.583175675676 MHz

/*
  Si570_drv::si570_assert(SI571_ADDR, 0x07, 0xE0);
  Si570_drv::si570_assert(SI571_ADDR, 0x08, 0xC2);
  Si570_drv::si570_assert(SI571_ADDR, 0x09, 0xB5);
  Si570_drv::si570_assert(SI571_ADDR, 0x0A, 0xA6);
  Si570_drv::si570_assert(SI571_ADDR, 0x0B, 0x87);
  Si570_drv::si570_assert(SI571_ADDR, 0x0C, 0x75);
*/

/*
// 124.997588 MHz
  Si570_drv::si570_assert(SI571_ADDR, 0x07, 0x21);
  Si570_drv::si570_assert(SI571_ADDR, 0x08, 0xC2);
  Si570_drv::si570_assert(SI571_ADDR, 0x09, 0xBC);
  Si570_drv::si570_assert(SI571_ADDR, 0x0A, 0x20);
  Si570_drv::si570_assert(SI571_ADDR, 0x0B, 0x3C);
  Si570_drv::si570_assert(SI571_ADDR, 0x0C, 0x99);
*/
/*
// 113.529121545 MHz.
  Si570_drv::si570_assert(SI571_ADDR, 0x07, 0xE0);
  Si570_drv::si570_assert(SI571_ADDR, 0x08, 0xC2);
  Si570_drv::si570_assert(SI571_ADDR, 0x09, 0xBB);
  Si570_drv::si570_assert(SI571_ADDR, 0x0A, 0x7A);
  Si570_drv::si570_assert(SI571_ADDR, 0x0B, 0x8B);
  Si570_drv::si570_assert(SI571_ADDR, 0x0C, 0x89);
*/

// 113.750000 MHz. NOT LOCKING!
/*
  Si570_drv::si570_assert(SI571_ADDR, 0x07, 0x41);
  Si570_drv::si570_assert(SI571_ADDR, 0x08, 0xC2);
  Si570_drv::si570_assert(SI571_ADDR, 0x09, 0xFC);
  Si570_drv::si570_assert(SI571_ADDR, 0x0A, 0x8D);
  Si570_drv::si570_assert(SI571_ADDR, 0x0B, 0x61);
  Si570_
  * drv::si570_assert(SI571_ADDR, 0x0C, 0x13);
*/

// 112 MHz.
/*
  Si570_drv::si570_assert(SI571_ADDR, 0x07, 0xE0);
  Si570_drv::si570_assert(SI571_ADDR, 0x08, 0xC2);
  Si570_drv::si570_assert(SI571_ADDR, 0x09, 0xB2);
  Si570_drv::si570_assert(SI571_ADDR, 0x0A, 0x0E);
  Si570_drv::si570_assert(SI571_ADDR, 0x0B, 0xB3);
  Si570_drv::si570_assert(SI571_ADDR, 0x0C, 0x84);
*/

/*
// 114.222973 MHz.
  Si570_drv::si570_assert(SI571_ADDR, 0x07, 0x41);
  Si570_drv::si570_assert(SI571_ADDR, 0x08, 0xC2);
  Si570_drv::si570_assert(SI571_ADDR, 0x09, 0xFF);
  Si570_drv::si570_assert(SI571_ADDR, 0x0A, 0xBB);
  Si570_drv::si570_assert(SI571_ADDR, 0x0B, 0x34);
  Si570_drv::si570_assert(SI571_ADDR, 0x0C, 0xC8);
*/

//113.376415 MHz
/*
  Si570_drv::si570_assert(SI571_ADDR, 0x07, 0xE0);
  Si570_drv::si570_assert(SI571_ADDR, 0x08, 0xC2);
  Si570_drv::si570_assert(SI571_ADDR, 0x09, 0xBA);
  Si570_drv::si570_assert(SI571_ADDR, 0x0A, 0x89);
  Si570_drv::si570_assert(SI571_ADDR, 0x0B, 0xAF);
  Si570_drv::si570_assert(SI571_ADDR, 0x0C, 0x5B);
*/

//208.927174 MHz
  Si570_drv::si570_assert(SI571_ADDR, 0x07, 0x40);
  Si570_drv::si570_assert(SI571_ADDR, 0x08, 0xC2);
  Si570_drv::si570_assert(SI571_ADDR, 0x09, 0xBE);
  Si570_drv::si570_assert(SI571_ADDR, 0x0A, 0xC6);
  Si570_drv::si570_assert(SI571_ADDR, 0x0B, 0xA5);
  Si570_drv::si570_assert(SI571_ADDR, 0x0C, 0x58);

  Si570_drv::si570_outputEnable(FPGA_CTRL_REGS | WB_CLK_CTRL);

  //exit(1);

  // ======================================================
  //      AD9510 configuration (clock distribution)
  // ======================================================
  // turn off reset! (defualt setting for function pin)
  cout << "============================================" << endl <<
      "     AD9510 config (clock distribution)     " << endl <<
      "============================================" << endl;

  int pll_status;

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

  //AD9510_drv::AD9510_config_si570(AD9510_ADDR); // with config check included
  AD9510_drv::AD9510_config_si570_pll_fmc_adc_130m_4ch(AD9510_ADDR); // with config check included

  // Check PLL lock

  data.wb_addr = FPGA_CTRL_REGS | WB_CLK_CTRL; // clock control
  _commLink->fmc_config_read(&data);

  pll_status = data.data_read[0] & AD9510_PLL_STATUS_MASK;

  cout << "WB Clock Control Reg: " << data.data_read[0] << endl;
  cout << "AD9510 PLL Status: " << pll_status << endl;

  cout << "If the AD9510 PLL Status pin is configured for Digital Lock the " << endl <<
             "information below makes sense. Otherwise ignore that!"  << endl;

  if (pll_status)
    cout << "Clock PLL Locked!" << endl;
  else
    cout << "Clock PLL NOT locked!" << endl;

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
//  test_pattern[0] = 0x0000; // 9999
//  test_pattern[1] = 0x0000;
//  test_pattern[2] = 0x0000;
//  test_pattern[3] = 0x0000;

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

  set_fpga_delay_s(_commLink, FPGA_CTRL_REGS | WB_IDELAY0_CAL, &delay_data_l[0], DLY_DATA);

  //data.wb_addr = FPGA_CTRL_REGS | WB_IDELAY0_CAL;
  //data.data_send[0] = IDELAY_ALL_LINES | IDELAY_TAP(18) | IDELAY_UPDATE; // should be 0x0050003f
  //_commLink->fmc_config_send(&data);
  //// check data
  //_commLink->fmc_config_read(&data);
  ////assert(data.data_read[0] == (IDELAY_ALL_LINES | IDELAY_TAP(18) | IDELAY_UPDATE));
  //usleep(1000);
  //data.data_send[0] = (IDELAY_ALL_LINES | IDELAY_TAP(18)) & 0xFFFFFFFE; // should be 0x0050003f
  //_commLink->fmc_config_send(&data);

  set_fpga_delay_s(_commLink, FPGA_CTRL_REGS | WB_IDELAY1_CAL, &delay_data_l[1], DLY_DATA);

  //data.wb_addr = FPGA_CTRL_REGS | WB_IDELAY1_CAL;
  //data.data_send[0] = IDELAY_ALL_LINES | IDELAY_TAP(18) | IDELAY_UPDATE; // should be 0x0050003f
  //_commLink->fmc_config_send(&data);
  //// check data
  //_commLink->fmc_config_read(&data);
  ////assert(data.data_read[0] == (IDELAY_ALL_LINES | IDELAY_TAP(9) | IDELAY_UPDATE));
  //usleep(1000);
  //data.data_send[0] = (IDELAY_ALL_LINES | IDELAY_TAP(18)) & 0xFFFFFFFE; // should be 0x0050003f
  //_commLink->fmc_config_send(&data);

  set_fpga_delay_s(_commLink, FPGA_CTRL_REGS | WB_IDELAY2_CAL, &delay_data_l[2], DLY_DATA);

  //data.wb_addr = FPGA_CTRL_REGS | WB_IDELAY2_CAL;
  //data.data_send[0] = IDELAY_ALL_LINES | IDELAY_TAP(18) | IDELAY_UPDATE; // should be 0x0050003f
  //_commLink->fmc_config_send(&data);
  //// check data
  //_commLink->fmc_config_read(&data);
  ////assert(data.data_read[0] == (IDELAY_ALL_LINES | IDELAY_TAP(13) | IDELAY_UPDATE));
  //usleep(1000);
  //data.data_send[0] = (IDELAY_ALL_LINES | IDELAY_TAP(18)) & 0xFFFFFFFE; // should be 0x0050003f
  //_commLink->fmc_config_send(&data);

  set_fpga_delay_s(_commLink, FPGA_CTRL_REGS | WB_IDELAY3_CAL, &delay_data_l[3], DLY_DATA);

  //data.wb_addr = FPGA_CTRL_REGS | WB_IDELAY3_CAL;
  //data.data_send[0] = IDELAY_ALL_LINES | IDELAY_TAP(18) | IDELAY_UPDATE; // should be 0x0050003f
  //_commLink->fmc_config_send(&data);
  //// check data
  //_commLink->fmc_config_read(&data);
  ////assert(data.data_read[0] == (IDELAY_ALL_LINES | IDELAY_TAP(13) | IDELAY_UPDATE));
  //usleep(1000);
  //data.data_send[0] = (IDELAY_ALL_LINES | IDELAY_TAP(18)) & 0xFFFFFFFE; // should be 0x0050003f
  //_commLink->fmc_config_send(&data);

  // walking "1" test
//  for (int i = 0; i < 16; i++) {
//
//    test_pattern[0] = (0x1) << i;
//    test_pattern[1] = test_pattern[0];
//    test_pattern[2] = test_pattern[0];
//    test_pattern[3] = test_pattern[0];
//
//    ISLA216P_drv::ISLA216P_setTestPattern(ISLA_ADC0_ADDR, 0x83, test_pattern);
//    ISLA216P_drv::ISLA216P_setTestPattern(ISLA_ADC1_ADDR, 0x83, test_pattern);
//    ISLA216P_drv::ISLA216P_setTestPattern(ISLA_ADC2_ADDR, 0x83, test_pattern);
//    ISLA216P_drv::ISLA216P_setTestPattern(ISLA_ADC3_ADDR, 0x83, test_pattern);
//
//    printf("Test pattern (walking 1): %08x\n", test_pattern[0]);
//
//    sleep(30);
//  }

  // walking "0" test
/*  for (int i = 0; i < 16; i++) {

    test_pattern[0] = ~((0x1) << i);
    test_pattern[1] = test_pattern[0];
    test_pattern[2] = test_pattern[0];
    test_pattern[3] = test_pattern[0];

    ISLA216P_drv::ISLA216P_setTestPattern(ISLA_ADC0_ADDR, 0x83, test_pattern);
    ISLA216P_drv::ISLA216P_setTestPattern(ISLA_ADC1_ADDR, 0x83, test_pattern);
    ISLA216P_drv::ISLA216P_setTestPattern(ISLA_ADC2_ADDR, 0x83, test_pattern);
    ISLA216P_drv::ISLA216P_setTestPattern(ISLA_ADC3_ADDR, 0x83, test_pattern);

    printf("Test pattern (walking 0): %08x\n", test_pattern[0]);

    sleep(35);
  }*/

  // turn off test pattern
  //cout << "Check test pattern" << endl;
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
