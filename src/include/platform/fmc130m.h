//============================================================================
// Author      : Lucas Russo
// Version     : 1.0
// Description :  Definitions of multiple fmc boards fot ml605 carrier
//============================================================================
#ifndef PLATFORM_ML605_H_
#define PLATFORM_ML605_H_

#include "common.h"

// adc0 ? -> IDELAY_TAP(7)
// adc1 ? -> IDELAY_TAP(7)
// adc2 ? -> IDELAY_TAP(7)
// adc3 ? -> IDELAY_TAP(7)
const struct delay_lines fmc_130m_ml605_delay_l[] = {
  {DELAY_LINES_INIT,  7},
  {DELAY_LINES_INIT,  7},
  {DELAY_LINES_INIT,  7},
  {DELAY_LINES_INIT,  7},
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

#endif /* PLATFORM_ML605_H_ */
