//============================================================================
// Author      : Lucas Russo
// Version     : 1.0
// Description : Software driver for PCIe-Wishbone Master IP core
//============================================================================
#include "fmc_config_130m_4ch_board.h"
#include <math.h>
#include "debug.h"

#define PLL_STATUS_MASK 0x04
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define ACTIVE_BOARD

fmc_config_130m_4ch_board::fmc_config_130m_4ch_board(WBMaster_unit* wb_master_unit, const delay_lines *delay_data_l,
                                        const delay_lines *delay_clk_l) {

    init(wb_master_unit, delay_data_l, delay_clk_l);
    config_defaults();
    // FIXME: workaround to avoid reseting the FPGA to adjust delays!!!!
    //_commLink_serial = new commLink();
    // FIXME: workaround to avoid reseting the FPGA to adjust delays!!!!
    //_commLink_serial->regWBMaster(new rs232_syscon_driver());
    usleep(100000);
    // FIXME: workaround to avoid reseting the FPGA to adjust delays!!!!
    config_defaults();

    this->acq_nsamples_n = 4096;
    this->acq_chan_n = 0;        //ADC
    this->acq_offset_n = 0x0;

    for (unsigned int i = 0; i < ARRAY_SIZE(acq_last_params); ++i) {
        this->acq_last_params[i].acq_nsamples = 1024; /* FIXME! Test! */
    }

    cout << "fmc_config_130m_4ch_board initilized!" << endl;

}

int fmc_config_130m_4ch_board::init(WBMaster_unit* wb_master_unit, const delay_lines *delay_data_l,
                                        const delay_lines *delay_clk_l) {

  //data.data_send.resize(10);
  //data.extra.resize(2);
  _commLink = new commLink();

  // CommLink configuration
  // Adding communication interfaces
  _commLink->regWBMaster(new pcie_link_driver(0));

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

  // Get delay lines
  this->delay_data_l = delay_data_l;
  this->delay_clk_l = delay_clk_l;

  this->adc_clk = DEFAULT_ADC_CLK;

  return 0;
}

