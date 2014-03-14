//============================================================================
// Author      : Andrzej Wojenski
// Version     : 1.0
// Description : Software driver for RS232-Wishbone Master IP core
//============================================================================
#ifndef __RS232_SYSCON_H
#define __RS232_SYSCON_H

#include "data.h"

#include <mxml.h>
#include <SerialStream.h>

#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <algorithm>
#include <stdint.h>
#include <stdlib.h>

#define RS232_PORT "/dev/ttyUSB0"
// -lserial
using namespace LibSerial;

class rs232_syscon_driver : public WBMaster_unit {
public:

  // proper destructor implementation!
  rs232_syscon_driver();
  ~rs232_syscon_driver();

  int wb_rst();
  // return - 1 error, 0 ok
  int wb_send_data(struct wb_data* data);
  // return - 1 error, 0 ok
  int wb_read_data(struct wb_data* data);

private:

  SerialStream serial_port;

  int init();
  int reset();
  int __reset2();

  int send_interface(string polecenie, struct wb_data* data = NULL);
  int read_interface(struct wb_data* data = NULL);

  wb_data dane_;
  string polecenie;
  int mode;

  int debug;
  int init_state;

};

#endif // __RS232_SYSCON_H
