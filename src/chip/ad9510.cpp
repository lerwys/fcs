//============================================================================
// Author      : Andrzej Wojenski
// Version     : 1.0
// Description : Software driver for AD9510 chip (clock distribution)
//============================================================================
#include "ad9510.h"

#define MAX_REPEAT 10

wb_data AD9510_drv::data_;
commLink* AD9510_drv::commLink_;
string AD9510_drv::spi_id_;

void AD9510_drv::AD9510_setCommLink(commLink* comm, string spi_id) {

  commLink_ = comm;
  spi_id_ = spi_id;
  data_.data_send.resize(1);
  data_.extra.resize(2);

}

wb_data AD9510_drv::AD9510_spi_write(uint32_t chip_select, uint8_t reg, uint8_t val) {

  //data->data_read.clear();
  //data_.data_read.clear();

  // chip address
  data_.extra[0] = chip_select;
  // number of bits to read/write
  // 3 x 8 bits
  data_.extra[1] = 0x18;

  // SPI core - starts sending data from data_send[2] - first transmission byte
  data_.data_send[0] = (0x00 << 16) | (reg << 8) | val; // data to write to register
  //data_.data_send[1] = reg; // register address (instruction header)
  //data_.data_send[2] = 0x00;// instruction header (write)

  //return spi_int->int_send_data(&data_);
  commLink_->fmc_send(spi_id_,&data_);

  return data_;

}

wb_data AD9510_drv::AD9510_spi_read(uint32_t chip_select, uint8_t reg) {

  data_.data_read.clear();

  // chip address
  data_.extra[0] = chip_select;
  // number of bits to read/write
  // 3 x 8 bits
  data_.extra[1] = 0x18;

  data_.data_send[0] = (0x80 << 16) | (reg << 8) | 0x00; // no data
  //data_.data_send[1] = reg; // register address (instruction header)
  //data_.data_send[2] = 0x80;// instruction header (read)

  //cout << "data1234 " << hex << data_.data_send[0] << endl;

  commLink_->fmc_send_read(spi_id_,&data_);

  return data_;

}

// provide chip select and reg address
int AD9510_drv::AD9510_reg_update(uint32_t chip_select) {

  wb_data ad9510_data;
  int repeat = 0;

  AD9510_spi_write(chip_select, 0x5A, 0x01);

  // check if updated
  while (1) {
    ad9510_data = AD9510_spi_read(chip_select, 0x5A);

    if ( (ad9510_data.data_read[0] & 0x01) == 0)
      break;

    repeat++;

    if (repeat > MAX_REPEAT) {
      cout << "AD9510 update register error (MAX_REPEAT)" << endl;
      return 1;
    }
  }

  cout << "AD9510 reg updated" << endl;

  return 0;
}

