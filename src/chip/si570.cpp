//============================================================================
// Author      : Andrzej Wojenski
// Version     : 1.0
// Description : Software driver for Si570/ Si571 chip (clock generator)
//============================================================================
#include "si570.h"

#define SI570_ADDR 0x55
#define MAX_REPEAT 10

// Define PRIx64 print macros
//#define __STDC_FORMAT_MACROS

wb_data Si570_drv::data_;
commLink* Si570_drv::commLink_;
string Si570_drv::i2c_id_;
string Si570_drv::gpio_id_;

void Si570_drv::si570_setCommLink(commLink* comm, string i2c_id, string gpio_id) {

  commLink_ = comm;
  i2c_id_ = i2c_id;
  gpio_id_ = gpio_id;
  data_.data_send.resize(2);
  data_.extra.resize(1);

}

// get info about startup setting so user can calculate needed values
int Si570_drv::si570_read_freq(wb_data* data) {

  uint32_t HS_DIV, N1;
  uint32_t RFFREQ_INTEGER; // 10 bits
  uint32_t RFFREQ_INTEGER_FLOAT;
  //uint64_t RFFREQ = 0; // 38 bits
  //uint64_t fdco = 0;
  int err = 0, std_read = 0;
  //int data_size = data->data_send.size();

  data->data_send.resize(1);

  //cout << showbase << internal << setfill('0') << setw(8);

//  if (data->extra.size() < 2) {
//    data->extra.resize(2);
//    data->extra[0] = SI570_ADDR;
//    data->extra[1] = 6; // number of registers to read
//    data->data_send[0] = 0x07; // starting register
//    std_read = 1;
//  }

  //if (data->data_send.size() == 0)
  //  data->data_send[0] = 0x07; // starting register

  err = commLink_->fmc_send_read(i2c_id_, data);

  //data->data_send.resize(data_size);

  if (err != 0)
    return err;

  // parsing data
  for (unsigned int i = 0; i < data->extra[1]; i++)
    cout << "Si570: data read [" << i << "] : 0x" << hex << (data->data_read[i] & 0xFF) << endl;

  if (std_read == 1) {
    //HS_DIV = data->data_read[0] >> 5;
    HS_DIV = (data->data_read[0] & 0xE0) >> 5;
    //N1 = ( (data->data_read[0] & 0x1F) << 2) | (data->data_read[1] >> 6);
    N1 = ( (data->data_read[0] & 0x1F) << 2) | ((data->data_read[1] & 0xC0) >> 6);

    RFFREQ_INTEGER = ((data->data_read[1] & 0x3F) << 4) | ((data->data_read[2] & 0xF0) >> 4);

    RFFREQ_INTEGER_FLOAT = (data->data_read[2] & 0x0F) << 3*8;
    RFFREQ_INTEGER_FLOAT |= data->data_read[3] << 2*8;
    RFFREQ_INTEGER_FLOAT |= data->data_read[4] << 1*8;
    RFFREQ_INTEGER_FLOAT |= data->data_read[5];

    //RFFREQ = data->data_read[1] & 0x3F;
    //
    //RFFREQ = (RFFREQ << 8) + data->data_read[2];
    //RFFREQ = (RFFREQ << 8) + data->data_read[3];
    //RFFREQ = (RFFREQ << 8) + data->data_read[4];
    //RFFREQ = (RFFREQ << 8) + data->data_read[5];

    cout << "Si570: Read parameters: " << endl <<
        "HS_DIV: 0x" << hex << HS_DIV << endl <<
        "N1: 0x" << hex << N1 << endl <<
        "RFFREQ_INTEGER: 0x" << hex << RFFREQ_INTEGER << endl <<
        "RFFREQ_INTEGER_FLOAT: 0x" << hex << RFFREQ_INTEGER_FLOAT << endl;

    //cout << "Si570: Read parameters: " << endl <<
    //    "HS_DIV: 0x" << hex << HS_DIV << endl <<
    //    "N1: 0x" << hex << N1 << endl <<
    //    "RFFREQ (LSB): 0x" << hex << (RFFREQ & 0x00000000FFFFFFFF) << endl <<
    //    "RFFREQ (MSB): 0x" << hex << ((RFFREQ & 0xFFFFFFFF00000000) >> 32) << endl;
  }

  return err;
}

int Si570_drv::si570_set_freq(wb_data* data) {

  int i;
  int err = 0, repeat = 0;

/*  if (data->extra.size() < 1) {
    data_.extra[0] = SI570_ADDR;
    data->extra[0] = SI570_ADDR;
  }
  else*/

  data_.extra[0] = data->extra[0]; // chip addr

  // freeze DCO - reg 137 bit 4
  data_.data_send[0] = 0x89;
  data_.data_send[1] = 0x10;

  err = commLink_->fmc_send(i2c_id_, &data_);
  if (err != 0)
    return err;

  // wait 10 ms
  usleep(10000);

  // write data (for 20ppm and 50ppm devices) - regs 7 - 12
  for (i = 0; i < 6; i++) {
    data_.data_send[0] = 0x07 + i;
    data_.data_send[1] = data->data_send[i];
    err =  commLink_->fmc_send(i2c_id_, &data_);
    if (err != 0)
      return err;
  }

  // unfreeze DCO + append new freq - bit 6 reg 135
  data_.data_send[0] = 0x89;
  data_.data_send[1] = 0x00; // unfreeze DCO
  err = commLink_->fmc_send(i2c_id_, &data_);
  if (err != 0)
    return err;

  data_.data_send[0] = 0x87;
  data_.data_send[1] = 0x40; // apply new freq (NewFreq bit)
  err = commLink_->fmc_send(i2c_id_, &data_);
  if (err != 0)
    return err;

  // wait 10 ms
  usleep(10000);

  // check if newfreq bit is cleared (new frequency applied)
  while(1) { // bit automatically cleared

    data_.data_send[0] = 0x87; // reg 135
    data_.extra[1] = 1;
    err = commLink_->fmc_send_read(i2c_id_, &data_); //i2c_int->int_send_read_data(&data_);
//cout << "data: " << hex << data_.data_read[0] << endl;
    if ( ( (data_.data_read[0] & 0xBF) >> 6 ) == 0)
      break;
    sleep(1);

    repeat++;

    if (repeat > MAX_REPEAT) {
      cout << "Si570: Error: Frequency not set." << endl;
      err = 1;
      return err;
    }

  }

  cout << "Si570: Setup new frequency completed" << endl;

  //dev_info(fmc->hwdev, "Si570: Setup new frequency completed\n");

  return err;

}

int Si570_drv::si570_outputEnable(uint32_t addr) {

  data_.wb_addr = addr;
  commLink_->fmc_read(gpio_id_, &data_);
  data_.data_send[0] = data_.data_read[0] | 0x1;
  commLink_->fmc_send(gpio_id_, &data_);

  usleep(30000); // 30ms

  cout << "Si571 output enabled" << endl;

  return 0;
}

int Si570_drv::si570_outputDisable(uint32_t addr) {

  data_.wb_addr = addr;
  commLink_->fmc_read(gpio_id_, &data_);
  data_.data_send[0] = data_.data_read[0] & 0xFFFFFFFE;
  commLink_->fmc_send(gpio_id_, &data_);

  usleep(30000); // 30ms

  cout << "Si571 output disabled" << endl;

  return 0;
}

void Si570_drv::si570_assert(uint32_t chip_addr, uint8_t reg, uint8_t val) {

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
