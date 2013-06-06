//============================================================================
// Author      : Andrzej Wojenski
// Version     : 1.0
// Description : Software driver for LM75A chip (temperature sensor)
//============================================================================
#include "lm75a.h"

#define MAX_REPEAT 10

wb_data LM75A_drv::data_;
commLink* LM75A_drv::commLink_;
string LM75A_drv::i2c_id_;

void LM75A_drv::LM75A_setCommLink(commLink* comm, string i2c_id) {

  commLink_ = comm;
  i2c_id_ = i2c_id;
  data_.data_send.resize(2);
  data_.extra.resize(1);

}

// pointer register (chip have timeout)
void LM75A_drv::LM75A_setPtrReg(uint16_t chip_addr, uint32_t reg) {

  data_.data_send.resize(1);

  data_.extra[0] = chip_addr;
  data_.extra[1] = 1;
  data_.data_send[0] = reg;

  commLink_->fmc_send(i2c_id_, &data_);

}

void LM75A_drv::LM75A_setConfig(uint16_t chip_addr, uint8_t data) {

  data_.data_send.resize(2); // ??? or 1

  data_.extra[0] = chip_addr;
  data_.data_send[0] = 0x01; // config reg
  data_.data_send[1] = data;
  commLink_->fmc_send(i2c_id_, &data_);

}

float LM75A_drv::LM75A_readTemp(uint16_t chip_addr) {

  int16_t temp_data;
  data_.data_send.resize(1);

  data_.extra[0] = chip_addr;
  data_.extra[1] = 2;
  data_.data_send[0] = 0x00; // temp reg
  commLink_->fmc_send_read(i2c_id_, &data_);

  temp_data = ((data_.data_read[0] & 0xFF) << 8) | (data_.data_read[1] & 0x80);
  temp_data = temp_data >> 7;

  // copy sign bit (is value is less then 0, two's complement data format)
  if ((temp_data & 0x100) != 0)
    temp_data |= 0xFE00;

  return temp_data * 0.5;
}

uint16_t LM75A_drv::LM75A_readID(uint16_t chip_addr) {

  data_.data_send.resize(1);

  data_.extra[0] = chip_addr;
  data_.extra[1] = 1; // only one byte read
  data_.data_send[0] = 0x07; // id reg
  commLink_->fmc_send_read(i2c_id_, &data_);

  return (data_.data_read[0] & 0xFF);

}


/*
void LM75A_drv::LM75A_drv_assert(uint32_t chip_addr, uint8_t reg, uint8_t val) {

  data_.extra[0] = chip_addr;
  data_.extra[1] = 1;
  data_.data_send[0] = reg; // starting register

  si570_read_freq(&data_);

  //cout << "Si571 assert, reg: 0x" << hex << reg <<
  //    " val: 0x" << hex << unsigned(data_.data_read[0] & 0xFF) << " =? 0x" << hex << unsigned(val) << "...";

  printf("Si571 assert, reg: 0x%02x val: 0x%02x =? 0x%02x...", reg, data_.data_read[0] & 0xFF, val);

  // compare data read data from I2C with expected value
  assert( (data_.data_read[0] & 0xFF) == val);

  printf("passed!\n");

  //cout << "passed!" << endl;

}
*/
