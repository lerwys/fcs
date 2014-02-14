//============================================================================
// Author      : Lucas Russo
// Version     : 1.0
// Description : Software driver for PCIe-Wishbone Master IP core
//============================================================================
#ifndef __FMC_CONFIG_130M_4CH_BOARD_H
#define __FMC_CONFIG_130M_4CH_BOARD_H

#include <iostream>
#include <iomanip>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <stdint.h>

#include "reg_map/fmc_config_130m_4ch.h"
#include "config.h"
#include "plat_opts.h"
#include "data.h"
#include "commlink/commLink.h"
#include "wishbone/rs232_syscon.h"
#include "wishbone/pcie_link.h"
#include "interface/i2c.h"
#include "interface/spi.h"
#include "interface/gpio.h"
#include "chip/si570.h"
#include "chip/ad9510.h"
#include "chip/isla216p.h"
#include "chip/amc7823.h"
#include "chip/eeprom_24a64.h"
#include "chip/lm75a.h"
#include "platform/fmc130m_plat.h"

using namespace std;

class fmc_config_130m_4ch_board {
public:

	fmc_config_130m_4ch_board(WBMaster_unit* wb_master_unit, const delay_lines *delay_data_l,
                                        const delay_lines *delay_clk_l);
	~fmc_config_130m_4ch_board();

  int init(WBMaster_unit* wb_master_unit, const delay_lines *delay_data_l,
                                        const delay_lines *delay_clk_l);

  /* Exported functions */
  int blink_leds();
  int config_defaults();
  int set_kx(uint32_t kx);
  int set_ky(uint32_t ky);
  int set_ksum(uint32_t ksum);

private:

	commLink* _commLink;

  WBInt_drv* int_drv;
  //wb_data data;

  const struct delay_lines *delay_data_l;
  const struct delay_lines *delay_clk_l;
};

#endif // __FMC_CONFIG_130M_4CH_BOARD_H
