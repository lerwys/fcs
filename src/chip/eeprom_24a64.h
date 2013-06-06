//============================================================================
// Author      : Andrzej Wojenski
// Version     : 1.0
// Description : Software driver for EEPROM 24A64 chip
//============================================================================
#ifndef EEPROM_H_
#define EEPROM_H_

#include "data.h"
#include "commLink.h"

class EEPROM_drv {
public:

  static void EEPROM_setCommLink(commLink* comm, string i2c_id);


  static int EEPROM_switch(uint32_t chip_addr); // KC705 board

  // only check if EEPROM is present
  static int EEPROM_sendData(uint32_t chip_addr);

  // not implemented
  static int EEPROM_readData(uint32_t chip_addr);


private:

  static wb_data data_;
  static commLink* commLink_;
  static string i2c_id_;
};

#endif /* EEPROM_H_ */