int fmc_config_130m_4ch_board::config_defaults() {

  wb_data data;

  data.data_send.resize(10);
  data.extra.resize(2);

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
  //data.data_send[0] = 0x00;
  //_commLink->fmc_config_send(&data);

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


/*************************************************************************/
/*********************** START OF ACTIVE BOARD ONYLY! **********************/
/*************************************************************************/
#ifdef ACTIVE_BOARD
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
  // startup freq:  155.49MHz (previously 155.52 MHz)
  // I2C addr: 0x49
  // Startup registers values (INCORRECT!!!):
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
  // fxtal = 114.262954 MHz ( previously 114.285 MHz )

  // Startup registers values (CORRECT ONES!):
  // reg 7 = 0x01      // 0000 0001 -> HS = 000 -> HS = 4d
  // reg 8 = 0xC2      // 1100 0010 -> N1 = 000 0111 -> N1 = 8d
  // reg 9 = 0xB8      // 1011 1000
  // reg 10 = 0xFD     // 1111 1101
  // reg 11 = 0x18     // 0001 1000
  // reg 12 = 0x58     // 0101 1000

  // fDCO_current = 4.97568 GHz (previously 4.97664 GHz)
  // RFreq = 00 0010 1011 1000 1111 1101 0001 1000 0101 1000 = 11693529176d
  // RFreq = 11693529176d / 2^28 = 43.5617907941341
  // fxtal = 114.243237233262 MHz (startup freq = 155.52 MHz)
  // fxtal = 114.221199571759 MHz (startup freq = 155.49 MHz)

  // ======================================================
  //          Si571 configuration (clock generation) V2 (previously crystek)
  // ======================================================
  // Set Si571 to f_out = 250MHz
  // data generated with Si570 software (from Si www)
  // part number:
  // 571
  // AJC000337G, rev. D
  // parameters:
  // OE - active high, 3.3V, LVPECL
  // 10-280MHz freq. range
  // startup freq:  155.49MHz (155.488229 MHz) WRONG!
  // startup freq:  155.48900107 MHz
  // I2C addr: 0x49
  // Startup registers values:
  // reg 7 = 0x01      // 0000 0001 -> HS = 000 -> HS = 4d
  // reg 8 = 0xC2      // 1100 0010 -> N1 = 000 0111 -> N1 = 8d
  // reg 9 = 0xB9      // 1011 1001
  // reg 10 = 0x35     // 0011 0101
  // reg 11 = 0x5E     // 0101 1110
  // reg 12 = 0x88     // 1000 1000

  // fDCO_current = 4975.621216 GHz
  // RFreq = 2B72EB759/2^28 (previously 43.154492552)
  // RFreq = 43.5755296051502
  // fxtal = (previously 115.297873 MHz + 0.343069999999983 MHz) WRONG!
  // fxtal = (previously 114.185187078298 MHz) WRONG!
  // fxtal = 114.195246117106 MHz
  // fxtal = 114.18445350695 MHz
  // fxtal = 114.20445350695 MHz
  // fxtal = 114.207217791607 MHz
  //

  //---------------FMC_130MSa_v2_Active_A1

  // fxtal = (fxout * Ns * H1)/(hex2dec(RFreq/2^28))
  //
  // RFreq  = 2B95DEF84/2^28
  // RFreq  = 43.5854334980249
  // reg 7  = 0x01
  // reg 8  = 0xc2
  // reg 9  = 0xb9
  // reg 10 = 0x5d
  // reg 11 = 0xef
  // reg 12 = 0x84
  // fxtal = 114.159240844203 MHz

  //---------------FMC_130MSa_v3_Active_A1
  // fxtal = (fxout * Ns * H1)/(hex2dec(RFreq/2^28))
  //
  //RFreq  = hex2dec('2B8D479ED')/2^28
  //RFreq  = 43.5518740899861
  //fxout = 155.49
  //Hs = 4
  //N1 = 8
  //fxtal = (fxout * Hs * N1)/(Rfreq)
  //fxtal = 114.247207587883 MHz

  cout << "============================================" << endl <<
      "   Si571 configuration (clock generation)   " << endl <<
      "============================================" << endl;

  data.data_send.clear();
  data.data_read.clear();
  data.extra.clear();
  data.data_send.resize(1);
  data.data_read.resize(10);
  data.extra.resize(2);
  data.extra[0] = SI571_ADDR;
  data.extra[1] = 6;
  data.data_send[0] = 0x07; // starting register

  Si570_drv::si570_setCommLink(_commLink, SI571_I2C_DRV, GENERAL_GPIO_DRV);
  Si570_drv::si570_outputDisable(FPGA_CTRL_REGS | WB_CLK_CTRL);

  uint64_t rfreq;
  unsigned int n1;
  unsigned int hs_div;

  // Fill with factory defaults
  Si570_drv::si570_get_defaults(SI571_ADDR);
  //Si570_drv::si570_read_freq(&data, &rfreq, &n1, &hs_div);
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
/*
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
*/
  // Configuration for 113.040445 MHz output
/*
  //HS = 111 -> HS = 11d
  //N1 = 000 0011 -> N1 = 4d
  // RFreq = 43.5367528131624 -> RFreq = 43.5367528131624*2^28 = 11686808094.1605 = 2B8968A1Fh
  // RFreq = 43.5292402820253 -> RFreq = 43.5292402820253*2^28 = 11684791464.439 = 2B877C4A9h
  data.data_send.clear();
  data.data_send.push_back(0xE0); // 1110 0000
  data.data_send.push_back(0xC2); // 1100 0010
  data.data_send.push_back(0xB8);
  data.data_send.push_back(0x77);
  data.data_send.push_back(0xC4);
  data.data_send.push_back(0xA9);
*/
  // Configuration for 113.040445 MHz output (FMC V2 previously crystek)

  //HS = 111 -> HS = 11d
  //N1 = 000 0011 -> N1 = 4d
  // RFreq = 43.5591661319924 -> RFreq = 43.5591661319924*2^28 = 11692824623.6211 = 2B8F25830h (semi-correct)
  // RFreq = 43.551537853971  -> RFreq = 43.551537853971*2^28  = 11690776923.332  = 2B8D3195Bh
  // RFreq = 43.5504837275313 -> RFreq = 43.5504837275313*2^28 = 11690493958.4204 = 2B8CEC806h
  //
  // REGS = dec2hex(round((fout*HS*N1/fxtal)*2^28))
  // RFreq = 43.5687864006374 -> RFreq = 43.5687864006374*2^28 = 11695407044.8217 = 2B919BFC4h
  //
  //
   //fxtal = 114.247207587883
   //fout = 113.040445
   //Hs = 11
   //N1 = 4
   //Rfreq = dec2hex(round((fout*Hs*N1/fxtal)*2^28))
   //RFreq = 2B890579F

  ////////////////////data.data_send.clear();
  ////////////////////data.data_send.push_back(0xE0); // 1110 0000
  ////////////////////data.data_send.push_back(0xC2); // 1100 0010
  ////////////////////data.data_send.push_back(0xB8);
  ////////////////////data.data_send.push_back(0x90);
  ////////////////////data.data_send.push_back(0x57);
  ////////////////////data.data_send.push_back(0x9F);

  Si570_drv::si570_set_freq_hl(113040445, SI571_ADDR);

  // Configuration for 64.247 MHz output
/*
  //HS = 000 -> HS = 4d
  //N1 = 001 0011 -> N1 = 20d
  // RFreq = 44.9896214819756 -> RFreq = 44.9896214819756*2^28 = 12076809557.7815 = 2CFD57D56h
  data.data_send.clear();
  data.data_send.push_back(0x04); // 0000 0100
  data.data_send.push_back(0xC2); // 1100 0010
  data.data_send.push_back(0xCF);
  data.data_send.push_back(0xD5);
  data.data_send.push_back(0x7D);
  data.data_send.push_back(0x56);
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
/*
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
*/

// Configuration for 113.515008 MHz output
/*
  //HS = 111 -> HS = 11d
  //N1 = 000 0011 -> N1 = 4d
  // RFreq = 43.7119834307802 -> RFreq = 43.7119834307802*2^28 = 11733846204.9059 = 2BB6448BDh
  data.data_send.clear();
  data.data_send.push_back(0xE0); // 1110 0000
  data.data_send.push_back(0xC2); // 1100 0010
  data.data_send.push_back(0xBB);
  data.data_send.push_back(0x64);
  data.data_send.push_back(0x48);
  data.data_send.push_back(0xBD);
*/

// Configuration for 113.515008 MHz output (V2, previously crystek)
/*
  //HS = 111 -> HS = 11d
  //N1 = 000 0011 -> N1 = 4d
  // RFreq = 43.7420349145516 -> RFreq = 43.7420349145516*2^28 = 11741913088.6556 = 2BBDF6001h
  data.data_send.clear();
  data.data_send.push_back(0xE0); // 1110 0000
  data.data_send.push_back(0xC2); // 1100 0010
  data.data_send.push_back(0xBB);
  data.data_send.push_back(0xDF);
  data.data_send.push_back(0x60);
  data.data_send.push_back(0x01);
*/

  ///////////////////////data.extra[0] = SI571_ADDR;
  ///////////////////////data.extra[1] = 6;
  ///////////////////////
  ///////////////////////Si570_drv::si570_set_freq(&data);
  ///////////////////////
  ///////////////////////sleep(1);
  ///////////////////////
  ///////////////////////data.data_send.clear();
  ///////////////////////data.data_read.clear();
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
/*
  Si570_drv::si570_assert(SI571_ADDR, 0x07, 0xE0);
  Si570_drv::si570_assert(SI571_ADDR, 0x08, 0xC2);
  Si570_drv::si570_assert(SI571_ADDR, 0x09, 0xBA);
  Si570_drv::si570_assert(SI571_ADDR, 0x0A, 0x89);
  Si570_drv::si570_assert(SI571_ADDR, 0x0B, 0xAF);
  Si570_drv::si570_assert(SI571_ADDR, 0x0C, 0x5B);
*/

// 113.040445 MHz output
/*
  Si570_drv::si570_assert(SI571_ADDR, 0x07, 0xE0);
  Si570_drv::si570_assert(SI571_ADDR, 0x08, 0xC2);
  Si570_drv::si570_assert(SI571_ADDR, 0x09, 0xB8);
  Si570_drv::si570_assert(SI571_ADDR, 0x0A, 0x77);
  Si570_drv::si570_assert(SI571_ADDR, 0x0B, 0xC4);
  Si570_drv::si570_assert(SI571_ADDR, 0x0C, 0xA9);
*/

// 113.040445 MHz output (v2 previously crystek)

  /////////////////////////////////Si570_drv::si570_assert(SI571_ADDR, 0x07, 0xE0);
  /////////////////////////////////Si570_drv::si570_assert(SI571_ADDR, 0x08, 0xC2);
  /////////////////////////////////Si570_drv::si570_assert(SI571_ADDR, 0x09, 0xB8);
  /////////////////////////////////Si570_drv::si570_assert(SI571_ADDR, 0x0A, 0x90);
  /////////////////////////////////Si570_drv::si570_assert(SI571_ADDR, 0x0B, 0x57);
  /////////////////////////////////Si570_drv::si570_assert(SI571_ADDR, 0x0C, 0x9F);


// 64.247 MHz output
/*
  Si570_drv::si570_assert(SI571_ADDR, 0x07, 0x04);
  Si570_drv::si570_assert(SI571_ADDR, 0x08, 0xC2);
  Si570_drv::si570_assert(SI571_ADDR, 0x09, 0xCF);
  Si570_drv::si570_assert(SI571_ADDR, 0x0A, 0xD5);
  Si570_drv::si570_assert(SI571_ADDR, 0x0B, 0x7D);
  Si570_drv::si570_assert(SI571_ADDR, 0x0C, 0x56);
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
/*
// 113.376415 MHz output
  Si570_drv::si570_assert(SI571_ADDR, 0x07, 0xE0);
  Si570_drv::si570_assert(SI571_ADDR, 0x08, 0xC2);
  Si570_drv::si570_assert(SI571_ADDR, 0x09, 0xBA);
  Si570_drv::si570_assert(SI571_ADDR, 0x0A, 0x89);
  Si570_drv::si570_assert(SI571_ADDR, 0x0B, 0xAF);
  Si570_drv::si570_assert(SI571_ADDR, 0x0C, 0x5B);
*/

// 113.515008 MHz output
/*
  Si570_drv::si570_assert(SI571_ADDR, 0x07, 0xE0);
  Si570_drv::si570_assert(SI571_ADDR, 0x08, 0xC2);
  Si570_drv::si570_assert(SI571_ADDR, 0x09, 0xBB);
  Si570_drv::si570_assert(SI571_ADDR, 0x0A, 0x64);
  Si570_drv::si570_assert(SI571_ADDR, 0x0B, 0x48);
  Si570_drv::si570_assert(SI571_ADDR, 0x0C, 0xBD);
*/

// 113.515008 MHz output (V2, previously crystek)
/*
  Si570_drv::si570_assert(SI571_ADDR, 0x07, 0xE0);
  Si570_drv::si570_assert(SI571_ADDR, 0x08, 0xC2);
  Si570_drv::si570_assert(SI571_ADDR, 0x09, 0xBB);
  Si570_drv::si570_assert(SI571_ADDR, 0x0A, 0xDF);
  Si570_drv::si570_assert(SI571_ADDR, 0x0B, 0x60);
  Si570_drv::si570_assert(SI571_ADDR, 0x0C, 0x01);
*/
  Si570_drv::si570_outputEnable(FPGA_CTRL_REGS | WB_CLK_CTRL);

  //exit(1);

  // ======================================================
  //      AD9510 configuration (clock distribution)
  // ======================================================
  cout << "============================================" << endl <<
      "     AD9510 config (clock distribution)     " << endl <<
      "============================================" << endl;

  // select ext clk ref pin from TUSB switch
  data.wb_addr = FPGA_CTRL_REGS | WB_CLK_CTRL; // clock control
  _commLink->fmc_config_read(&data);
  cout << "CLK_CTRL reg read: " << data.data_read[0] << endl;
  data.data_send[0] = data.data_read[0] & ~(0x08);
  ////////////////////////////////////////////data.data_send[0] = data.data_read[0] | (0x08);
  cout << "CLK_CTRL reg will write: " << data.data_send[0] << endl;
  _commLink->fmc_config_send(&data);
  sleep(1);

  int pll_status;
  uint32_t data_temp;

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
  sleep (2);

  data.wb_addr = FPGA_CTRL_REGS | WB_CLK_CTRL; // clock control
  _commLink->fmc_config_read(&data);

  pll_status = data.data_read[0] & PLL_STATUS_MASK;

  cout << "WB Clock Control Reg: " << data.data_read[0] << endl;
  cout << "AD9510 PLL Status: " << pll_status << endl;

  cout << "If the AD9510 PLL Status pin is configured for Digital Lock the " << endl <<
             "information below makes sense. Otherwise ignore that!"  << endl;

  if (pll_status)
    cout << "Clock PLL Locked!" << endl;
  else
    cout << "Clock PLL NOT locked!" << endl;

/*************************************************************************/
/*********************** END OF ACTIVE BOARD ONYLY! **********************/
/*************************************************************************/
#endif

  // ======================================================
  //                  LTC ADC configuration
  // ======================================================
  cout << "============================================" << endl <<
      "     LTC2208 config (4 ADC chips)     " << endl <<
      "============================================" << endl;

  data.wb_addr = FPGA_CTRL_REGS | WB_ADC_LTC_CTRL; // trigger control
  //data.data_send[0] = 0x08; // dither off, power on, random off, pga on (input 2.25 Vpp)
  //data.data_send[0] = 0x0A; // dither on, power on, random off, pga on (input 1.50 Vpp)
  //data.data_send[0] = 0x02; // dither on, power on, random off, pga off (input 2.25 Vpp)
  data.data_send[0] = 0x00; // dither off, power on, random off, pga off (input 2.25 Vpp)
  _commLink->fmc_config_send(&data);

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

  set_fpga_delay_s(_commLink, FPGA_CTRL_REGS | WB_IDELAY0_CAL, &delay_data_l[0], DLY_DATA);
  set_fpga_delay_s(_commLink, FPGA_CTRL_REGS | WB_IDELAY1_CAL, &delay_data_l[1], DLY_DATA);
  set_fpga_delay_s(_commLink, FPGA_CTRL_REGS | WB_IDELAY2_CAL, &delay_data_l[2], DLY_DATA);
  set_fpga_delay_s(_commLink, FPGA_CTRL_REGS | WB_IDELAY3_CAL, &delay_data_l[3], DLY_DATA);

  set_fpga_delay_s(_commLink, FPGA_CTRL_REGS | WB_IDELAY0_CAL, &delay_clk_l[0], DLY_CLK);
  set_fpga_delay_s(_commLink, FPGA_CTRL_REGS | WB_IDELAY1_CAL, &delay_clk_l[1], DLY_CLK);
  set_fpga_delay_s(_commLink, FPGA_CTRL_REGS | WB_IDELAY2_CAL, &delay_clk_l[2], DLY_CLK);
  set_fpga_delay_s(_commLink, FPGA_CTRL_REGS | WB_IDELAY3_CAL, &delay_clk_l[3], DLY_CLK);

  // train communication links

  // ======================================================
  //                BPM Swap configuration
  // ======================================================
  cout << "============================================" << endl <<
          "           BPM Swap configuration           " << endl <<
          "============================================" << endl;

  // BPM Swap parameters
  data.wb_addr = DSP_BPM_SWAP | BPM_SWAP_REG_A;
  data.data_send[0] = 32768 | 32768 << 16; // no gain for AA and AC
  _commLink->fmc_config_send(&data);

  data.wb_addr = DSP_BPM_SWAP | BPM_SWAP_REG_B;
  data.data_send[0] = 32768 | 32768 << 16; // no gain for BB and BD
  _commLink->fmc_config_send(&data);

  data.wb_addr = DSP_BPM_SWAP | BPM_SWAP_REG_C;
  data.data_send[0] = 32768 | 32768 << 16; // no gain for CC and CA
  _commLink->fmc_config_send(&data);

  data.wb_addr = DSP_BPM_SWAP | BPM_SWAP_REG_D;
  data.data_send[0] = 32768 | 32768 << 16; // no gain for DD and DB
  _commLink->fmc_config_send(&data);

  printf("BPM Swap static gain set to: %d\n", 32768);

  // Switching DIVCLK
  data.wb_addr = DSP_BPM_SWAP | BPM_SWAP_REG_CTRL;
  data.data_send[0] = 553 << 8;
  _commLink->fmc_config_send(&data);

  printf("BPM Swap DIVCLK set to: %d\n", 553);

  // Switching mode
  data.wb_addr = DSP_BPM_SWAP | BPM_SWAP_REG_CTRL;
  data.data_send[0] |= data.data_send[0] | 0x1 << 1 | 0x1 << 3; // Direct mode for both sets of channels
  _commLink->fmc_config_send(&data);

  printf("BPM Swap Switching set to OFF!\n");

  // Disable switching clock
  data.wb_addr = DSP_BPM_SWAP | BPM_SWAP_REG_CTRL;
  data.data_send[0] &= ~BPM_SWAP_CTRL_CLK_SWAP_EN;
  _commLink->fmc_config_send(&data);

  data.wb_addr = DSP_BPM_SWAP | BPM_SWAP_REG_DLY;
  data.data_send[0] = BPM_SWAP_DLY_1_W(210) |
                        BPM_SWAP_DLY_2_W(210);
  _commLink->fmc_config_send(&data);

  printf("BPM Swap Delay 210\n");

  // Set windowing mode
  data.data_send[0] = 0x0;
  data.wb_addr = DSP_BPM_SWAP | BPM_SWAP_REG_WDW_CTL;
  data.data_send[0] &= ~BPM_SWAP_WDW_CTL_USE; // No windowing
  _commLink->fmc_config_send(&data);

  printf("BPM Swap Windowing set to OFF!\n");

  data.wb_addr = DSP_BPM_SWAP | BPM_SWAP_REG_WDW_CTL;
  data.data_send[0] &= ~BPM_SWAP_WDW_CTL_SWCLK_EXT; // Internal clock windowing
  _commLink->fmc_config_send(&data);

  printf("BPM Swap switching clock set to internal!\n");

  // Set windowing delay
  data.wb_addr = DSP_BPM_SWAP | BPM_SWAP_REG_WDW_CTL;
  data.data_send[0] = (data.data_read[0] & ~BPM_SWAP_WDW_CTL_DLY_MASK) |
                        BPM_SWAP_WDW_CTL_DLY_W(210);
  _commLink->fmc_config_send(&data);

  printf("BPM Swap Windowing delay set to 210!\n");

  // ======================================================
  //                DSP configuration
  // ======================================================
  cout << "============================================" << endl <<
          "            DSP configuration         " << endl <<
          "============================================" << endl;

  //// DSP parameters
  data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DS_TBT_THRES;
  data.data_send[0] = 0x0200;  // 1.2207e-04 FIX26_22
  _commLink->fmc_config_send(&data);

  data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DS_FOFB_THRES;
  data.data_send[0] = 0x0200;  // 1.2207e-04 FIX26_22
  _commLink->fmc_config_send(&data);

  data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DS_MONIT_THRES;
  data.data_send[0] = 0x0200;  // 1.2207e-04 FIX26_22
  _commLink->fmc_config_send(&data);

  printf("BPM DSP Threshold Calculation set to %f\n",
          1.2207e-4);

  data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_KX;
  data.data_send[0] = 8388608;  // 10000000 UFIX25_0
  _commLink->fmc_config_send(&data);

  data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_KY;
  data.data_send[0] = 8388608;  // 10000000 UFIX25_0
  _commLink->fmc_config_send(&data);

  printf("BPM DSP KX, KY set to %d\n", 10000000);

  data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_KSUM;
  data.data_send[0] = 0x0FFFFFF;  // 1.0 FIX25_24
  _commLink->fmc_config_send(&data);

  printf("BPM DSP KSum set to %f\n", 1.0);

  // DDS config.

  data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DDS_PINC_CH0;
  data.data_send[0] = 245366784;  // phase increment
  _commLink->fmc_config_send(&data);

  data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DDS_PINC_CH1;
  data.data_send[0] = 245366784;  // phase increment
  _commLink->fmc_config_send(&data);

  data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DDS_PINC_CH2;
  data.data_send[0] = 245366784;  // phase increment
  _commLink->fmc_config_send(&data);

  data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DDS_PINC_CH3;
  data.data_send[0] = 245366784;  // phase increment
  _commLink->fmc_config_send(&data);

  printf("BPM DSP DDS Phase increment set to %d\n", data.data_send[0]);

  // FIXME FIXME FIXME. Wrong FPGA firmware!!!
  unsigned int i;
  for (i = 0; i < 10; ++i) {
    data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DDS_CFG;
    // toggle valid signal for all four DDS's
    data.data_send[0] = POS_CALC_DDS_CFG_VALID_CH0 |
                POS_CALC_DDS_CFG_VALID_CH1 |
                POS_CALC_DDS_CFG_VALID_CH2 |
                POS_CALC_DDS_CFG_VALID_CH3;
    _commLink->fmc_config_send(&data);
  }

  // ======================================================
  //                Acq configuration
  // ======================================================
  cout << "============================================" << endl <<
          "            Acq configuration         " << endl <<
          "============================================" << endl;

  printf("Acq Config. is done!\n");

  return 0;
}

