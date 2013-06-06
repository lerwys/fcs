//============================================================================
// Author      : Andrzej Wojenski
// Version     : 1.0
// Description : Software driver for LM75A chip (temperature sensor)
//============================================================================
#ifndef LM75A_H_
#define LM75A_H_

#include "data.h"
#include "commLink.h"

class LM75A_drv {
public:

  static void LM75A_setCommLink(commLink* comm, string i2c_id);

  static void LM75A_setPtrReg(uint16_t chip_addr, uint32_t reg); // pointer register (chip have timeout)

  static void LM75A_setConfig(uint16_t chip_addr, uint8_t data);
  // data is read in two complement format, 9 bits
  static float LM75A_readTemp(uint16_t chip_addr);
  static uint16_t LM75A_readID(uint16_t chip_addr);

  // for tests
  // reg - to read
  // val - expected val
  // not implemented
  static void LM75A_assert(uint32_t chip_addr, uint16_t reg, uint16_t val);

private:

  static wb_data data_;
  static commLink* commLink_;
  static string i2c_id_;
};

#endif /* LM75A_H_ */
