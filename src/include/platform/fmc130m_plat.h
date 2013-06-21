//============================================================================
// Author      : Lucas Russo
// Version     : 1.0
// Description :  Definitions of multiple fpga carrier boards for the
// fmc130m board
//============================================================================
#ifndef FMC130M_PLAT_H_
#define FMC130M_PLAT_H_

#include "common.h"

// adc0 -1.780/0.078 -> IDELAY_TAP(23)
// adc1 -1.947/0.078 -> IDELAY_TAP(25)
// adc2 -0.786/0.078 -> IDELAY_TAP(10)
// adc3 -1.509/0.078 -> IDELAY_TAP(20)
const struct delay_lines fmc_130m_ml605_delay_l[] = {
  {DELAY_LINES_INIT,  23+6},
  {DELAY_LINES_INIT,  25+6},
  {DELAY_LINES_INIT,  11+6},
  {DELAY_LINES_INIT,  20+6},
  {DELAY_LINES_END,  -1}
};

// adc0 ? -> IDELAY_TAP(7)
// adc1 ? -> IDELAY_TAP(7)
// adc2 ? -> IDELAY_TAP(7)
// adc3 ? -> IDELAY_TAP(7)
const struct delay_lines fmc_130m_kc705_delay_l[] = {
  {DELAY_LINES_INIT,  7},
  {DELAY_LINES_INIT,  7},
  {DELAY_LINES_INIT,  7},
  {DELAY_LINES_INIT,  7},
  {DELAY_LINES_END,  -1}
};

#endif /* FMC130M_PLAT_H_ */
