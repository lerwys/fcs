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
#include "plat_opts.h" // must be included before reg_map*
#include "data.h"
#include "reg_map/fmc_config_130m_4ch.h"

#include <iostream>
#include <unistd.h>  /* getopt */

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

#define PLL_STATUS_MASK 0x04

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
  // startup freq:  155.4882208 MHz (previously 155.49MHz previously 155.52 MHz)
  // I2C addr: 0x49
  // Startup registers values:
  // reg 7 = 0x01      // 0000 0001 -> HS = 000 -> HS = 4d
  // reg 8 = 0xC2      // 1100 0010 -> N1 = 000 0111 -> N1 = 8d
  // reg 9 = 0xB8      // 1011 1000
  // reg 10 = 0xBB     // 1011 1011
  // reg 11 = 0xE4     // 1110 0100
  // reg 12 = 0x72     // 0111 0010
  // are those default values correct?

  // fDCO_current = 4.97568 GHz (previously 4.97664 GHz)
  // RFreq = 00 0010 1011 1000 1011 1011 1110 0100 0111 0010 = 11689256050d
  // RFreq = 11689256050d / 2^28 = 43.545872159
  // fxtal = 114.26164683147 (previously 114.262954 MHz previously 114.285 MHz )
  
  
  
  //fxtal = 92.698155690157???

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

  // Configuration for 113.511169 MHz output
/*
  //HS = 111 -> HS = 11d
  //N1 = 000 0011 -> N1 = 4d
  // RFreq = 43.7105051213712 -> RFreq = 43.7105051213712*2^28 = 11733449374.2456 = 2BB5E3A9Eh
  data.data_send.clear();
  data.data_send.push_back(0xE0); // 1110 0000
  data.data_send.push_back(0xC2); // 1100 0010
  data.data_send.push_back(0xBB);
  data.data_send.push_back(0x5E);
  data.data_send.push_back(0x3A);
  data.data_send.push_back(0x9E);
*/

  // Configuration for 113.376415 MHz output

  //HS = 111 -> HS = 11d
  //N1 = 000 0011 -> N1 = 4d
  // RFreq = 43.6586144972237 -> RFreq = 43.6586144972237*2^28 = 11719520090.8904 = 2BA89AF5Bh
  data.data_send.clear();
  data.data_send.push_back(0xE0); // 1110 0000
  data.data_send.push_back(0xC2); // 1100 0010
  data.data_send.push_back(0xBA);
  data.data_send.push_back(0x89);
  data.data_send.push_back(0xAF);
  data.data_send.push_back(0x5B);

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
 // 113.376415 MHz ++ 42 KHz output

  // Configuration for 113.376415 MHz output
  //RFreq = 2BACDFA1F
/*
  //HS = 111 -> HS = 11d
  //N1 = 000 0011 -> N1 = 4d
  // RFreq = 43.6591139576158 -> RFreq = 43.6591139576158*2^28 = 11719654164 = 2BA8BBB14h
  data.data_send.clear();
  data.data_send.push_back(0xE0); // 1110 0000
  data.data_send.push_back(0xC2); // 1100 0010
  data.data_send.push_back(0xBA);
  data.data_send.push_back(0xCD);
  data.data_send.push_back(0xFA);
  data.data_send.push_back(0x1F);
*/

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

// 113.511169 MHz output
/*
  Si570_drv::si570_assert(SI571_ADDR, 0x07, 0xE0);
  Si570_drv::si570_assert(SI571_ADDR, 0x08, 0xC2);
  Si570_drv::si570_assert(SI571_ADDR, 0x09, 0xBB);
  Si570_drv::si570_assert(SI571_ADDR, 0x0A, 0x5E);
  Si570_drv::si570_assert(SI571_ADDR, 0x0B, 0x3A);
  Si570_drv::si570_assert(SI571_ADDR, 0x0C, 0x9E);
*/

// 113.376415 MHz output

  Si570_drv::si570_assert(SI571_ADDR, 0x07, 0xE0);
  Si570_drv::si570_assert(SI571_ADDR, 0x08, 0xC2);
  Si570_drv::si570_assert(SI571_ADDR, 0x09, 0xBA);
  Si570_drv::si570_assert(SI571_ADDR, 0x0A, 0x89);
  Si570_drv::si570_assert(SI571_ADDR, 0x0B, 0xAF);
  Si570_drv::si570_assert(SI571_ADDR, 0x0C, 0x5B);


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

/*
//113.376415 +  42 KHz MHz
  Si570_drv::si570_assert(SI571_ADDR, 0x07, 0xE0);
  Si570_drv::si570_assert(SI571_ADDR, 0x08, 0xC2);
  Si570_drv::si570_assert(SI571_ADDR, 0x09, 0xBA);
  Si570_drv::si570_assert(SI571_ADDR, 0x0A, 0xCD);
  Si570_drv::si570_assert(SI571_ADDR, 0x0B, 0xFA);
  Si570_drv::si570_assert(SI571_ADDR, 0x0C, 0x1F);
*/

  Si570_drv::si570_outputEnable(FPGA_CTRL_REGS | WB_CLK_CTRL);

  //exit(1);

  // ======================================================
  //      AD9510 configuration (clock distribution)
  // ======================================================
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

  // FPGA working with clock copy for ADC (FMC ADC 130M 4CH rev.1)
  //AD9510_drv::AD9510_config_si570_fmc_adc_130m_4ch(AD9510_ADDR); // with config check included
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
