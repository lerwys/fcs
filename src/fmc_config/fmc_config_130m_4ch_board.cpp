//============================================================================
// Author      : Lucas Russo
// Version     : 1.0
// Description : Software driver for PCIe-Wishbone Master IP core
//============================================================================
#include "fmc_config_130m_4ch_board.h"
#include <math.h>

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

  // Switching DIVCLK
  data.wb_addr = DSP_BPM_SWAP | BPM_SWAP_REG_CTRL;
  data.data_send[0] = 4288 << 8;
  _commLink->fmc_config_send(&data);

  // Switching mode
  data.wb_addr = DSP_BPM_SWAP | BPM_SWAP_REG_CTRL;
  data.data_send[0] |= data.data_send[0] | 0x1 << 1 | 0x1 << 3; // Direct mode for both sets of channels
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
    _commLink->fmc_config_send(&data);
    
    data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DDS_PINC_CH1;
    data.data_send[0] = POS_CALC_DDS_PINC_CH1_VAL_W(pinc);  // phase increment ch1
    _commLink->fmc_config_send(&data);
    
    data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DDS_PINC_CH2;
    data.data_send[0] = POS_CALC_DDS_PINC_CH2_VAL_W(pinc);  // phase increment ch2
    _commLink->fmc_config_send(&data);
    
    data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DDS_PINC_CH3;
    data.data_send[0] = POS_CALC_DDS_PINC_CH3_VAL_W(pinc);  // phase increment ch3
    _commLink->fmc_config_send(&data);
    
    data.wb_addr = DSP_CTRL_REGS | POS_CALC_REG_DDS_CFG;
    // toggle valid signal for all four DDS's
    data.data_send[0] = (0x1) | (0x1 << 8) | (0x1 << 16) | (0x1 << 24);
    _commLink->fmc_config_send(&data);
  }
  
  return 0;
}

#define MAX_TRIES 10

int fmc_config_130m_4ch_board::set_data_acquire(uint32_t num_samples, uint32_t offset, int acq_chan)
{
  wb_data data;
  uint32_t acq_core_ctl_reg;
  int tries = 0;

  data.data_send.resize(10);
  data.extra.resize(2);
  
  // Num shots	
  data.wb_addr = WB_ACQ_BASE_ADDR | ACQ_CORE_REG_SHOTS;
  data.data_send[0] = ACQ_CORE_SHOTS_NB_W(1);
  _commLink->fmc_config_send(&data);

  // Pre-trigger samples
  data.wb_addr = WB_ACQ_BASE_ADDR | ACQ_CORE_REG_PRE_SAMPLES;
  data.data_send[0] = num_samples;
  _commLink->fmc_config_send(&data);

  // Pos-trigger samples
  data.wb_addr = WB_ACQ_BASE_ADDR | ACQ_CORE_REG_POST_SAMPLES;
  data.data_send[0] = 0;
  _commLink->fmc_config_send(&data);

  // DDR3 start address
  data.wb_addr = WB_ACQ_BASE_ADDR | ACQ_CORE_REG_DDR3_START_ADDR;
  data.data_send[0] = offset;
  _commLink->fmc_config_send(&data);

  // Prepare core_ctl register
  acq_core_ctl_reg = ACQ_CORE_CTL_FSM_ACQ_NOW;
  data.wb_addr = WB_ACQ_BASE_ADDR | ACQ_CORE_REG_CTL;
  data.data_send[0] = acq_core_ctl_reg;
  _commLink->fmc_config_send(&data);

  // Prepare acquisition channel control
  data.wb_addr = WB_ACQ_BASE_ADDR | ACQ_CORE_REG_ACQ_CHAN_CTL;
  data.data_send[0] = ACQ_CORE_ACQ_CHAN_CTL_WHICH_W(acq_chan);
  _commLink->fmc_config_send(&data);

  // Starting acquisition...
  acq_core_ctl_reg |= ACQ_CORE_CTL_FSM_START_ACQ;
  data.wb_addr = WB_ACQ_BASE_ADDR | ACQ_CORE_REG_CTL;
  data.data_send[0] = acq_core_ctl_reg;
  _commLink->fmc_config_send(&data);

  // Check for completion
  do {
    data.wb_addr = WB_ACQ_BASE_ADDR | ACQ_CORE_REG_STA;
    _commLink->fmc_config_send(&data);
    ++tries;
  } while (!(data.data_read[0] & ACQ_CORE_STA_DDR3_TRANS_DONE) && (tries < MAX_TRIES));

  if (tries == MAX_TRIES) {
    return -3; // exceeded number of tries
  }
  
  return 0;
}