int AD9510_drv::AD9510_config_si570(uint32_t chip_select) {

  //wb_data ad9510_data;
  wb_data data;

  // reset on startup done by function pin

  // reset registers (don't turn off Long Instruction bit)
  AD9510_spi_write(chip_select, 0x00, 0x30);
  AD9510_reg_update(chip_select);
  // wait
  usleep(10000);
  //sleep(1);
  // turn off reset
  AD9510_spi_write(chip_select, 0x00, 0x10);
  AD9510_reg_update(chip_select);
  // wait
  usleep(10000);
  //sleep(1);

  // testing end
  // PLL power down (PLL is not used) - default
  // output OUT0 - OUT3 - power on
  // voltage output 810mV

  AD9510_spi_write(chip_select, 0x3C, 0x08);
  AD9510_spi_write(chip_select, 0x3D, 0x08);
  AD9510_spi_write(chip_select, 0x3E, 0x08);
  AD9510_spi_write(chip_select, 0x3F, 0x08);

  // output OUT4 - OUT7 - power down
  AD9510_spi_write(chip_select, 0x40, 0x03);
  AD9510_spi_write(chip_select, 0x41, 0x03);
  AD9510_spi_write(chip_select, 0x42, 0x03);
  AD9510_spi_write(chip_select, 0x43, 0x03);

  // Clock selection (distribution mode)
  // CLK1 - power down
  // CLK2 - power on
  // Clock select = CLK2
  // Prescaler Clock -  Power-Down
  // REFIN - Power-Down
  AD9510_spi_write(chip_select, 0x45, 0x1A);

  // Clock dividers OUT0 - OUT3
  // divide = off (bypassed, ratio 1)
  // duty cycle 50%
  // lo-hi 0x00
  // phase offset = 0
  // start high
  // sync

  AD9510_spi_write(chip_select, 0x48, 0x00); // divide by 2 (off)
  AD9510_spi_write(chip_select, 0x4A, 0x00); // divide by 2 (off)
  AD9510_spi_write(chip_select, 0x4C, 0x00); // divide by 2 (off)
  AD9510_spi_write(chip_select, 0x4E, 0x00); // divide by 2 (off)

  AD9510_spi_write(chip_select, 0x49, 0x90); // phase offset = 0 , divider off
  AD9510_spi_write(chip_select, 0x4B, 0x90); // phase
  AD9510_spi_write(chip_select, 0x4D, 0x90); // phase
  AD9510_spi_write(chip_select, 0x4F, 0x90); // phase

  // Clock dividers OUT0 - OUT3 - not used config
  // divide = 2
  // duty cycle 50%
  // lo-hi 0x00
  // phase = 0
  // start high
  // sync
  /*
    AD9510_spi_write(chip_select, 0x48, 0x00); // divide
    AD9510_spi_write(chip_select, 0x4A, 0x00); // divide
    AD9510_spi_write(chip_select, 0x4C, 0x00); // divide
    AD9510_spi_write(chip_select, 0x4E, 0x00); // divide
  //
    AD9510_spi_write(chip_select, 0x49, 0x10); // phase
    AD9510_spi_write(chip_select, 0x4B, 0x10); // phase
    AD9510_spi_write(chip_select, 0x4D, 0x10); // phase
    AD9510_spi_write(chip_select, 0x4F, 0x10); // phase
  */
  // Function pin is SYNCB
  AD9510_spi_write(chip_select, 0x58, 0x20);

  AD9510_reg_update(chip_select);

  usleep(10000);

  // software sync
  AD9510_spi_write(chip_select, 0x58, 0x24);
  AD9510_reg_update(chip_select);

  usleep(10000);

  AD9510_spi_write(chip_select, 0x58, 0x20);
  AD9510_reg_update(chip_select);

  // Check configuration

  AD9510_assert(chip_select, 0x3C, 0x08);
  AD9510_assert(chip_select, 0x3D, 0x08);
  AD9510_assert(chip_select, 0x3E, 0x08);
  AD9510_assert(chip_select, 0x3F, 0x08);

  AD9510_assert(chip_select, 0x40, 0x03);
  AD9510_assert(chip_select, 0x41, 0x03);
  AD9510_assert(chip_select, 0x42, 0x03);
  AD9510_assert(chip_select, 0x43, 0x03);

  AD9510_assert(chip_select, 0x45, 0x1A);

  AD9510_assert(chip_select, 0x48, 0x00);
  AD9510_assert(chip_select, 0x4A, 0x00);
  AD9510_assert(chip_select, 0x4C, 0x00);
  AD9510_assert(chip_select, 0x4E, 0x00);

  AD9510_assert(chip_select, 0x49, 0x90);
  AD9510_assert(chip_select, 0x4B, 0x90);
  AD9510_assert(chip_select, 0x4D, 0x90);
  AD9510_assert(chip_select, 0x4F, 0x90);

  return 0;
}

