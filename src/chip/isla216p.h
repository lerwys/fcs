//============================================================================
// Author      : Andrzej Wojenski
// Version     : 1.0
// Description : Software driver for ISLA216P chip (ADC)
//============================================================================
#ifndef ISLA216P_H_
#define ISLA216P_H_

#include "data.h"
#include "commLink.h"

class ISLA216P_drv {
public:

  static void ISLA216P_setCommLink(commLink* comm, string spi_id, string gpio_id);

  static wb_data ISLA216P_spi_write(uint32_t chip_select, uint8_t reg, uint8_t val);
  static wb_data ISLA216P_spi_read(uint32_t chip_select, uint8_t reg);

  // mode = 0 - normal, power on
  // mode = 1 - nap
  // mode = 2 - sleep
  static int ISLA216P_sleep(uint32_t ctrl_reg, uint8_t mode);

  // resets all ADC ISLA chips (one line)
  // and perform auto-calibration
  // Note: must check idependently each chip if calibration is done (with checkCalibration)
  static int ISLA216P_AutoCalibration(uint32_t ctrl_reg);
  // Check if calibration is done
  static int ISLA216P_checkCalibration(uint32_t chip_select);

  static int ISLA216P_config(uint32_t chip_select);

  // Sync multiple ADC ISLA chips (clock phase) - clkdivrst
  // to use this feature clock must be divided at least 2x!
  static int ISLA216P_sync(uint32_t ctrl_reg);

  // Train communication link
  // (turns on test pattern)
  // stage1 - one word (get middle of data window)
  // stage2 - check for 3 word cycle
  // stage3 - sync all chips (no phase offset) 1024 samples
  static int ISLA216P_train(uint32_t chip_select);

  // Test pattern
  // mode - output test mode (as in ISLA datasheet)
  // test_pattern - depending on vector size:
  // 1 = user pattern 1 only
  // 2 = cycle pattern 1,3
  // 3 = cycle pattern 1,3,5
  // 4 = cycle pattern 1,3,5,7
  static int ISLA216P_setTestPattern(uint32_t chip_select, uint8_t mode, vector<uint16_t> test_pattern);
  static int ISLA216P_TestPatternOff(uint32_t chip_select);

  static int ISLA216P_reset(uint32_t ctrl_reg, uint8_t mode);
  // not all regs get default values while using softReset (SPI)
  // resets also auto-calibration status?
  static int ISLA216P_resetSPI(uint32_t chip_select, uint32_t ctrl_reg);

  // Temperature
  // returns temp value
  static uint32_t ISLA216P_getTemp(uint32_t chip_select);

  static uint32_t ISLA216P_getChipID(uint32_t chip_select);
  static uint32_t ISLA216P_getChipVersion(uint32_t chip_select);

  static void ISLA216P_assert(uint32_t chip_select, uint8_t reg, uint8_t val);

private:

  static wb_data data_;
  static commLink* commLink_;
  static string spi_id_;
  static string gpio_id_;
};

#endif /* ISLA216P_H_ */