fmc_config_130m_4ch_board::~fmc_config_130m_4ch_board() {

    // Close device
}

int fmc_config_130m_4ch_board::blink_leds() {

  wb_data data;

  data.data_send.resize(10);
  data.extra.resize(2);

  data.wb_addr = FPGA_CTRL_REGS | WB_MONITOR_CTRL; // monitor register (HW address)

  data.data_send[0] = 0x02;
  _commLink->fmc_config_send(&data);
  usleep(500000);

  data.data_send[0] = 0x04;
  _commLink->fmc_config_send(&data);
  usleep(500000);

  data.data_send[0] = 0x08;
  _commLink->fmc_config_send(&data);
  usleep(500000);

  return 0;
}

int fmc_config_130m_4ch_board::get_fmc_temp1(uint64_t *temp)
{
  LM75A_drv::LM75A_setCommLink(_commLink, LM75A_I2C_DRV);
  *(double *)temp = LM75A_drv::LM75A_readTemp(LM75A_ADDR_1);
  return 0;
}

int fmc_config_130m_4ch_board::get_fmc_temp2(uint64_t *temp)
{
  LM75A_drv::LM75A_setCommLink(_commLink, LM75A_I2C_DRV);
  *(double *)temp = LM75A_drv::LM75A_readTemp(LM75A_ADDR_2);
  return 0;
}