int AD9510_drv::AD9510_config_si570_fmc_adc_130m_4ch(uint32_t chip_select) {

  wb_data data;

  // reset on startup done by function pin

  // reset registers (don't turn off Long Instruction bit)
  AD9510_spi_write(chip_select, 0x00, 0x30);
  AD9510_reg_update(chip_select);
  // wait
  usleep(10000);
  //sleep(1);
  // turn off reset
  AD9510_spi_write(chip_select, 0x00, 0x10);
  AD9510_reg_update(chip_select);
  // wait
  usleep(10000);
  //sleep(1);

  // testing end
  // PLL power down (PLL is not used) - default
  // output OUT0 - OUT3 - power on
  // voltage output 810mV
  AD9510_spi_write(chip_select, 0x3C, 0x08);
  AD9510_spi_write(chip_select, 0x3D, 0x08);
  AD9510_spi_write(chip_select, 0x3E, 0x08);
  AD9510_spi_write(chip_select, 0x3F, 0x08);

  // OUT4 power up
  // LVDS, 3.5mA, 100ohm termination, power on
  //AD9510_spi_write(chip_select, 0x40, 0x03);
  AD9510_spi_write(chip_select, 0x40, 0x02);
  // output OUT5 - OUT6 - power down
  AD9510_spi_write(chip_select, 0x41, 0x03);
  AD9510_spi_write(chip_select, 0x42, 0x03);

  // output OUT7 - power on (clock copy, LVDS)
  // LVDS, 3.5mA, 100ohm termination, power on
  AD9510_spi_write(chip_select, 0x43, 0x02);

   // Clock selection (distribution mode)
  // CLK1 - power on
  // CLK2 - power down
  // Clock select = CLK1
  // Prescaler Clock -  Power-Down
  // REFIN - Power-Down
  //AD9510_spi_write(chip_select, 0x45, 0x1D);

  // Clock selection (distribution mode)
  // CLK1 - power down
  // CLK2 - power on
  // Clock select = CLK2
  // Prescaler Clock -  Power-Down
  // REFIN - Power-Down
  AD9510_spi_write(chip_select, 0x45, 0x1A);

  // Clock dividers OUT0 - OUT3 and OUT7
  // divide = off (bypassed, ratio 1)
  // duty cycle 50%
  // lo-hi 0x00
  // phase offset = 0
  // start high
  // sync
  AD9510_spi_write(chip_select, 0x48, 0x00); // divide by 2 (off)
  AD9510_spi_write(chip_select, 0x4A, 0x00); // divide by 2 (off)
  AD9510_spi_write(chip_select, 0x4C, 0x00); // divide by 2 (off)
  AD9510_spi_write(chip_select, 0x4E, 0x00); // divide by 2 (off)
  AD9510_spi_write(chip_select, 0x50, 0x00); // divide by 2 (off)
  AD9510_spi_write(chip_select, 0x56, 0x00); // divider

  AD9510_spi_write(chip_select, 0x49, 0x90); // phase offset = 0 , divider off
  AD9510_spi_write(chip_select, 0x4B, 0x90); // phase
  AD9510_spi_write(chip_select, 0x4D, 0x90); // phase
  AD9510_spi_write(chip_select, 0x4F, 0x90); // phase
  AD9510_spi_write(chip_select, 0x51, 0x90); // phase
  AD9510_spi_write(chip_select, 0x57, 0x90); // phase

  // Function pin is SYNCB
  AD9510_spi_write(chip_select, 0x58, 0x20);

  AD9510_reg_update(chip_select);

  usleep(10000);

  // software sync
  AD9510_spi_write(chip_select, 0x58, 0x24);
  AD9510_reg_update(chip_select);

  usleep(10000);

  AD9510_spi_write(chip_select, 0x58, 0x20);
  AD9510_reg_update(chip_select);

  // Check configuration

  AD9510_assert(chip_select, 0x3C, 0x08);
  AD9510_assert(chip_select, 0x3D, 0x08);
  AD9510_assert(chip_select, 0x3E, 0x08);
  AD9510_assert(chip_select, 0x3F, 0x08);

  AD9510_assert(chip_select, 0x40, 0x02);
  AD9510_assert(chip_select, 0x41, 0x03);
  AD9510_assert(chip_select, 0x42, 0x03);
  AD9510_assert(chip_select, 0x43, 0x02);

  AD9510_assert(chip_select, 0x45, 0x1A);
  //AD9510_assert(chip_select, 0x45, 0x1D);

  AD9510_assert(chip_select, 0x48, 0x00);
  AD9510_assert(chip_select, 0x4A, 0x00);
  AD9510_assert(chip_select, 0x4C, 0x00);
  AD9510_assert(chip_select, 0x4E, 0x00);
  AD9510_assert(chip_select, 0x50, 0x00);
  AD9510_assert(chip_select, 0x56, 0x00);

  AD9510_assert(chip_select, 0x49, 0x90);
  AD9510_assert(chip_select, 0x4B, 0x90);
  AD9510_assert(chip_select, 0x4D, 0x90);
  AD9510_assert(chip_select, 0x4F, 0x90);
  AD9510_assert(chip_select, 0x51, 0x90);
  AD9510_assert(chip_select, 0x57, 0x90);

  return 0;
}

