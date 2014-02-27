//============================================================================
// Author      : Lucas Russo
// Version     : 1.0
// Description : Software driver for PCIe-Wishbone Master IP core
//============================================================================
#include "fmc_config_130m_4ch_board.h"
#include <math.h>
#include "debug.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

fmc_config_130m_4ch_board::fmc_config_130m_4ch_board(WBMaster_unit* wb_master_unit, const delay_lines *delay_data_l,
                                        const delay_lines *delay_clk_l) {

	init(wb_master_unit, delay_data_l, delay_clk_l);
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

  printf("BPM Swap static gain set to: %d\n", 32768);

  // Switching DIVCLK
  data.wb_addr = DSP_BPM_SWAP | BPM_SWAP_REG_CTRL;
  data.data_send[0] = 4288 << 8;
  _commLink->fmc_config_send(&data);

  printf("BPM Swap DIVCLK set to: %d\n", 4288);

  // Switching mode
  data.wb_addr = DSP_BPM_SWAP | BPM_SWAP_REG_CTRL;
  data.data_send[0] |= data.data_send[0] | 0x1 << 1 | 0x1 << 3; // Direct mode for both sets of channels
  _commLink->fmc_config_send(&data);

  printf("BPM Swap Switching set to OFF!\n");

  // Set windowing mode
  data.wb_addr = DSP_BPM_SWAP | BPM_SWAP_REG_USE_WDW;
  data.data_send[0] = 0x0; // No windowing
  _commLink->fmc_config_send(&data);

  printf("BPM Swap Windowing set to OFF!\n");

  // Set windowing delay
  data.wb_addr = DSP_BPM_SWAP | BPM_SWAP_REG_WDW_DLY;
  data.data_send[0] = BPM_SWAP_WDW_DLY_W(0x0); // No delay
  _commLink->fmc_config_send(&data);

  printf("BPM Swap Windowing delay set to 0!\n");

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

  data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DDS_CFG;
  // toggle valid signal for all four DDS's
  data.data_send[0] = (0x1) | (0x1 << 8) | (0x1 << 16) | (0x1 << 24);
  _commLink->fmc_config_send(&data);

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
    data.data_send[0] |= (data.data_read[0] & ~BPM_SWAP_CTRL_MODE1_MASK & ~BPM_SWAP_CTRL_MODE2_MASK) |
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
    data.data_send[0] |= (data.data_read[0] & ~BPM_SWAP_CTRL_MODE1_MASK & ~BPM_SWAP_CTRL_MODE2_MASK) |
                          BPM_SWAP_CTRL_MODE1_W(0x1)  |
                          BPM_SWAP_CTRL_MODE2_W(0x1); // Switching mode for both sets of channels
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
    data.data_send[0] |= (data.data_read[0] & ~BPM_SWAP_CTRL_SWAP_DIV_F_MASK) |
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
    data.data_send[0] = BPM_SWAP_DLY_1_W(phase) |
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
  data.wb_addr = DSP_BPM_SWAP | BPM_SWAP_REG_USE_WDW;

  if (wdwon_out) {
    _commLink->fmc_config_read(&data);
    *wdwon_out = data.data_read[0];
  }
  else {
    data.data_send[0] = 0x1; // Use windowing
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
  data.wb_addr = DSP_BPM_SWAP | BPM_SWAP_REG_USE_WDW;

  if (wdwon_out) {
    _commLink->fmc_config_read(&data);
    *wdwon_out = data.data_read[0];
  }
  else {
    data.data_send[0] = 0x0; // Use windowing
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
  data.wb_addr = DSP_BPM_SWAP | BPM_SWAP_REG_WDW_DLY;

  if (wdw_dly_out) {
    _commLink->fmc_config_read(&data);
    *wdw_dly_out = BPM_SWAP_WDW_DLY_R(data.data_read[0]);
  }
  else {
    data.data_send[0] = BPM_SWAP_WDW_DLY_W(wdw_dly);
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

    data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DDS_CFG;
    // toggle valid signal for all four DDS's
    data.data_send[0] = (0x1) | (0x1 << 8) | (0x1 << 16) | (0x1 << 24);
    _commLink->fmc_config_send(&data);

    data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DDS_PINC_CH0;
    _commLink->fmc_config_read(&data);
    printf ("DDS PINC set to: %d\n", data.data_read[0]);
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