int fmc_config_130m_4ch_board::set_kx(uint32_t kx, uint32_t *kx_out) {
  wb_data data;

  data.data_send.resize(10);
  data.extra.resize(2);

  data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_KX;

  if (kx_out) {
    _commLink->fmc_config_read(&data);
    *kx_out = POS_CALC_KX_VAL_R(data.data_read[0]);
  }
  else {
    data.data_send[0] = POS_CALC_KX_VAL_W(kx);
    _commLink->fmc_config_send(&data);
  }

  return 0;
}

int fmc_config_130m_4ch_board::set_ky(uint32_t ky,  uint32_t *ky_out) {
  wb_data data;

  data.data_send.resize(10);
  data.extra.resize(2);

  data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_KY;

  if (ky_out) {
    _commLink->fmc_config_read(&data);
    *ky_out = POS_CALC_KY_VAL_R(data.data_read[0]);
  }
  else {
    data.data_send[0] = POS_CALC_KY_VAL_W(ky);
    _commLink->fmc_config_send(&data);
  }

  return 0;
}

int fmc_config_130m_4ch_board::set_ksum(uint32_t ksum, uint32_t *ksum_out) {
  wb_data data;

  data.data_send.resize(10);
  data.extra.resize(2);

  data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_KSUM;

  if (ksum_out) {
    _commLink->fmc_config_read(&data);
    *ksum_out = POS_CALC_KSUM_VAL_R(data.data_read[0]);
  }
  else {
    data.data_send[0] = POS_CALC_KSUM_VAL_W(ksum);
    _commLink->fmc_config_send(&data);
  }

  return 0;
}

