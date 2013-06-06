//============================================================================
// Author      : Andrzej Wojenski
// Version     : 1.0
// Description : Software driver for AD9510 chip (clock distribution)
//============================================================================
#ifndef AD9510_H_
#define AD9510_H_

#include "data.h"
#include "commLink.h"

class AD9510_drv {
public:

  static void AD9510_setCommLink(commLink* comm, string spi_id);

  // only one byte read/write
  static wb_data AD9510_spi_write(uint32_t chip_select, uint8_t reg, uint8_t val);
  static wb_data AD9510_spi_read(uint32_t chip_select, uint8_t reg);

  // provide chip select and reg address
  static int AD9510_reg_update(uint32_t chip_select); // transfers registers to internal regs of AD9510 chip

  // Configuration for clock distribution mode (clk2) (using AD9510 chip)
  // Si571 clk = 250MHz
  // AD9510 - divider pass-through (output is 250MHz)
  static int AD9510_config_si570(uint32_t chip_select);

  // Configuration for clock distribution mode (clk2) (using AD9510 chip)
  // Si571 clk = 130MHz
  // AD9510 - divider pass-through (output is 250MHz)
  // FPGA working on copy of ADC clock (FPGA_CLK, output 7 from AD9510 chip)
  static int AD9510_config_si570_fmc_adc_130m_4ch(uint32_t chip_select);

  static void AD9510_assert(uint32_t chip_select, uint8_t reg, uint8_t val);

private:

  static wb_data data_;
  static commLink* commLink_;
  static string spi_id_;

};

#endif /* AD9510_H_ */
