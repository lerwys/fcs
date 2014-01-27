//============================================================================
// Author      : Andrzej Wojenski
// Version     : 1.0
// Description : Software driver for Wishbone I2C IP core
//               (standard I2C Master Controller core from OpenCores)
//============================================================================
#ifndef __I2C_H
#define __I2C_H

#include "data.h"

/* I2C register */
#define I2C_PRER_LO (0x00 << WB_GR_SHIFT) // clock
#define I2C_PRER_HI (0x01 << WB_GR_SHIFT)
#define I2C_CTR     (0x02 << WB_GR_SHIFT) // control reg
#define I2C_TXR     (0x03 << WB_GR_SHIFT) // transmit reg
#define I2C_RXR     (0x03 << WB_GR_SHIFT) // receive reg (read)
#define I2C_CR      (0x04 << WB_GR_SHIFT) // command reg
#define I2C_SR      (0x04 << WB_GR_SHIFT) // status reg (read)

/* I2C control register fields mask */
#define I2C_CTR_EN  0x80
#define I2C_CTR_INT 0x40

#define I2C_CR_STA  0x80
#define I2C_CR_STO  0x40
#define I2C_CR_RD   0x20
#define I2C_CR_WR   0x10
#define I2C_CR_ACK  0x08
#define I2C_CR_IACK 0x01

#define I2C_SR_RXACK 0x80 // ack from slave
#define I2C_SR_BUSY  0x40 // i2c busy
#define I2C_SR_AL    0x20 // arbitration lost
#define I2C_SR_TIP   0x02 // transfer in progress
#define I2C_SR_IF    0x01 // interrupt flag

using namespace std;

// extra field [0] - i2c chip address (7 bits)
// extra field [1] - num data read

class i2c_int : public WBInt_drv {
public:

  // proper destructor implementation!
  i2c_int();
  ~i2c_int() {};

  int i2c_init(int sys_freq, int i2c_freq); // config frequency (in Hz)

  int int_reg(WBMaster_unit* wb_master, uint32_t core_addr); // used by commLink

  // extra0 - i2c addr
  // extra1 - number of data read/write
  int int_send_data(struct wb_data* data);
  int int_read_data(struct wb_data* data);
  int int_send_read_data(struct wb_data* data);

private:

  int i2c_check_transfer(int ack_check);

  WBMaster_unit* wb_master;
  uint32_t core_addr;
  int i2c_mode_stop; // 1 - normal stop after write, 0 - don't send stop (used for repeated start)

  wb_data data_;

};

#endif // __I2C_H