int fmc_config_130m_4ch_board::set_sw_on(uint32_t *swon_out) {
  wb_data data;

  data.data_send.resize(10);
  data.data_read.resize(1);
  data.extra.resize(2);

  // Switching mode
  data.wb_addr = DSP_BPM_SWAP | BPM_SWAP_REG_CTRL;

  if (swon_out) {
    _commLink->fmc_config_read(&data);
    *swon_out = BPM_SWAP_CTRL_MODE1_R(data.data_read[0]) |
                BPM_SWAP_CTRL_MODE2_R(data.data_read[0]);
  }
  else {
    _commLink->fmc_config_read(&data);
    data.data_send[0] = (data.data_read[0] & ~BPM_SWAP_CTRL_MODE1_MASK & ~BPM_SWAP_CTRL_MODE2_MASK) |
                            BPM_SWAP_CTRL_MODE1_W(0x3)  |
                            BPM_SWAP_CTRL_MODE2_W(0x3); // Switching mode for both sets of channels
    _commLink->fmc_config_send(&data);
  }

  return 0;
}

int fmc_config_130m_4ch_board::set_sw_off(uint32_t *swoff_out) {
  wb_data data;

  data.data_send.resize(10);
  data.data_read.resize(1);
  data.extra.resize(2);

  // Switching mode
  data.wb_addr = DSP_BPM_SWAP | BPM_SWAP_REG_CTRL;

  if (swoff_out) {
    _commLink->fmc_config_read(&data);
    *swoff_out = BPM_SWAP_CTRL_MODE1_R(data.data_read[0]) |
                BPM_SWAP_CTRL_MODE2_R(data.data_read[0]);
  }
  else {
    _commLink->fmc_config_read(&data);
    data.data_send[0] = (data.data_read[0] & ~BPM_SWAP_CTRL_MODE1_MASK & ~BPM_SWAP_CTRL_MODE2_MASK) |
                          BPM_SWAP_CTRL_MODE1_W(0x1)  |
                          BPM_SWAP_CTRL_MODE2_W(0x1); // Switching mode for both sets of channels
    _commLink->fmc_config_send(&data);
  }

  return 0;
}

int fmc_config_130m_4ch_board::set_sw_clk_en_on(uint32_t *swclk_en_out) {
  wb_data data;

  data.data_send.resize(10);
  data.data_read.resize(1);
  data.extra.resize(2);

  // Switching mode
  data.wb_addr = DSP_BPM_SWAP | BPM_SWAP_REG_CTRL;

  if (swclk_en_out) {
    _commLink->fmc_config_read(&data);
    *swclk_en_out = (data.data_read[0] & BPM_SWAP_CTRL_CLK_SWAP_EN) >> 24;
  }
  else {
    _commLink->fmc_config_read(&data);
    data.data_send[0] = data.data_read[0] | BPM_SWAP_CTRL_CLK_SWAP_EN;
    _commLink->fmc_config_send(&data);
  }

  return 0;
}

int fmc_config_130m_4ch_board::set_sw_clk_en_off(uint32_t *swclk_en_out) {
  wb_data data;

  data.data_send.resize(10);
  data.data_read.resize(1);
  data.extra.resize(2);

  // Switching mode
  data.wb_addr = DSP_BPM_SWAP | BPM_SWAP_REG_CTRL;

  if (swclk_en_out) {
    _commLink->fmc_config_read(&data);
    *swclk_en_out = (data.data_read[0] & BPM_SWAP_CTRL_CLK_SWAP_EN) >> 24;
  }
  else {
    _commLink->fmc_config_read(&data);
    data.data_send[0] = data.data_read[0] & ~BPM_SWAP_CTRL_CLK_SWAP_EN;
    _commLink->fmc_config_send(&data);
  }

  return 0;
}

int fmc_config_130m_4ch_board::set_sw_divclk(uint32_t divclk, uint32_t *divclk_out) {
  wb_data data;

  data.data_send.resize(10);
  data.data_read.resize(1);
  data.extra.resize(2);

  // Switching mode
  data.wb_addr = DSP_BPM_SWAP | BPM_SWAP_REG_CTRL;

  if (divclk_out) {
    _commLink->fmc_config_read(&data);
    *divclk_out = BPM_SWAP_CTRL_SWAP_DIV_F_R(data.data_read[0]);
  }
  else {
    _commLink->fmc_config_read(&data);
    data.data_send[0] = (data.data_read[0] & ~BPM_SWAP_CTRL_SWAP_DIV_F_MASK) |
                            BPM_SWAP_CTRL_SWAP_DIV_F_W(divclk); // Clock divider for swap clk
    _commLink->fmc_config_send(&data);
  }

  return 0;
}

