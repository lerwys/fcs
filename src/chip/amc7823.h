//============================================================================
// Author      : Andrzej Wojenski
// Version     : 1.0
// Description : Software driver for AMC7823 chip (temperature monitor)
//============================================================================
#ifndef AMC7823_H_
#define AMC7823_H_

#include "data.h"
#include "commLink.h"

class AMC7823_drv {
public:

  static void AMC7823_setCommLink(commLink* comm, string spi_id, string gpio_id);

  // only one word transfers
  static void AMC7823_spi_write(uint32_t chip_select, uint8_t page, uint8_t reg, uint16_t val);
  static uint16_t AMC7823_spi_read(uint32_t chip_select, uint8_t page, uint8_t reg);

  // 1 - reset done
  // 0 - still in reset mode
  static int AMC7823_checkReset(uint32_t chip_select);
  static void AMC7823_config(uint32_t chip_select);
  static void AMC7823_powerUp(uint32_t chip_select);

  // perform ADC data measurement
  // ADC0, ADC1, ADC2, ADC3, on-chip temp
  static vector<uint16_t> AMC7823_getADCData(uint32_t ctrl_reg, uint32_t chip_select);
  static float AMC7823_tempConvert(uint16_t temp);

  static void AMC7823_assert(uint32_t chip_select, uint8_t page, uint8_t reg, uint16_t val);

private:

  static wb_data data_;
  static commLink* commLink_;
  static string spi_id_;
  static string gpio_id_;

};

#endif /* AMC7823_H_ */
