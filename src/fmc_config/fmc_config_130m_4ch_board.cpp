//============================================================================
// Author      : Lucas Russo
// Version     : 1.0
// Description : Software driver for PCIe-Wishbone Master IP core
//============================================================================
#include "fmc_config_130m_4ch_board.h"

fmc_config_130m_4ch_board::fmc_config_130m_4ch_board(WBMaster_unit* wb_master_unit, const delay_lines *delay_data_l,
                                        const delay_lines *delay_clk_l) {

	init(wb_master_unit, delay_data_l, delay_clk_l);
	config_defaults();

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

  // Why is it not working?????
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

  // Switching mode
  data.wb_addr = DSP_BPM_SWAP | BPM_SWAP_REG_CTRL;
  data.data_send[0] = 0x1 << 1 | 0x1 << 3; // Direct mode for both sets of channels
  _commLink->fmc_config_send(&data);

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

  data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_KX;
  data.data_send[0] = 8388608;  // 10000000 UFIX25_0
  _commLink->fmc_config_send(&data);

  data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_KY;
  data.data_send[0] = 8388608;  // 10000000 UFIX25_0
  _commLink->fmc_config_send(&data);

  data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_KSUM;
  data.data_send[0] = 0x0FFFFFF;  // 1.0 FIX25_24
  _commLink->fmc_config_send(&data);

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

  data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DDS_CFG;
  // toggle valid signal for all four DDS's
  data.data_send[0] = (0x1) | (0x1 << 8) | (0x1 << 16) | (0x1 << 24);
  _commLink->fmc_config_send(&data);

	return 0;
}

fmc_config_130m_4ch_board::~fmc_config_130m_4ch_board() {

	// Close device
}