int fmc_config_130m_4ch_board::set_sw_phase(uint32_t phase, uint32_t *phase_out) {
  wb_data data;

  data.data_send.resize(10);
  data.extra.resize(2);

  // Switching mode
  data.wb_addr = DSP_BPM_SWAP | BPM_SWAP_REG_DLY;

  if (phase_out) {
    _commLink->fmc_config_read(&data);
    *phase_out = BPM_SWAP_DLY_1_R(data.data_read[0]) |
                  BPM_SWAP_DLY_2_R(data.data_read[0]);
  }
  else {
    _commLink->fmc_config_read(&data);
    data.data_send[0] = (data.data_read[0] & ~BPM_SWAP_DLY_1_MASK &
                         ~BPM_SWAP_DLY_2_MASK) |
                          BPM_SWAP_DLY_1_W(phase) |
                          BPM_SWAP_DLY_2_W(phase); // Phase delay for swap clk
    _commLink->fmc_config_send(&data);
  }

  return 0;
}

int fmc_config_130m_4ch_board::set_wdw_on(uint32_t *wdwon_out) {
  wb_data data;

  data.data_send.resize(10);
  data.data_read.resize(1);
  data.extra.resize(2);

  // Windowing Selection mode
  data.wb_addr = DSP_BPM_SWAP | BPM_SWAP_REG_WDW_CTL;

  if (wdwon_out) {
    _commLink->fmc_config_read(&data);
    *wdwon_out = data.data_read[0];
  }
  else {
    data.data_send[0] = (BPM_SWAP_WDW_CTL_USE |
                         BPM_SWAP_WDW_CTL_SWCLK_EXT); // Use windowing and external clock
    _commLink->fmc_config_send(&data);
  }

  return 0;
}

int fmc_config_130m_4ch_board::set_wdw_off(uint32_t *wdwon_out) {
  wb_data data;

  data.data_send.resize(10);
  data.data_read.resize(1);
  data.extra.resize(2);

  // Windowing Selection mode
  data.wb_addr = DSP_BPM_SWAP | BPM_SWAP_REG_WDW_CTL;

  if (wdwon_out) {
    _commLink->fmc_config_read(&data);
    *wdwon_out = data.data_read[0];
  }
  else {
    _commLink->fmc_config_read(&data);
    data.data_send[0] = (data.data_read[0] & ~BPM_SWAP_WDW_CTL_USE &
                         ~BPM_SWAP_WDW_CTL_SWCLK_EXT); // Don't use windowing and switch to internal clock
    _commLink->fmc_config_send(&data);
  }

  return 0;
}

int fmc_config_130m_4ch_board::set_wdw_dly(uint32_t wdw_dly, uint32_t *wdw_dly_out)
{
  wb_data data;

  data.data_send.resize(10);
  data.extra.resize(2);

  // Switching mode
  data.wb_addr = DSP_BPM_SWAP | BPM_SWAP_REG_WDW_CTL;

  if (wdw_dly_out) {
    _commLink->fmc_config_read(&data);
    *wdw_dly_out = BPM_SWAP_WDW_CTL_DLY_R(data.data_read[0]);
  }
  else {
    _commLink->fmc_config_read(&data);
    data.data_send[0] = (data.data_read[0] & ~BPM_SWAP_WDW_CTL_DLY_MASK) |
                            BPM_SWAP_WDW_CTL_DLY_W(wdw_dly);
    _commLink->fmc_config_send(&data);
  }

  return 0;
}

int fmc_config_130m_4ch_board::set_adc_clk(uint32_t adc_clk, uint32_t *adc_clk_out)
{
    if (adc_clk_out) {
        *adc_clk_out = this->adc_clk;
    }
    else {
      this->adc_clk = adc_clk;
    }

    return 0;
}

int fmc_config_130m_4ch_board::set_dds_freq(uint32_t dds_freq, uint32_t *dds_freq_out) {
  wb_data data;
  data.data_send.resize(10);
  data.extra.resize(2);
  data.extra[0] = 1;

  // Phase increment mode
  data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DDS_PINC_CH0;

  if (dds_freq_out) {
    _commLink->fmc_config_read(&data);
    uint32_t pinc = POS_CALC_DDS_PINC_CH0_VAL_R(data.data_read[0]);
    *dds_freq_out = floor((pinc / (double) (1 << 30)) * this->adc_clk);
  }
  else {
    uint32_t pinc =  floor((dds_freq / (double) this->adc_clk) * (1 << 30));

    data.data_send[0] = POS_CALC_DDS_PINC_CH0_VAL_W(pinc);  // phase increment ch0
    //data.data_send[0] = 23536678;  // phase increment ch0
    _commLink->fmc_config_send(&data);

    data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DDS_PINC_CH1;
    data.data_send[0] = POS_CALC_DDS_PINC_CH1_VAL_W(pinc);  // phase increment ch1
    //data.data_send[0] = 23536678;  // phase increment ch1
    _commLink->fmc_config_send(&data);

    data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DDS_PINC_CH2;
    data.data_send[0] = POS_CALC_DDS_PINC_CH2_VAL_W(pinc);  // phase increment ch2
    //data.data_send[0] = 23536678;  // phase increment ch2
    _commLink->fmc_config_send(&data);

    data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DDS_PINC_CH3;
    data.data_send[0] = POS_CALC_DDS_PINC_CH3_VAL_W(pinc);  // phase increment ch3
    //data.data_send[0] = 23536678;  // phase increment ch3
    _commLink->fmc_config_send(&data);

    //data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DDS_CFG;
    //// toggle valid signal for all four DDS's
    //data.data_send[0] = (0x1) | (0x1 << 8) | (0x1 << 16) | (0x1 << 24);
    //_commLink->fmc_config_send(&data);

    // FIXME FIXME FIXME. Wrong FPGA firmware!!!
    unsigned int i;
    for (i = 0; i < 10; ++i) {
      data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DDS_CFG;
      // toggle valid signal for all four DDS's
      data.data_send[0] = POS_CALC_DDS_CFG_VALID_CH0 |
                    POS_CALC_DDS_CFG_VALID_CH1 |
                    POS_CALC_DDS_CFG_VALID_CH2 |
                    POS_CALC_DDS_CFG_VALID_CH3;
      _commLink->fmc_config_send(&data);
    }

    data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DDS_PINC_CH0;
    _commLink->fmc_config_read(&data);
    printf ("DDS PINC CH0 set to: %d\n",
    POS_CALC_DDS_PINC_CH0_VAL_R(data.data_read[0]));

    data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DDS_PINC_CH1;
    _commLink->fmc_config_read(&data);
    printf ("DDS PINC CH1 set to: %d\n",
    POS_CALC_DDS_PINC_CH1_VAL_R(data.data_read[0]));

    data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DDS_PINC_CH2;
    _commLink->fmc_config_read(&data);
    printf ("DDS PINC CH2 set to: %d\n",
    POS_CALC_DDS_PINC_CH2_VAL_R(data.data_read[0]));

    data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DDS_PINC_CH3;
    _commLink->fmc_config_read(&data);
    printf ("DDS PINC CH3 set to: %d\n",
    POS_CALC_DDS_PINC_CH3_VAL_R(data.data_read[0]));
  }

  return 0;
}

