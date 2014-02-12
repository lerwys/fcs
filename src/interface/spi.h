//============================================================================
// Author      : Andrzej Wojenski
// Version     : 1.0
// Description : Software driver for Wishbone SPI_BIDIR_1_0 IP core
//               (modification of SPI Master Controller from OpenCores
//                with modification to handle bidirectional transfer on MOSI line)
//============================================================================
#ifndef __SPI_H
#define __SPI_H

#include "data.h"

/* SPI BIDIR register */
#define SPI_BIDIR_RX0        (0x00 << 2) // 0
#define SPI_BIDIR_RX1        (0x01 << 2) // 1
#define SPI_BIDIR_RX2        (0x02 << 2) // 2
#define SPI_BIDIR_RX3        (0x03 << 2) // 3
#define SPI_BIDIR_TX0        (0x00 << 2) // 0
#define SPI_BIDIR_TX1        (0x01 << 2) // 1
#define SPI_BIDIR_TX2        (0x02 << 2) // 2
#define SPI_BIDIR_TX3        (0x03 << 2) // 3
#define SPI_BIDIR_CTRL       (0x04 << 2) // 4
#define SPI_BIDIR_DIVIDER    (0x05 << 2) // 5
#define SPI_BIDIR_SS         (0x06 << 2) // 6
#define SPI_BIDIR_CFG_BIDIR  (0x07 << 2) // 7
// For RX data from MISO (single line)
#define SPI_RX_MISO_0        (0x08 << 2) // 8
#define SPI_RX_MISO_1        (0x09 << 2) // 9
#define SPI_RX_MISO_2        (0x0A << 2) // 10
#define SPI_RX_MISO_3        (0x0B << 2) // 11

/* SPI BIDIR fields mask */
#define SPI_BIDIR_CTRL_ASS 0x2000
#define SPI_BIDIR_CTRL_IE 0x1000
#define SPI_BIDIR_CTRL_LSB 0x0800
#define SPI_BIDIR_CTRL_TX_NEG 0x0400
#define SPI_BIDIR_CTRL_RX_NEG 0x0200
#define SPI_BIDIR_CTRL_GO_BSY 0x0100

using namespace std;

// extra field [0] - spi chip address
// extra field [1] - number of bits to transfer

class spi_int : public WBInt_drv {
public:

  // proper destructor implementation!
  spi_int();
  ~spi_int() {};

  int spi_init(int sys_freq, int spi_freq, int config); // config frequency (in Hz)

  int int_reg(WBMaster_unit* wb_master, uint32_t core_addr);

  int int_send_data(struct wb_data* data);
  int int_read_data(struct wb_data* data);
  int int_send_read_data(struct wb_data* data);

private:

  int spi_transfer(int mode, struct wb_data* data);

  WBMaster_unit* wb_master;
  uint32_t core_addr;

  wb_data data_;

};

#endif // __SPI_H
