//============================================================================
// Author      : Lucas Russo
// Version     : 1.0
// Description : Definitions of multiple fmc boards fot kc705 carrier
//============================================================================
#ifndef PLATFORM_KC705_H_
#define PLATFORM_KC705_H_

#include "common.h"

// adc0 -0.483ns -> IDELAY_TAP(7)
// adc1 -0.897ns -> IDELAY_TAP(12)
// adc2 -0.609ns -> IDELAY_TAP(8)
// adc3 -0.415ns -> IDELAY_TAP(6)
const struct delay_lines fmc_250m_ml605_delay_l[] = {
  {DELAY_LINES_INIT,   7},
  {DELAY_LINES_INIT,  12},
  {DELAY_LINES_INIT,   8},
  {DELAY_LINES_INIT,   6},
  {DELAY_LINES_END,   -1}
};

// adc0 -1.364ns -> IDELAY_TAP(18)
// adc1 -0.705ns -> IDELAY_TAP(9)
// adc2 -0.996ns -> IDELAY_TAP(13)

const struct delay_lines fmc_250m_kc705_delay_l[] = {
  {DELAY_LINES_INIT,  18},
  {DELAY_LINES_INIT,   9},
  {DELAY_LINES_INIT,  13},
  {DELAY_LINES_NO_INIT, 0},  // for kc705 board there is no channel 3 available
  {DELAY_LINES_END,   -1}
};


#endif /* PLATFORM_H_ */