//#define MAX_TRIES 10
#define MAX_TRIES (1 << 28)
/* FIXME: In FPGA ADC samples fill both streams
 * of the acquisition channel, so the number of samples
 * the user wants are interpreted as only half */
#define NUM_SAMPLES_8_CORR 1 /* Correction factor for 16-bit
                                (8-byte, 4 channels) samples */
#define NUM_SAMPLES_16_CORR 1 /* Correction factor for 32-bit
                                 (16-byte, 4 channels) samples */
//#define NUM_SAMPLES_8_CORR 1 /* Correction factor for 16-bit
//                                (8-byte, 4 channels) samples */
//#define NUM_SAMPLES_16_CORR 1 /* Correction factor for 32-bit
//                                 (16-byte, 4 channels) samples */

// Acquire data with previously set parameters
int fmc_config_130m_4ch_board::set_data_acquire(/*uint32_t num_samples, uint32_t offset, int acq_chan*/)
{
  wb_data data;
  uint32_t acq_core_ctl_reg;
  uint32_t samples_size = ddr3_acq_chan[this->acq_chan_n].samples_size;
  int tries = 0;

  data.data_send.resize(10);
  data.extra.resize(3);

  DEBUGP ("set_data_acq: parameters:\n");
  DEBUGP ("num samples = %d\n", this->acq_nsamples_n);
  DEBUGP ("acq_chan = %d\n", this->acq_chan_n);
  DEBUGP ("acq_offset = %d\n", this->acq_offset_n);

  // sets the number of samples of this acq_chan
  acq_last_params[this->acq_chan_n].acq_nsamples = this->acq_nsamples_n;
  DEBUGP ("last params nsamples = %d\n",
          acq_last_params[this->acq_chan_n].acq_nsamples);

  DEBUGP ("wb_acq_addr = 0x%x\n", WB_ACQ_BASE_ADDR);
  DEBUGP ("shot reg offset = 0x%x\n", ACQ_CORE_REG_SHOTS);

  // Num shots
  DEBUGP ("Num shots offset = 0x%x\n", ACQ_CORE_REG_PRE_SAMPLES);
  /* FIXME!! */
  /* Wrong FPGA firmware */
  data.extra[2] = 1;
  data.wb_addr = WB_ACQ_BASE_ADDR | ACQ_CORE_REG_SHOTS;
  data.data_send[0] = ACQ_CORE_SHOTS_NB_W(1);
  _commLink->fmc_config_send(&data);

  // Pre-trigger samples
  DEBUGP ("Pre trig offset = 0x%x\n", ACQ_CORE_REG_PRE_SAMPLES);
  /* FIXME!! */
  /* Wrong FPGA firmware */
  data.extra[2] = 1;
  data.wb_addr = WB_ACQ_BASE_ADDR | ACQ_CORE_REG_PRE_SAMPLES;

  /* Apply correction factor */
  if (samples_size == 8) { // ADC
    data.data_send[0] = this->acq_nsamples_n * NUM_SAMPLES_8_CORR;
  }
  else { // All others
    data.data_send[0] = this->acq_nsamples_n * NUM_SAMPLES_16_CORR;
  }

  _commLink->fmc_config_send(&data);

  // Pos-trigger samples
  DEBUGP ("pos trig offset = 0x%x\n", ACQ_CORE_REG_POST_SAMPLES);
  /* FIXME!! */
  /* Wrong FPGA firmware */
  data.extra[2] = 1;
  data.wb_addr = WB_ACQ_BASE_ADDR | ACQ_CORE_REG_POST_SAMPLES;
  data.data_send[0] = 0;
  _commLink->fmc_config_send(&data);

  // DDR3 start address
  DEBUGP ("DDR3 Start addr = 0x%x\n", ACQ_CORE_REG_DDR3_START_ADDR);
  DEBUGP ("DDR3 Start val = 0x%x\n", ddr3_acq_chan[this->acq_chan_n].start_addr);
  /* FIXME!! */
  /* Wrong FPGA firmware */
  data.extra[2] = 1;
  data.wb_addr = WB_ACQ_BASE_ADDR | ACQ_CORE_REG_DDR3_START_ADDR;
  //data.data_send[0] = this->acq_offset_n;
  //data.data_send[0] = ddr3_acq_chan[this->acq_chan_n].start_addr;
  data.data_send[0] = ddr3_acq_chan[this->acq_chan_n].start_addr/8;
  //data.data_send[0] = 0x100000/8;
  _commLink->fmc_config_send(&data);

  // Prepare core_ctl register
  DEBUGP ("Skip trigger addr = 0x%x\n", ACQ_CORE_REG_CTL);
  /* FIXME!! */
  /* Wrong FPGA firmware */
  data.extra[2] = 1;
  acq_core_ctl_reg = ACQ_CORE_CTL_FSM_ACQ_NOW;
  data.wb_addr = WB_ACQ_BASE_ADDR | ACQ_CORE_REG_CTL;
  data.data_send[0] = acq_core_ctl_reg;
  _commLink->fmc_config_send(&data);

  // Prepare acquisition channel control
  DEBUGP ("Acq Channel addr = 0x%x\n", ACQ_CORE_REG_ACQ_CHAN_CTL);
  /* FIXME!! */
  /* Wrong FPGA firmware */
  data.extra[2] = 1;
  data.wb_addr = WB_ACQ_BASE_ADDR | ACQ_CORE_REG_ACQ_CHAN_CTL;
  data.data_send[0] = ACQ_CORE_ACQ_CHAN_CTL_WHICH_W(this->acq_chan_n);
  _commLink->fmc_config_send(&data);

  // Starting acquisition...
  DEBUGP ("Start Acq addr = 0x%x\n", ACQ_CORE_REG_CTL);
  /* FIXME!! */
  /* Wrong FPGA firmware */
  data.extra[2] = 1;
  acq_core_ctl_reg |= ACQ_CORE_CTL_FSM_START_ACQ;
  data.wb_addr = WB_ACQ_BASE_ADDR | ACQ_CORE_REG_CTL;
  data.data_send[0] = acq_core_ctl_reg;
  _commLink->fmc_config_send(&data);

  DEBUGP ("acquisition started!\n");

  // Check for completion
  do {
    /* FIXME!! */
    /* Wrong FPGA firmware */
    usleep(100000); // 100msec wait
    data.extra[2] = 1;
    data.data_read.clear();
    data.wb_addr = WB_ACQ_BASE_ADDR | ACQ_CORE_REG_STA;
    _commLink->fmc_config_read(&data);
    ++tries;
    DEBUGP ("waiting for completion... #%d\n", tries);
    DEBUGP ("status done = 0x%X\n", data.data_read[0]);
  } while (!(data.data_read[0] & ACQ_CORE_STA_DDR3_TRANS_DONE) && (tries < MAX_TRIES));

  if (tries == MAX_TRIES) {
    return -3; // exceeded number of tries
  }

  return 0;
}

