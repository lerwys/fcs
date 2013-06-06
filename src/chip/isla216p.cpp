//============================================================================
// Author      : Andrzej Wojenski
// Version     : 1.0
// Description : Software driver for ISLA216P chip (ADC)
//============================================================================
#include "isla216p.h"

#define MAX_REPEAT 10

wb_data ISLA216P_drv::data_;
commLink* ISLA216P_drv::commLink_;
string ISLA216P_drv::spi_id_;
string ISLA216P_drv::gpio_id_;

void ISLA216P_drv::ISLA216P_setCommLink(commLink* comm, string spi_id, string gpio_id) {

  commLink_ = comm;
  spi_id_ = spi_id;
  gpio_id_ = gpio_id;
  data_.data_send.resize(2);
  data_.extra.resize(2);
}

wb_data ISLA216P_drv::ISLA216P_spi_write(uint32_t chip_select, uint8_t reg, uint8_t val) {

  // chip address
  data_.extra[0] = chip_select;
  // number of bits to read/write
  // 3 x 8 bits
  data_.extra[1] = 0x18;

  // SPI core - 32bit tx register
  data_.data_send[0] = (0x00 << 16) | (reg << 8) | val; // data to write to register
  //data_.data_send[1] = reg; // register address (instruction header)
  //data_.data_send[2] = 0x00;// instruction header (write)

  //cout << "SPI writing: " << hex << data_.data_send[0] << endl;

  commLink_->fmc_send(spi_id_, &data_);

  return data_;

}

wb_data ISLA216P_drv::ISLA216P_spi_read(uint32_t chip_select, uint8_t reg) {

  // chip address
  data_.data_read.clear();
  data_.extra[0] = chip_select;
  // number of bits to read/write
  // 3 x 8 bits
  data_.extra[1] = 0x18;

  data_.data_send[0] = (0x80 << 16) | (reg << 8) | 0x00; // no data
  //data_.data_send[1] = reg; // register address (instruction header)
  //data_.data_send[2] = 0x80;// instruction header (read)

  commLink_->fmc_send_read(spi_id_, &data_);

  return data_;

}

int ISLA216P_drv::ISLA216P_AutoCalibration(uint32_t ctrl_reg) {

  ISLA216P_reset(ctrl_reg, 0x01); // turn off reset
  sleep(1); // according to datasheet, maximum setup time for 250MHz clock is 200ms, maximum 550ms
  ISLA216P_reset(ctrl_reg, 0x00); // turn on reset
  sleep(1); // according to datasheet, maximum setup time for 250MHz clock is 200ms, maximum 550ms
  ISLA216P_reset(ctrl_reg, 0x01); // turn off reset
  sleep(1); // according to datasheet, maximum setup time for 250MHz clock is 200ms, maximum 550ms

  cout << "ISLA216P ADC chip auto-calibration done! Check status" << endl;

  return 0;
}

int ISLA216P_drv::ISLA216P_checkCalibration(uint32_t chip_select) {

  int repeat = 0;

  while (1) {

    // read reg
    data_ = ISLA216P_spi_read(chip_select, 0xB6);
    cout << "data isla " << hex << data_.data_read[0] << endl;

    if (data_.data_read[0] & 0x01) // cal_status must be 1 - calibration done
      break;

    sleep(1);
    repeat++;

    if (repeat < MAX_REPEAT) {
      cout << "EEROR: ISLA216P ADC chip calibration status: not done!" << endl;
    }
  }

  cout << "ISLA216P ADC chip calibration status: done!" << endl;

  return 0;

}

// as described in wb_regs
int ISLA216P_drv::ISLA216P_reset(uint32_t ctrl_reg, uint8_t mode) {

  // get register value
  uint32_t reg_val;

  data_.wb_addr = ctrl_reg;

  commLink_->fmc_read(gpio_id_, &data_);
  reg_val = data_.data_read[0];

  data_.data_send[0] = (reg_val & 0xFFFFFFFD) | ( (mode & 0x1) << 1);

  cout << "reset isla reg val: " << hex << data_.data_send[0] << endl;

  commLink_->fmc_send(gpio_id_, &data_);

  return 0;
}

int ISLA216P_drv::ISLA216P_resetSPI(uint32_t chip_select, uint32_t ctrl_reg) {

  uint8_t val;

  data_ = ISLA216P_spi_read(chip_select, 0x00);

  val = data_.data_read[0] | 0x20;
  ISLA216P_spi_write(chip_select, 0x00, val);
  sleep(1);

  val = data_.data_read[0] & 0xFFFFFFDF;
  ISLA216P_spi_write(chip_select, 0x00, val);
  sleep(1);

  return 0;
}

int ISLA216P_drv::ISLA216P_sleep(uint32_t ctrl_reg, uint8_t mode) {

  // get register value
  uint32_t reg_val;

  data_.wb_addr = ctrl_reg;

  commLink_->fmc_read(gpio_id_, &data_);
  reg_val = data_.data_read[0];

  data_.data_send[0] = (reg_val & 0xFFFFFFF3) | (mode << 2);

  cout << "sleep isla reg val: " << hex << data_.data_send[0] << endl;

  commLink_->fmc_send(gpio_id_, &data_);

  //usleep(100000); // TODO check timing
  sleep(1);

  return 0;
}

