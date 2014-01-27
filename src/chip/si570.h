//============================================================================
// Author      : Andrzej Wojenski
// Version     : 1.0
// Description : Software driver for Si570/ Si571 chip (clock generator)
//============================================================================
// Parts taken from si570 linux kernel driver
#ifndef SI570_H_
#define SI570_H_

#include "data.h"
#include "commLink.h"

/* Si570 registers */
#define SI570_REG_HS_N1                     7
#define SI570_REG_N1_RFREQ0                 8
#define SI570_REG_RFREQ1                    9
#define SI570_REG_RFREQ2                    10
#define SI570_REG_RFREQ3                    11
#define SI570_REG_RFREQ4                    12
#define SI570_REG_CONTROL                   135
#define SI570_REG_FREEZE_DCO                137
#define SI570_REG_START                     SI570_REG_HS_N1
#define SI570_NUM_FREQ_REGS                 6

#define HS_DIV_SHIFT                        5
#define HS_DIV_MASK                         0xe0
#define HS_DIV_OFFSET                       4
#define N1_6_2_MASK                         0x1f
#define N1_1_0_MASK                         0xc0
#define RFREQ_37_32_MASK                    0x3f
#define RFREQ_31_28_MASK                    0xf0
#define RFREQ_27_24_MASK                    0x0f

#define SI570_FOUT_FACTORY_DFLT             125000000LL
#define SI598_FOUT_FACTORY_DFLT             10000000LL

#define SI570_MIN_FREQ                      10000000L
#define SI570_MAX_FREQ                      1417500000L
#define SI598_MAX_FREQ                      525000000L

#define FDCO_MIN                            4850000000LL
#define FDCO_MAX                            5670000000LL
#define FDCO_CENTER                         ((FDCO_MIN + FDCO_MAX) / 2)

#define SI570_CNTRL_RECALL                  (1 << 0)
#define SI570_CNTRL_FREEZE_ADC              (1 << 4)
#define SI570_CNTRL_FREEZE_M                (1 << 5)
#define SI570_CNTRL_NEWFREQ                 (1 << 6)
#define SI570_CNTRL_RESET                   (1 << 7)

#define SI570_CNTRL_NEWFREQ_MASK            0xbf
#define SI570_CNTRL_NEWFREQ_SHIFT           6

#define SI570_FREEZE_DCO                    (1 << 4)
#define SI570_UNFREEZE_DCO                  (0)

class Si570_drv {
public:

  static void si570_setCommLink(commLink* comm, string i2c_id, string gpio_id);

  // all fields optional
  // extra[0] - Si570 address
  // extra[1] - num of registers to read
  // data_send[0] - starting register
  static int si570_read_freq(wb_data* data);

  // extra[0] - Si570 address
  // data_send[0...6] - configuration registers
  static int si570_set_freq(wb_data* data);

  // addr - Wishbone register address
  static int si570_outputEnable(uint32_t addr);
  static int si570_outputDisable(uint32_t addr);

  // for tests
  // reg - to read
  // val - expected val
  static void si570_assert(uint32_t chip_addr, uint8_t reg, uint8_t val);

private:

  static wb_data data_;
  static commLink* commLink_;
  static string i2c_id_;
  static string gpio_id_;
};

#endif /* SI570_H_ */
