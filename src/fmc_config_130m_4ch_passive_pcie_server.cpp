//============================================================================
// Author      : Andrzej Wojenski
// Version     : 1.0
// Description : Software driver for configuration FMC ADC 130M 4CH card (PASSIVE version)
//               Also checks and tests avaiable chips on the board
//                               Includes:
//               - firmware identification
//                               - LED configuration
//               - trigger configuration
//               - LM75A configuration and data acquisition (temperature sensor)
//               - EEPROM 24A64 check
//               - LTC2208 configuration (4 ADC chips)
//               - Clock and data lines calibation (IDELAY)
//               - most of the functions provides assertions to check if data is written to the chip (checks readback value)
//============================================================================
//#include "reg_map/fmc_config_130m_4ch.h"

#include <iostream>
#include <string>
#include <unistd.h> /* getopt */
#include <iostream>
#include <iomanip>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <stdint.h>

#include "fmc_config_130m_4ch_board.h"

#include "config.h"
#include "plat_opts.h"
#include "data.h"
//#include "commlink/commLink.h"
#include "wishbone/rs232_syscon.h"
#include "wishbone/pcie_link.h"
//#include "interface/i2c.h"
//#include "interface/spi.h"
//#include "interface/gpio.h"
//#include "chip/si570.h"
//#include "chip/ad9510.h"
//#include "chip/isla216p.h"
//#include "chip/amc7823.h"
//#include "chip/eeprom_24a64.h"
//#include "chip/lm75a.h"
#include "platform/fmc130m_plat.h"
#include "common.h"
#include "tcp_server.h"

using namespace std;

#define S "sllp_server: "

fmc_config_130m_4ch_board *fmc_config_130m_4ch_board_p;
tcp_server *tcp_server_p;

/***************************************************************/ 
/**********************   SLLP methods  **********************/
/***************************************************************/
static uint8_t ad_convert(uint8_t *input, uint8_t *output)
{
    printf(S"Starting conversion of the A/D converters...\n");
    
    return 0; // Success!!
}

static struct sllp_func ad_convert_func = {
  {0, 0, 0},
  ad_convert
};

uint8_t fmc130m_blink_leds(uint8_t *input, uint8_t *output)
{
    printf(S"blinking leds!...\n");

    fmc_config_130m_4ch_board_p->blink_leds();
    
    return 0; // Success!!
}

static struct sllp_func fmc130m_blink_leds_func = {
  {0, 0, 0},
  fmc130m_blink_leds
};

