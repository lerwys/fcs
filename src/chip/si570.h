//============================================================================
// Author      : Andrzej Wojenski
// Version     : 1.0
// Description : Software driver for Si570/ Si571 chip (clock generator)
//============================================================================
#ifndef SI570_H_
#define SI570_H_

#include "data.h"
#include "commLink.h"

class Si570_drv {
public:

  static void si570_setCommLink(commLink* comm, string i2c_id, string gpio_id);

  // all fields optional
  // extra[0] - Si570 address
  // extra[1] - num of registers to read
  // data_send[0] - starting register
  static int si570_read_freq(wb_data* data);

  // extra[0] - Si570 address
  // data_send[0...6] - configuration registers
  static int si570_set_freq(wb_data* data);

  // addr - Wishbone register address
  static int si570_outputEnable(uint32_t addr);
  static int si570_outputDisable(uint32_t addr);

  // for tests
  // reg - to read
  // val - expected val
  static void si570_assert(uint32_t chip_addr, uint8_t reg, uint8_t val);

private:

  static wb_data data_;
  static commLink* commLink_;
  static string i2c_id_;
  static string gpio_id_;
};

#endif /* SI570_H_ */
