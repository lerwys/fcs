//============================================================================
// Author      : Lucas Russo
// Version     : 1.0
// Description : 
//============================================================================

#include "fmc_config_130m_4ch.h"

const struct ddr3_acq_chan_s ddr3_acq_chan[END_CHAN_ID] = {
  {ADC_CHAN_ID, DDR3_ADC_START_ADDR, DDR3_ADC_END_ADDR, DDR3_ADC_MAX_SAMPLES},
  {TBTAMP_CHAN_ID, DDR3_TBTAMP_START_ADDR, DDR3_TBTAMP_END_ADDR, DDR3_TBTAMP_MAX_SAMPLES},
  {TBTPOS_CHAN_ID, DDR3_TBTPOS_START_ADDR, DDR3_TBTPOS_END_ADDR, DDR3_TBTPOS_MAX_SAMPLES},
  {FOFBAMP_CHAN_ID, DDR3_FOFBAMP_START_ADDR, DDR3_FOFBAMP_END_ADDR, DDR3_FOFBAMP_MAX_SAMPLES},
  {FOFBPOS_CHAN_ID, DDR3_FOFBPOS_START_ADDR, DDR3_FOFBPOS_END_ADDR, DDR3_FOFBPOS_MAX_SAMPLES}
};