int main(int argc, const char **argv) {

  cout << "FMC configuration software for FMC ADC 130M 4CH card (PASSIVE version)" << endl <<
                  "Author: Andrzej Wojenski" << endl;

  //WBInt_drv* int_drv;
  wb_data data;
  vector<uint16_t> amc_temp;
  vector<uint16_t> test_pattern;
  //commLink* _commLink = new commLink();
  //fmc_config_130m_4ch_board *fmc_config_130m_4ch_board_p;
  //tcp_server *tcp_server_p;
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
          delay_data_l = fmc_130m_ml605_delay_data_l;
          delay_clk_l = fmc_130m_ml605_delay_clk_l;
          platform_name = ML605_STRING;
          break;
        case KC705:
          delay_data_l = fmc_130m_kc705_delay_l;
          platform_name = KC705_STRING;
          break;
        case AFC:
          delay_data_l = fmc_130m_afc_delay_l;
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

  fmc_config_130m_4ch_board_p = new fmc_config_130m_4ch_board(new pcie_link_driver(0),
                                                  delay_data_l,
                                                  delay_clk_l);

  //data.data_send.resize(10);
  //data.extra.resize(2);
  //
  //// CommLink configuration
  //// Adding communication interfaces
  //// PCI-E version, 0 is 
  ////for Linux driver, for example if you have more AFC cards you can put 
  ////there 1,2,3 (I will in near future work on automapping of uTCA crate for 
  ////AFC cards and occupied slots).
  //_commLink->regWBMaster(new pcie_link_driver(0));
  //
  //int_drv = _commLink->regIntDrv(SI571_I2C_DRV, FPGA_SI571_I2C, new i2c_int());
  //((i2c_int*)int_drv)->i2c_init(FPGA_SYS_FREQ, 100000); // 100kHz
  //
  //int_drv = _commLink->regIntDrv(AD9510_SPI_DRV, FPGA_AD9510_SPI, new spi_int());
  //((spi_int*)int_drv)->spi_init(FPGA_SYS_FREQ, 1000000, 0x2400); // 1MHZ, ASS = 1,
  ////TX_NEG = 1 (data changed on falling edge), RX_NEG = 0 (data latched on rising edge)
  //
  //int_drv = _commLink->regIntDrv(EEPROM_I2C_DRV, FPGA_EEPROM_I2C, new i2c_int());
  //((i2c_int*)int_drv)->i2c_init(FPGA_SYS_FREQ, 400000); // 400kHz
  //
  //int_drv = _commLink->regIntDrv(LM75A_I2C_DRV, FPGA_LM75A_I2C, new i2c_int());
  //((i2c_int*)int_drv)->i2c_init(FPGA_SYS_FREQ, 400000); // 400kHz
  //
  //_commLink->regIntDrv(GENERAL_GPIO_DRV, FPGA_CTRL_REGS, new gpio_int());
  //
  //// ======================================================
  ////                Firmware identification
  //// ======================================================
  //cout << "============================================" << endl <<
  //        "            Firmware identification         " << endl <<
  //        "============================================" << endl;
  //
  //data.wb_addr = FPGA_CTRL_REGS | WB_FMC_STATUS; // FMC status register (HW address)
  //
  //// check id data
  //_commLink->fmc_config_read(&data);
  //
  //cout << "Reg: " << hex << (data.data_read[0]) << endl;
  //cout << "Firmware ID: " << hex << (data.data_read[0] >> 3) <<
  //                "(" << dec << (data.data_read[0] >> 3)<< ")" << endl; // should be 0x01332A11 (20130321)
  //
  //// Check communication
  //assert(0x01332A11 == (data.data_read[0] >> 3) );
  //
  //// ======================================================
  ////                  LEDs configuration
  //// ======================================================
  //cout << "============================================" << endl <<
  //        "            LEDs configuration              " << endl <<
  //        "============================================" << endl;
  //
  //data.wb_addr = FPGA_CTRL_REGS | WB_MONITOR_CTRL; // monitor register (HW address)
  //
  //data.data_send[0] = 0x02;
  //_commLink->fmc_config_send(&data);
  ////sleep(4);
  //
  //data.data_send[0] = 0x04;
  //_commLink->fmc_config_send(&data);
  ////sleep(4);
  //
  //data.data_send[0] = 0x08;
  //_commLink->fmc_config_send(&data);
  ////sleep(4);
  //
  //// Check if data properly written
  //_commLink->fmc_config_read(&data);
  //assert( (data.data_read[0] & 0x0E) == 0x08); // ignore TEMP_ALARM pin
  //
  //data.data_send[0] = 0x0E;
  //_commLink->fmc_config_send(&data);
  ////sleep(4);
  //
  //// Set status config (blue LED)
  //data.data_send[0] = 0x02;
  //_commLink->fmc_config_send(&data);
  //
  //// for trigger test
  ////data.data_send[0] = 0x00;
  ////_commLink->fmc_config_send(&data);
  //
  //// ======================================================
  ////                  Trigger configuration
  //// ======================================================
  //cout << "============================================" << endl <<
  //        "            Trigger configuration           " << endl <<
  //        "============================================" << endl;
  //
  //data.wb_addr = FPGA_CTRL_REGS | WB_TRG_CTRL; // trigger control
  //data.data_send[0] = 0x01;
  //_commLink->fmc_config_send(&data);
  //
  //// ======================================================
  ////        LM75A configuration (temperature monitor)
  //// ======================================================
  //cout << "============================================" << endl <<
  //        "             LM75A check data    " << endl <<
  //        "============================================" << endl;
  //// lm75 i2c have timeout
  //
  //// Why is it not working?????
  //LM75A_drv::LM75A_setCommLink(_commLink, LM75A_I2C_DRV);
  //printf("LM75A chip number 1, chip ID: 0x%02x (should be 0xA1)\n", LM75A_drv::LM75A_readID(LM75A_ADDR_1));
  //printf("LM75A chip number 1, temperature: %f *C\n", LM75A_drv::LM75A_readTemp(LM75A_ADDR_1));
  //printf("LM75A chip number 2, chip ID: 0x%02x (should be 0xA1)\n", LM75A_drv::LM75A_readID(LM75A_ADDR_2));
  //printf("LM75A chip number 2, temperature: %f *C\n", LM75A_drv::LM75A_readTemp(LM75A_ADDR_2));
  //
  //// ======================================================
  ////                  EEPROM configuration
  //// ======================================================
  //cout << "============================================" << endl <<
  //        "              EEPROM check    " << endl <<
  //        "============================================" << endl;
  //
  //EEPROM_drv::EEPROM_setCommLink(_commLink, EEPROM_I2C_DRV);
  //EEPROM_drv::EEPROM_switch(0x02); // according to documentation, switches to i2c fmc lines
  ////EEPROM_drv::EEPROM_sendData(0x00); // wrong address
  //EEPROM_drv::EEPROM_sendData(EEPROM_ADDR); // good address
  //
  //// ======================================================
  ////                  LTC ADC configuration
  //// ======================================================
  //cout << "============================================" << endl <<
  //        "     LTC2208 config (4 ADC chips)     " << endl <<
  //        "============================================" << endl;
  //
  //data.wb_addr = FPGA_CTRL_REGS | WB_ADC_LTC_CTRL; // trigger control
  ////data.data_send[0] = 0x02; // dither on, power on, random off, pga off (input 2.25 Vpp)
  //data.data_send[0] = 0x00; // dither off, power on, random off, pga off (input 2.25 Vpp)
  //_commLink->fmc_config_send(&data);
  //
  //// ======================================================
  ////            Clock and data lines calibration
  //// ======================================================
  //cout << "============================================" << endl <<
  //        "      Clock and data lines calibration     " << endl <<
  //        "============================================" << endl;
  //
  //// Calibration
  //// reset IDELAYCTRLs in FPGA
  //data.wb_addr = FPGA_CTRL_REGS | WB_FPGA_CTRL;
  //data.data_send[0] = 0x01;
  //_commLink->fmc_config_send(&data);
  //sleep(1);
  //data.data_send[0] = 0x00;
  //_commLink->fmc_config_send(&data);
  //
  //// check if ready
  //_commLink->fmc_config_read(&data);
  //printf("data: %08x\n", data.data_read[0]);
  //
  //if ( (data.data_read[0] >> 2) & 0x0F ) // check adc0 adc1 adc2 adc3 idelayctrl
  //        cout << "All IDELAY controllers had been reset and are ready to work!" << endl;
  //else {
  //        cout << "Error while resetting IDELAY controllers, some of them are not ready!" << endl;
  //        exit(1);
  //}
  //
  //cout << "IDELAY lines calibration" << endl;
  //
  //// upload new tap value (130m)
  //// for fmc adc 130m
  //// adc0
  //// adc1
  //// adc2
  //// adc3
  //// tap resolution 78ps
  //
  //set_fpga_delay_s(_commLink, FPGA_CTRL_REGS | WB_IDELAY0_CAL, &delay_data_l[0], DLY_DATA);
  //set_fpga_delay_s(_commLink, FPGA_CTRL_REGS | WB_IDELAY1_CAL, &delay_data_l[1], DLY_DATA);
  //set_fpga_delay_s(_commLink, FPGA_CTRL_REGS | WB_IDELAY2_CAL, &delay_data_l[2], DLY_DATA);
  //set_fpga_delay_s(_commLink, FPGA_CTRL_REGS | WB_IDELAY3_CAL, &delay_data_l[3], DLY_DATA);
  //
  //set_fpga_delay_s(_commLink, FPGA_CTRL_REGS | WB_IDELAY0_CAL, &delay_clk_l[0], DLY_CLK);
  //set_fpga_delay_s(_commLink, FPGA_CTRL_REGS | WB_IDELAY1_CAL, &delay_clk_l[1], DLY_CLK);
  //set_fpga_delay_s(_commLink, FPGA_CTRL_REGS | WB_IDELAY2_CAL, &delay_clk_l[2], DLY_CLK);
  //set_fpga_delay_s(_commLink, FPGA_CTRL_REGS | WB_IDELAY3_CAL, &delay_clk_l[3], DLY_CLK);
  //
  //// train communication links
  //
  //// ======================================================
  ////                BPM Swap configuration
  //// ======================================================
  //cout << "============================================" << endl <<
  //        "           BPM Swap configuration           " << endl <<
  //        "============================================" << endl;
  //
  //// BPM Swap parameters
  //data.wb_addr = DSP_BPM_SWAP | BPM_SWAP_REG_A;
  //data.data_send[0] = 32768 | 32768 << 16; // no gain for AA and AC
  //_commLink->fmc_config_send(&data);
  //
  //data.wb_addr = DSP_BPM_SWAP | BPM_SWAP_REG_B;
  //data.data_send[0] = 32768 | 32768 << 16; // no gain for BB and BD
  //_commLink->fmc_config_send(&data);
  //
  //data.wb_addr = DSP_BPM_SWAP | BPM_SWAP_REG_C;
  //data.data_send[0] = 32768 | 32768 << 16; // no gain for CC and CA
  //_commLink->fmc_config_send(&data);
  //
  //data.wb_addr = DSP_BPM_SWAP | BPM_SWAP_REG_D;
  //data.data_send[0] = 32768 | 32768 << 16; // no gain for DD and DB
  //_commLink->fmc_config_send(&data);
  //
  //// Switching mode
  //data.wb_addr = DSP_BPM_SWAP | BPM_SWAP_REG_CTRL;
  //data.data_send[0] = 0x1 << 1 | 0x1 << 3; // Direct mode for both sets of channels
  //_commLink->fmc_config_send(&data);
  //
  //// ======================================================
  ////                DSP configuration
  //// ======================================================
  //cout << "============================================" << endl <<
  //        "            DSP configuration         " << endl <<
  //        "============================================" << endl;
  //
  ////// DSP parameters
  //data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DS_TBT_THRES;
  //data.data_send[0] = 0x0200;  // 1.2207e-04 FIX26_22
  //_commLink->fmc_config_send(&data);
  //
  //data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DS_FOFB_THRES;
  //data.data_send[0] = 0x0200;  // 1.2207e-04 FIX26_22
  //_commLink->fmc_config_send(&data);
  //
  //data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DS_MONIT_THRES;
  //data.data_send[0] = 0x0200;  // 1.2207e-04 FIX26_22
  //_commLink->fmc_config_send(&data);
  //
  //data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_KX;
  //data.data_send[0] = 8388608;  // 10000000 UFIX25_0
  //_commLink->fmc_config_send(&data);
  //
  //data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_KY;
  //data.data_send[0] = 8388608;  // 10000000 UFIX25_0
  //_commLink->fmc_config_send(&data);
  //
  //data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_KSUM;
  //data.data_send[0] = 0x0FFFFFF;  // 1.0 FIX25_24
  //_commLink->fmc_config_send(&data);
  //
  //// DDS config.
  //
  //data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DDS_PINC_CH0;
  //data.data_send[0] = 245366784;  // phase increment
  //_commLink->fmc_config_send(&data);
  //
  //data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DDS_PINC_CH1;
  //data.data_send[0] = 245366784;  // phase increment
  //_commLink->fmc_config_send(&data);
  //
  //data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DDS_PINC_CH2;
  //data.data_send[0] = 245366784;  // phase increment
  //_commLink->fmc_config_send(&data);
  //
  //data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DDS_PINC_CH3;
  //data.data_send[0] = 245366784;  // phase increment
  //_commLink->fmc_config_send(&data);
  //
  //data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DDS_CFG;
  //// toggle valid signal for all four DDS's
  //data.data_send[0] = (0x1) | (0x1 << 8) | (0x1 << 16) | (0x1 << 24);
  //_commLink->fmc_config_send(&data);
  //
  //
  //cout << "All done! All components on the FMC card had been configured and tested!" << endl <<
  //                "FMC card is ready to work!" << endl;
  //
  ///* Just for simple debug! */
  //if (0) {
  //  sleep(4);
  //
  //  // Monit. Data Polling
  //  cout << "Monit Amp Ch0   Monit Amp Ch1   Monit Amp Ch2   Monit Amp Ch3" << endl;
  //
  //  for (int i = 0; i < 16; ++i) {
  //    data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DSP_MONIT_AMP_CH0;
  //    _commLink->fmc_config_read(&data);
  //    cout << setw(10) << data.data_read[0] << "  ";
  //
  //    data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DSP_MONIT_AMP_CH1;
  //    _commLink->fmc_config_read(&data);
  //    cout << setw(10) << data.data_read[0] << "  ";
  //
  //    data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DSP_MONIT_AMP_CH2;
  //    _commLink->fmc_config_read(&data);
  //    cout << setw(10) << data.data_read[0] << "  ";
  //
  //    data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DSP_MONIT_AMP_CH3;
  //    _commLink->fmc_config_read(&data);
  //    cout << setw(10) << data.data_read[0] << endl;
  //  }
  //
  //  cout << "Monit Pos X   Monit Pos Y   Monit Pos Q   Monit Pos Sum" << endl;
  //
  //  for (int i = 0; i < 16; ++i) {
  //    data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DSP_MONIT_POS_X;
  //    _commLink->fmc_config_read(&data);
  //    cout << setw(10) << (static_cast<int>(data.data_read[0]));
  //    cout << "  ";
  //
  //    data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DSP_MONIT_POS_Y;
  //    _commLink->fmc_config_read(&data);
  //    cout << setw(10) << (static_cast<int>(data.data_read[0]));
  //    cout << "  ";
  //
  //    data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DSP_MONIT_POS_Q;
  //    _commLink->fmc_config_read(&data);
  //    cout << setw(10) << (static_cast<int>(data.data_read[0]));
  //    cout << "  ";
  //
  //    data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DSP_MONIT_POS_SUM;
  //    _commLink->fmc_config_read(&data);
  //    cout << setw(10) << (static_cast<int>(data.data_read[0]));
  //    cout << endl;
  //  }
  //}
  //
  //cout << "============================================" << endl <<
  //        "                  Error Counter             " << endl <<
  //        "============================================" << endl;
  ////data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DSP_ERR_CLR;
  ////data.data_send[0] = 0x1 | 0x1 << 1 | 0x1 << 2| 0x1 << 3;
  ////_commLink->fmc_config_send(&data);
  ////
  ////sleep(20);
  //
  //// TBT CH 01 ERROR
  //data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DSP_CTNR_TBT;
  //_commLink->fmc_config_read(&data);
  //cout << "TBT ch01 error count: " << POS_CALC_DSP_CTNR_TBT_CH01_R(data.data_read[0]);
  //cout << endl;
  //cout << "TBT ch23 error count: " << POS_CALC_DSP_CTNR_TBT_CH23_R(data.data_read[0]);
  //cout << endl;
  //
  //// FOFB CH 01 ERROR
  //data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DSP_CTNR_FOFB;
  //_commLink->fmc_config_read(&data);
  //cout << "FOFB ch01 error count: " << POS_CALC_DSP_CTNR_FOFB_CH01_R(data.data_read[0]);
  //cout << endl;
  //cout << "FOFB ch23 error count: " << POS_CALC_DSP_CTNR_FOFB_CH23_R(data.data_read[0]);
  //cout << endl;
  //
  //// Monit Part1 ERROR
  //data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DSP_CTNR1_MONIT;
  //_commLink->fmc_config_read(&data);
  //cout << "Monit CIC error count: " << POS_CALC_DSP_CTNR1_MONIT_CIC_R(data.data_read[0]);
  //cout << endl;
  //cout << "Monit CFIR error count: " << POS_CALC_DSP_CTNR1_MONIT_CFIR_R(data.data_read[0]);
  //cout << endl;
  //
  //// Monit Part2 ERROR
  //data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DSP_CTNR2_MONIT;
  //_commLink->fmc_config_read(&data);
  //cout << "Monit PFIR error count: " << POS_CALC_DSP_CTNR2_MONIT_PFIR_R(data.data_read[0]);
  //cout << endl;
  //cout << "Monit 0.1 error count: " << POS_CALC_DSP_CTNR2_MONIT_FIR_01_R(data.data_read[0]);
  //cout << endl;

  // Initialize TCP server and SLLP library

  tcp_server_p = new tcp_server(string("8080"), fmc_config_130m_4ch_board_p);
  
  // Register functions
  tcp_server_p->register_func(&ad_convert_func);
  tcp_server_p->register_func(&fmc130m_blink_leds_func);
  
  /* Endless tcp loop */
  tcp_server_p->start();

  return 0;
}
