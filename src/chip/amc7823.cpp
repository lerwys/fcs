//============================================================================
// Author      : Andrzej Wojenski
// Version     : 1.0
// Description : Software driver for AMC7823 chip (temperature monitor)
//============================================================================
#include "amc7823.h"

#define MAX_REPEAT 10

// Register map
// PAGE 1
#define DAC_CONF 0x09
#define AMC_CONF 0x0A
#define ADC_CTRL 0x0B
#define PWR_DWN_CTRL 0x0D
// PAGE 0
#define ADC0_DATA 0x00
#define ADC1_DATA 0x01
#define ADC2_DATA 0x02
#define ADC3_DATA 0x03
#define ADC4_DATA 0x04
#define ADC5_DATA 0x05
#define ADC6_DATA 0x06
#define ADC7_DATA 0x07
#define ADC8_DATA 0x08 // on-chip temp

wb_data AMC7823_drv::data_;
commLink* AMC7823_drv::commLink_;
string AMC7823_drv::spi_id_;
string AMC7823_drv::gpio_id_;

void AMC7823_drv::AMC7823_setCommLink(commLink* comm, string spi_id, string gpio_id) {

  commLink_ = comm;
  spi_id_ = spi_id;
  gpio_id_ = gpio_id;
  data_.extra.resize(2);
  data_.data_send.resize(1);

}

void AMC7823_drv::AMC7823_spi_write(uint32_t chip_select, uint8_t page, uint8_t reg, uint16_t val) {

  // chip address
  data_.extra[0] = chip_select;
  // number of bits to read/write
  // 2 x 16 bits
  data_.extra[1] = 0x20;

  // SPI core
  data_.data_send[0] = 0;
  data_.data_send[0] |= ( (page & 0x03) << 12) | ( (reg & 0x1F) << 6);
  data_.data_send[0] = data_.data_send[0] << 16; // command word;
  data_.data_send[0] |= val; // add data (second word)

  commLink_->fmc_send(spi_id_, &data_);

  return;
}

uint16_t AMC7823_drv::AMC7823_spi_read(uint32_t chip_select, uint8_t page, uint8_t reg) {

  // chip address
  data_.extra[0] = chip_select;
  // number of bits to read/write
  // 2 x 16 bits
  data_.extra[1] = 0x20;

  // SPI core
  data_.data_send[0] = 0;
  data_.data_send[0] |= (0x1 << 15) | ( (page & 0x03) << 12) | ( (reg & 0x1F) << 6);
  data_.data_send[0] = data_.data_send[0] << 16; // command word (read)

  commLink_->fmc_send_read(spi_id_, &data_);
  //cout << "data_amc spi: " << hex << data_.data_read[0] << endl;
  return data_.data_read[0];

}

int AMC7823_drv::AMC7823_checkReset(uint32_t chip_select) {

  uint16_t data;

  data = AMC7823_spi_read(chip_select, 0x1, AMC_CONF); // default 0x4000
  //cout << "AMC data: 0x" << hex << data << endl;
  if (data & 0x4000) {
    cout << "AMC7823 reset done" << endl;
    return 1;
  } else {
    cout << "AMC7823 still in reset mode" << endl;
    return 0;
  }

}

void AMC7823_drv::AMC7823_config(uint32_t chip_select) {

  uint16_t data;

  data = AMC7823_spi_read(chip_select, 0x1, AMC_CONF);
  data &= 0x6000;

  // Internal refernece 1.25V, ADC input range 0 to 2.5V
  // ADC internal trigger mode
  AMC7823_spi_write(chip_select, 0x1, AMC_CONF, data);
  AMC7823_assert(chip_select, 0x1, AMC_CONF, data);

  // ADC direct mode (internal trigger)
  // read data from ADC0 to ADC8
  // writing to this register starts conversion (wait for DAV pin)
  data = 0x0080;
  AMC7823_spi_write(chip_select, 0x1, ADC_CTRL, data);
  AMC7823_assert(chip_select, 0x1, ADC_CTRL, data);

}

void AMC7823_drv::AMC7823_powerUp(uint32_t chip_select) {

  uint16_t data;

  // Power management
  // ADC - on
  // other - off
  data = 0x8000;
  AMC7823_spi_write(chip_select, 0x1, PWR_DWN_CTRL, data);
  AMC7823_assert(chip_select, 0x1, PWR_DWN_CTRL, data);

}

// ADC0, ADC1, ADC2, ADC3, on-chip temp
vector<uint16_t> AMC7823_drv::AMC7823_getADCData(uint32_t ctrl_reg, uint32_t chip_select) {

  uint16_t data;
  vector<uint16_t> adc_data;
  int repeat = 0;

  // trigger
  data = 0x0080;
  AMC7823_spi_write(chip_select, 0x1, ADC_CTRL, data);

  // wait for data (DAV pin) - conversion done
  while (1) {

    data_.wb_addr = ctrl_reg;
    commLink_->fmc_read(gpio_id_, &data_);

    if ((data_.data_read[0] & 0x1) == 0) // DAV = 0 - conversion done
      break;

    repeat++;

    if (repeat > MAX_REPEAT) {
      cout << "AMC7823 ADC conversion error!" << endl;
      break;
    }

    usleep(100000);
  }

  // read regs
  //cout << "Reading data==========================================" << endl;

  data = AMC7823_spi_read(chip_select, 0x0, ADC0_DATA);
  adc_data.push_back(data);

  data = AMC7823_spi_read(chip_select, 0x0, ADC1_DATA);
  adc_data.push_back(data);

  data = AMC7823_spi_read(chip_select, 0x0, ADC2_DATA);
  adc_data.push_back(data);

  data = AMC7823_spi_read(chip_select, 0x0, ADC3_DATA);
  adc_data.push_back(data);

  data = AMC7823_spi_read(chip_select, 0x0, ADC8_DATA);
  adc_data.push_back(data);

  //cout << "Reading data==========================================" << endl;

  //for (int i = 0; i < adc_data.size(); i++)
  //  cout << "dane " << hex << adc_data[i] << endl;

  return adc_data;
}

float AMC7823_drv::AMC7823_tempConvert(uint16_t temp) {

  float f_temp;
  int temp_int = temp - 0x8000; // binary offset
  // For V_ref = 1.25V
  // 80bd = 50bcd 32957
  // binary code?
  f_temp = ((float)temp_int * 0.61) * 2.6 - 273; // 0.00061V or 0.61mV?

  return f_temp;

}

void AMC7823_drv::AMC7823_assert(uint32_t chip_select, uint8_t page, uint8_t reg, uint16_t val) {

  uint16_t data;

  data = AMC7823_spi_read(chip_select, page, reg);

  //cout << "AMC7823 assert, reg: 0x" << hex << reg <<
  //    " val: 0x" << hex << unsigned(data) << " =? 0x" << hex << unsigned(val) << "...";

  printf("AMC7823 assert, page: 0x%02x reg: 0x%02x val: 0x%04x =? 0x%04x...", page, reg, data, val);

  // compare data read from SPI with expected value
  assert(data == val);

  printf("passed!\n");

}