int ISLA216P_drv::ISLA216P_sync(uint32_t ctrl_reg) {

  // get register value
  uint32_t reg_val;

  data_.wb_addr = ctrl_reg;

  commLink_->fmc_read(gpio_id_, &data_);
  reg_val = data_.data_read[0];

  // set divclkrst reset to 0
  data_.data_send[0] = reg_val & 0xFFFFFFFE;
  commLink_->fmc_send(gpio_id_, &data_);
  // wait
  sleep(1);

  // set divclkrst reset to 1
  data_.data_send[0] = reg_val | 0x01;
  commLink_->fmc_send(gpio_id_, &data_);
  // wait
  sleep(2);

  cout << "ISLA216P ADC chips phase sync done!" << endl;

  return 0;

}

int ISLA216P_drv::ISLA216P_config(uint32_t chip_select) {

  uint16_t data_temp;

  // activate SDO
  ISLA216P_spi_write(chip_select, 0x00, 0x99);

  // registers as offset, gain etc should be automatically set
  // after auto-calibration

  // modes_adc0
  // power down mode - pin control
  ISLA216P_spi_write(chip_select, 0x25, 0x00);
  // modes_adc1

  // phase_slip - no phase slip (no tests)
  // skew_diff - not supported

  // clock_divide - divide by 2 - not used
  //ISLA216P_spi_write(chip_select, 0x72, 0x02);

  // clock_divide - divide by 1
  ISLA216P_spi_write(chip_select, 0x72, 0x01);

  // output mode A
  // default LVDS 3mA, two's complement
  //ISLA216P_spi_write(chip_select, 0x73, 0x00);
  ISLA216P_spi_write(chip_select, 0x73, 0x20);

  // output mode B - default is fast mode (ADC clock frequency)
  data_ = ISLA216P_spi_read(chip_select, 0x74);
  //data_temp = 0x40 | data_.data_read[0]; // low speed
  data_temp = 0xBF & data_.data_read[0]; // high speed
  ISLA216P_spi_write(chip_select, 0x74, data_temp);

  // offset/gain adjust enable - not implemented in chip?

  // check configuration
  ISLA216P_assert(chip_select, 0x25, 0x00);
  ISLA216P_assert(chip_select, 0x72, 0x01);
  //ISLA216P_assert(chip_select, 0x73, 0x00);
  ISLA216P_assert(chip_select, 0x73, 0x20);
  ISLA216P_assert(chip_select, 0x74, data_temp);

  return 0;
}

int ISLA216P_drv::ISLA216P_train(uint32_t chip_select) {

  cout << "ISLA216P ADC chip - function not implemented" << endl;

  return 0;

}

// mode - output test mode
// test_pattern - depending on vector size:
// 1 = user pattern 1 only
// 2 = cycle pattern 1,3
// 3 = cycle pattern 1,3,5
// 4 = cycle pattern 1,3,5,7
int ISLA216P_drv::ISLA216P_setTestPattern(uint32_t chip_select, uint8_t mode, vector<uint16_t> test_pattern) {

  uint32_t reg_addr = 0xC1; // user_patt1_lsb

  if (mode != 0) {
    for (unsigned int i = 0; i < test_pattern.size() && i < 4; i++) {
      ISLA216P_spi_write(chip_select, reg_addr+i*4, (test_pattern[i] & 0xFF)); // user_pattX_lsb
      ISLA216P_spi_write(chip_select, reg_addr+i*4+1, ( (test_pattern[i] >> 8) & 0xFF) ); // user_pattX_msb
    }
  }

  ISLA216P_spi_write(chip_select, 0xC0, mode);

  return 0;
}

int ISLA216P_drv::ISLA216P_TestPatternOff(uint32_t chip_select) {

  ISLA216P_spi_write(chip_select, 0xC0, 0x00);

  return 0;

}

uint32_t ISLA216P_drv::ISLA216P_getTemp(uint32_t chip_select) {

  uint16_t temp;

  // as in ISLA216P ADC chip datasheet page 28
  ISLA216P_spi_write(chip_select, 0x4D, 0xCA);
  usleep(500);
  ISLA216P_spi_write(chip_select, 0x4D, 0x20);

  data_ = ISLA216P_spi_read(chip_select, 0x4B); // msb
  temp = (data_.data_read[0] & 0xFF) << 8;

  data_ = ISLA216P_spi_read(chip_select, 0x4C); // lsb
  temp = temp | (data_.data_read[0] & 0xFF);

  // set back to IPTAT mode
  ISLA216P_spi_write(chip_select, 0x4D, 0x20);

  return temp;
}

uint32_t ISLA216P_drv::ISLA216P_getChipID(uint32_t chip_select) {

  data_ = ISLA216P_spi_read(chip_select, 0x08);

  return data_.data_read[0];

}

uint32_t ISLA216P_drv::ISLA216P_getChipVersion(uint32_t chip_select) {

  data_ = ISLA216P_spi_read(chip_select, 0x09);

  return data_.data_read[0];
}

void ISLA216P_drv::ISLA216P_assert(uint32_t chip_select, uint8_t reg, uint8_t val) {

  data_ = ISLA216P_spi_read(chip_select, reg);

  //cout << "ISLA216P assert, reg: 0x" << hex << reg <<
  //    " val: 0x" << hex << unsigned(data_.data_read[0] & 0xFF) << " =? 0x" << hex << unsigned(val) << "...";

  printf("ISLA216P assert, reg: 0x%02x val: 0x%02x =? 0x%02x...", reg, data_.data_read[0] & 0xFF, val);

  assert( (data_.data_read[0] & 0xFF) == val);

  printf("passed!\n");

}


