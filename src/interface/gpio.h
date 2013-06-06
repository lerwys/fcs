//============================================================================
// Author      : Andrzej Wojenski
// Version     : 1.0
// Description : Software driver for standard input/ output data flow
//============================================================================
#ifndef __GPIO_H
#define __GPIO_H

#include "data.h"

class gpio_int : public WBInt_drv {
public:

  gpio_int();
  ~gpio_int() {};

  int int_reg(WBMaster_unit* wb_master, uint32_t core_addr); // used by commLink

  int int_send_data(struct wb_data* data);
  int int_read_data(struct wb_data* data);
  int int_send_read_data(struct wb_data* data) { cout << "GPIO not implemented method" << endl; return 1; };

private:

  WBMaster_unit* wb_master;
  uint32_t core_addr;
  wb_data data_;

};

#endif // __GPIO_H