int AD9510_drv::AD9510_config_si570_pll_fmc_adc_130m_4ch(uint32_t chip_select) {

  wb_data data;

  // reset on startup done by function pin

  // reset registers (don't turn off Long Instruction bit)
  AD9510_spi_write(chip_select, 0x00, 0x30);
  //AD9510_reg_update(chip_select);
  // wait
  usleep(10000);
  //sleep(1);
  // turn off reset
  AD9510_spi_write(chip_select, 0x00, 0x10);
  //AD9510_reg_update(chip_select);
  // wait
  usleep(10000);
  //sleep(1);

  // A counter = 0 - N divider
  AD9510_spi_write(chip_select, 0x04, 0x00);
  // B counter (MSB) = 0 - N divider
  AD9510_spi_write(chip_select, 0x05, 0x00);
  // B counter (MSB) = 64 - N divider
  //AD9510_spi_write(chip_select, 0x05, 0x40);
  // B counter (MSB) = ?? - N divider
  //AD9510_spi_write(chip_select, 0x05, 0x3F);
  // B counter (MSB) = ?? - N divider
   //AD9510_spi_write(chip_select, 0x05, 0x3F);
   // B counter (LSB) = 74 - N divider
  //AD9510_spi_write(chip_select, 0x06, 0x4A);
   // B counter (LSB) = 35 - N divider
  //AD9510_spi_write(chip_select, 0x06, 0x23);
  // B counter (LSB) = 10 - N divider
  AD9510_spi_write(chip_select, 0x06, 0x0A);
  // B counter (LSB) = 200 - N divider
  //AD9510_spi_write(chip_select, 0x06, 0xC8);
  // B counter (LSB) = 00 - N divider
  //AD9510_spi_write(chip_select, 0x06, 0x00);
  // B counter (LSB) = ?? - N divider
  //AD9510_spi_write(chip_select, 0x06, 0xFC);
  // B counter (LSB) = ?? - N divider
  //AD9510_spi_write(chip_select, 0x06, 0xFC);
  // B counter (LSB) = 5 - N divider
  //AD9510_spi_write(chip_select, 0x06, 0x05);
  // B counter (LSB) = 2 - N divider
  //AD9510_spi_write(chip_select, 0x06, 0x02);
  // B counter (LSB) = 1 - N divider
  //AD9510_spi_write(chip_select, 0x06, 0x01);

  // Mux Status Pin. PLL Mux Select = Digital Lock Detect, CP Mode = Normal operation, PFD polarity = 1 (positive)
  AD9510_spi_write(chip_select, 0x08, 0x04 | 0x03 | 0x40);
  // Mux Status Pin. N divider Output, CP Mode = Normal operation, PFD polarity = 1 (positive)
  //AD9510_spi_write(chip_select, 0x08, 0x08 | 0x03 | 0x40);
  // Mux Status Pin. N divider Output, CP Mode = Tristated operation, PFD polarity = 1 (positive)
  //AD9510_spi_write(chip_select, 0x08, 0x08 | 0x00 | 0x40);
  // Mux Status Pin. R divider Output, CP Mode = Normal operation, PFD polarity = 1 (positive)
  //AD9510_spi_write(chip_select, 0x08, 0x10 | 0x03 | 0x40);
  // Mux Status Pin. PFD Up output, CP Mode = Normal operation, PFD polarity = 1 (positive)
  //AD9510_spi_write(chip_select, 0x08,  0x20 | 0x03 | 0x40);
  // Mux Status Pin. PFD Down output, CP Mode = Normal operation, PFD polarity = 1 (positive)
  //AD9510_spi_write(chip_select, 0x08,  0x20 | 0x04 | 0x03 | 0x40);

  // Charge Pump Current. I = 0.6mA
  AD9510_spi_write(chip_select, 0x09, 0x00);
  // Charge Pump Current. I = 4.8mA
  //AD9510_spi_write(chip_select, 0x09, 0x70);

  // Pre scaler = divide by 2, mode = FD, PLL in normal operation 00
  //AD9510_spi_write(chip_select, 0x0A, 0x04 | 0x00);
  // Pre scaler = divide by 1, mode = FD, PLL in normal operation 00
  AD9510_spi_write(chip_select, 0x0A, 0x00 | 0x00);
  //AD9510_spi_write(chip_select, 0x0A, 0x08);

  // R divider = 1
  AD9510_spi_write(chip_select, 0x0B, 0x00);
  AD9510_spi_write(chip_select, 0x0C, 0x01);
  // R divider = 20
  //AD9510_spi_write(chip_select, 0x0B, 0x00); // 0
  //AD9510_spi_write(chip_select, 0x0C, 0x14); // 20
  // R divider = 16384
  //AD9510_spi_write(chip_select, 0x0B, 0x40);
  //AD9510_spi_write(chip_select, 0x0C, 0x00);
  // R divider = 1638
  //AD9510_spi_write(chip_select, 0x0B, 0x06);
  //AD9510_spi_write(chip_select, 0x0C, 0x66);
  // R divider = 1000
  //AD9510_spi_write(chip_select, 0x0B, 0x03);
  //AD9510_spi_write(chip_select, 0x0C, 0xE8);
  
  // Antibacklash pulse. 6.0 ns
  AD9510_spi_write(chip_select, 0x0D, 0x02);
  // Antibacklash pulse. 1.3 ns
  //AD9510_spi_write(chip_select, 0x0D, 0x00);

  // testing end
  // PLL power up
  // output OUT0 - OUT3 - power on
  // voltage output 810mV
  AD9510_spi_write(chip_select, 0x3C, 0x08);
  AD9510_spi_write(chip_select, 0x3D, 0x08);
  AD9510_spi_write(chip_select, 0x3E, 0x08);
  AD9510_spi_write(chip_select, 0x3F, 0x08);

  // OUT4 power up
  // LVDS, 3.5mA, 100ohm termination, power on
  //AD9510_spi_write(chip_select, 0x40, 0x03);
  AD9510_spi_write(chip_select, 0x40, 0x02);
  // output OUT5 - OUT6 - power down
  AD9510_spi_write(chip_select, 0x41, 0x03);
  AD9510_spi_write(chip_select, 0x42, 0x03);
  //AD9510_spi_write(chip_select, 0x41, 0x08);
  //AD9510_spi_write(chip_select, 0x42, 0x08);

  // output OUT7 - power on (clock copy, LVDS)
  // LVDS, 3.5mA, 100ohm termination, power on
  AD9510_spi_write(chip_select, 0x43, 0x02);

  // Clock selection (distribution mode)
  // CLK1 - power down
  // CLK2 - power on
  // Clock select = CLK2
  // Prescaler Clock - Power-Down
  // REFIN - Power-Down
  //AD9510_spi_write(chip_select, 0x45, 0x1A);

  // Clock selection (distribution mode)
  // CLK1 - power down
  // CLK2 - power on
  // Clock select = CLK2
  // Prescaler Clock - Power-Up
  // REFIN - Power-Up
  AD9510_spi_write(chip_select, 0x45, 0x02);

  // Clock selection (distribution mode)
  // CLK1 - power on
  // CLK2 - power down
  // Clock select = CLK1
  // Prescaler Clock -  Power-Down
  // REFIN - Power-Down
  //AD9510_spi_write(chip_select, 0x45, 0x1D);

  AD9510_reg_update(chip_select);

  // Clock dividers OUT0 - OUT3 and OUT7
  // divide = off (bypassed, ratio 1)
  // duty cycle 50%
  // lo-hi 0x00
  // phase offset = 0
  // start high
  // sync
  AD9510_spi_write(chip_select, 0x48, 0x00); // divide by 2 (off)
  AD9510_spi_write(chip_select, 0x4A, 0x00); // divide by 2 (off)
  AD9510_spi_write(chip_select, 0x4C, 0x00); // divide by 2 (off)
  AD9510_spi_write(chip_select, 0x4E, 0x00); // divide by 2 (off)
  AD9510_spi_write(chip_select, 0x50, 0x00); // divide by 2 (off)
  AD9510_spi_write(chip_select, 0x56, 0x00); // divider

  AD9510_spi_write(chip_select, 0x49, 0x90); // phase offset = 0 , divider off
  AD9510_spi_write(chip_select, 0x4B, 0x90); // phase
  AD9510_spi_write(chip_select, 0x4D, 0x90); // phase
  AD9510_spi_write(chip_select, 0x4F, 0x90); // phase
  AD9510_spi_write(chip_select, 0x51, 0x90); // phase
  AD9510_spi_write(chip_select, 0x57, 0x90); // phase

  // Function pin is SYNCB
  AD9510_spi_write(chip_select, 0x58, 0x20); //0010 0000

  AD9510_reg_update(chip_select);

  usleep(10000);

  // software sync
  AD9510_spi_write(chip_select, 0x58, 0x24); //0010 0100
  AD9510_reg_update(chip_select);

  usleep(10000);

  AD9510_spi_write(chip_select, 0x58, 0x20); //0010 0000
  AD9510_reg_update(chip_select);

  // Check configuration

  AD9510_assert(chip_select, 0x04, 0x00);
  AD9510_assert(chip_select, 0x05, 0x00);
  //AD9510_assert(chip_select, 0x05, 0x40);
  //AD9510_assert(chip_select, 0x05, 0x3F);
  AD9510_assert(chip_select, 0x06, 0x0A);
  //AD9510_assert(chip_select, 0x06, 0xC8);
  //AD9510_assert(chip_select, 0x06, 0x00);
  //AD9510_assert(chip_select, 0x06, 0xFC);
  //AD9510_assert(chip_select, 0x06, 0x4A);
  //AD9510_assert(chip_select, 0x06, 0x23);
  //AD9510_assert(chip_select, 0x06, 0x05);
  //AD9510_assert(chip_select, 0x06, 0x02);
  //AD9510_assert(chip_select, 0x06, 0x01);
  //AD9510_assert(chip_select, 0x08, 0x04 | 0x03 | 0x40);
  //AD9510_assert(chip_select, 0x08, 0x08 | 0x03 | 0x40);
  //AD9510_assert(chip_select, 0x08, 0x08 | 0x00 | 0x40);
  //AD9510_assert(chip_select, 0x08, 0x04 | 0x00);
  //AD9510_assert(chip_select, 0x08, 0x20 | 0x03 | 0x40);
  //AD9510_assert(chip_select, 0x08, 0x20 | 0x04 | 0x03 | 0x40);
  //AD9510_assert(chip_select, 0x08, 0x10 | 0x03 | 0x40);

  AD9510_assert(chip_select, 0x09, 0x00);
  //AD9510_assert(chip_select, 0x09, 0x70);

  //AD9510_assert(chip_select, 0x0A, 0x04 | 0x00);
  AD9510_assert(chip_select, 0x0A, 0x00 | 0x00);
  //AD9510_assert(chip_select, 0x0A, 0x08);

  AD9510_assert(chip_select, 0x0B, 0x00);
  AD9510_assert(chip_select, 0x0C, 0x01);
  //AD9510_assert(chip_select, 0x0B, 0x00);
  //AD9510_assert(chip_select, 0x0C, 0x14);
  //AD9510_assert(chip_select, 0x0B, 0x40);
  //AD9510_assert(chip_select, 0x0C, 0x00);
  //AD9510_assert(chip_select, 0x0B, 0x06);
  //AD9510_assert(chip_select, 0x0C, 0x66);
  
  AD9510_assert(chip_select, 0x0D, 0x02);
  //AD9510_assert(chip_select, 0x0D, 0x00);

  AD9510_assert(chip_select, 0x3C, 0x08);
  AD9510_assert(chip_select, 0x3C, 0x08);
  AD9510_assert(chip_select, 0x3D, 0x08);
  AD9510_assert(chip_select, 0x3E, 0x08);
  AD9510_assert(chip_select, 0x3F, 0x08);

  //AD9510_assert(chip_select, 0x40, 0x03);
  AD9510_assert(chip_select, 0x40, 0x02);
  AD9510_assert(chip_select, 0x41, 0x03);
  AD9510_assert(chip_select, 0x42, 0x03);
  AD9510_assert(chip_select, 0x43, 0x02);

  //AD9510_assert(chip_select, 0x45, 0x1A);
  AD9510_assert(chip_select, 0x45, 0x02);
  //AD9510_assert(chip_select, 0x45, 0x1D);

  AD9510_assert(chip_select, 0x48, 0x00);
  AD9510_assert(chip_select, 0x4A, 0x00);
  AD9510_assert(chip_select, 0x4C, 0x00);
  AD9510_assert(chip_select, 0x4E, 0x00);
  AD9510_assert(chip_select, 0x56, 0x00);

  AD9510_assert(chip_select, 0x49, 0x90);
  AD9510_assert(chip_select, 0x4B, 0x90);
  AD9510_assert(chip_select, 0x4D, 0x90);
  AD9510_assert(chip_select, 0x4F, 0x90);
  AD9510_assert(chip_select, 0x57, 0x90);

  return 0;
}

void AD9510_drv::AD9510_assert(uint32_t chip_select, uint8_t reg, uint8_t val) {

  wb_data data;

  data = AD9510_spi_read(chip_select, reg);

  //cout << "AD9510 assert, reg: 0x" << hex << unsigned(reg) <<
  //    " val: 0x" << hex << unsigned(data_.data_read[0] & 0xFF) << " =? 0x" << hex << unsigned(val) << "...";

  printf("AD9510 assert, reg: 0x%02x val: 0x%02x =? 0x%02x...", reg, data.data_read[0] & 0xFF, val);

  assert( (data.data_read[0] & 0xFF) == val);

  printf("passed!\n");

}
