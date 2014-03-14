//============================================================================
// Author      : Andrzej Wojenski
// Version     : 1.0
// Description : Class for handling pointers to interface objects
//               Allows interface drivers to send and read data through communication interfaces
//               Options avaiable:
//               - send or read data to/ from IP core of a communication interface
//               - send or read data to/ from chip (using communication interface)
//============================================================================
#ifndef COMMLINK_H_
#define COMMLINK_H_

#include "data.h"

#include <map>
#include <string>
#include <iostream>

using namespace std;

// Communication link
// Automatically transfers data between Wishbone interface and chip interface (like I2C)
class commLink {
public:
  commLink();
  ~commLink() {
    // TODO Auto-generated destructor stub

  }

  // Register software driver
  WBMaster_unit* regWBMaster(WBMaster_unit* wb_master_unit); // register software driver for Wishbone master (RS-232, PCI-E driver)
  WBInt_drv* regIntDrv(string interfaceName, uint32_t core_addr, WBInt_drv* interfaceDrv); // register software driver for communication interface (I2C, SPI)

  int fmc_rst_int();
  // Config communication interface (FPGA core)
  int fmc_config_send(struct wb_data* data); // send interface config data
  int fmc_config_read(struct wb_data* data); // read interface config data
  int fmc_config_read_unsafe(struct wb_data* data, uint32_t *data_out);

  // Send data through communication interface (like I2C, SPI)
  int fmc_send(string intName, struct wb_data* data); // send data through interface
  int fmc_send_read(string intName, struct wb_data* data); // send data through interface
  int fmc_read(string intName, struct wb_data* data); // read data from interface

private:

  WBInt_drv* searchIntDrv(string intName);

  WBMaster_unit* wb_master;
  map<string, WBInt_drv*> fmc_interface;


  WBInt_drv* interface;
  map<string, WBInt_drv*>::iterator interface_it;
};

#endif /* COMMLINK_H_ */