/*int fmc_config_130m_4ch_board::set_acq_params(uint32_t acq_nsamples, uint32_t acq_chan,
                                                uint32_t acq_offset, uint32_t *acq_nsamples_out,
                                                uint32_t *acq_chan_out, uint32_t *acq_offset_out)
{
  if (acq_nsamples_out) {
      DEBUGP ("set_acq_params: getting acq_nsamples to %d\n", this->acq_nsamples_n);
    *acq_nsamples_out = acq_nsamples_n;
  }
  else {
      DEBUGP ("set_acq_params: setting acq_nsamples to %d\n", acq_nsamples);
    acq_nsamples_n = acq_nsamples;
      DEBUGP ("set_acq_params: this->acq_nsamples_n = %d\n", acq_nsamples_n);
  }

  if (acq_chan_out) {
    *acq_chan_out = this->acq_chan_n;
  }
  else {
    this->acq_chan_n = acq_chan;
  }

  if (acq_offset_out) {
    *acq_offset_out = this->acq_offset_n;
  }
  else {
    this->acq_offset_n = acq_offset;
  }

  return 0;
}*/


int fmc_config_130m_4ch_board::set_acq_nsamples(uint32_t acq_nsamples)
{
    DEBUGP ("set_acq_nsamples....\n");
    this->acq_nsamples_n = acq_nsamples;
    DEBUGP ("this->nsamples is: %d\n", this->acq_nsamples_n);
    DEBUGP ("acq_nsamples is: %d\n", acq_nsamples);
    return 0;
}

int fmc_config_130m_4ch_board::get_acq_nsamples(uint32_t *acq_nsamples)
{
    DEBUGP ("get_acq_nsamples....\n");
    *acq_nsamples = this->acq_nsamples_n;
    DEBUGP ("this->nsamples_n is: %d\n", this->acq_nsamples_n);
    DEBUGP ("*acq_nsamples is: %d\n", *acq_nsamples);
    return 0;
}

int fmc_config_130m_4ch_board::set_acq_chan(uint32_t acq_chan)
{
    this->acq_chan_n = acq_chan;
    return 0;
}

int fmc_config_130m_4ch_board::get_acq_chan(uint32_t *acq_chan)
{
    *acq_chan = this->acq_chan_n;
    return 0;
}

int fmc_config_130m_4ch_board::set_acq_offset(uint32_t acq_offset)
{
    this->acq_offset_n = acq_offset;
    return 0;
}

int fmc_config_130m_4ch_board::get_acq_offset(uint32_t *acq_offset)
{
    *acq_offset = this->acq_offset_n;
    return 0;
}

int fmc_config_130m_4ch_board::get_acq_data_block(uint32_t acq_chan,
        uint32_t acq_offs, uint32_t acq_bytes,
        uint32_t *data_out, uint32_t *acq_bytes_out)
{
  wb_data data;
  data.extra.resize(2);

  data.wb_addr = ddr3_acq_chan[acq_chan].start_addr + acq_offs;
  uint32_t acq_end = acq_last_params[acq_chan].acq_nsamples *
                            ddr3_acq_chan[acq_chan].samples_size;

  DEBUGP ("get_acq_block: last_nsamples = %d\n",
          acq_last_params[acq_chan].acq_nsamples);
  DEBUGP ("get_acq_block: samples size = %d\n",
          ddr3_acq_chan[acq_chan].samples_size);
  DEBUGP ("get_acq_block: addr = %d, acq_offs = %d\n",
          data.wb_addr, acq_offs);
  DEBUGP ("acq_bytes = %d, acq_end = %d\n", acq_bytes, acq_end);

  // No data to be read
  if (acq_offs > acq_end) {
    *acq_bytes_out = 0;
    return 0;
  }

  DEBUGP ("Checking out of bounds request...\n");
  // ughhhh... check for out of bounds request
  if (acq_offs + acq_bytes >= acq_end) {
    data.extra[0] = acq_end - acq_offs;
  }
  else {
    data.extra[0] = acq_bytes;
  }

  *acq_bytes_out = data.extra[0];

  // always from bar2
  DEBUGP ("reading %d bytes from bar2\n", data.extra[0]);
  _commLink->fmc_config_read_unsafe(&data, data_out);

  return 0;
}

/* For monitoring rates */
/* data_out must be 4*32-bits !! */
int fmc_config_130m_4ch_board::get_acq_sample_monit_amp(uint32_t *data_out, uint32_t *len)
{
  wb_data data;
  data.extra.resize(2);

  data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DSP_MONIT_AMP_CH0;
  _commLink->fmc_config_read(&data);
  data_out[0] = data.data_read[0];

  data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DSP_MONIT_AMP_CH1;
  _commLink->fmc_config_read(&data);
  data_out[1] = data.data_read[0];

  data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DSP_MONIT_AMP_CH2;
  _commLink->fmc_config_read(&data);
  data_out[2] = data.data_read[0];

  data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DSP_MONIT_AMP_CH3;
  _commLink->fmc_config_read(&data);
  data_out[3] = data.data_read[0];

  *len = 16; // 4 32-bit samples

  return 0;
}

/* For monitoring rates */
/* data_out must be 4*32-bits !! */
int fmc_config_130m_4ch_board::get_acq_sample_monit_pos(uint32_t *data_out, uint32_t *len)
{
  wb_data data;
  data.extra.resize(2);

  data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DSP_MONIT_POS_X;
  _commLink->fmc_config_read(&data);
  data_out[0] = data.data_read[0];

  data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DSP_MONIT_POS_Y;
  _commLink->fmc_config_read(&data);
  data_out[1] = data.data_read[0];

  data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DSP_MONIT_POS_Q;
  _commLink->fmc_config_read(&data);
  data_out[2] = data.data_read[0];

  data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DSP_MONIT_POS_SUM;
  _commLink->fmc_config_read(&data);
  data_out[3] = data.data_read[0];

  *len = 16; // 4 32-bit samples

  return 0;
}
