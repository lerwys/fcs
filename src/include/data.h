//============================================================================
// Author      : Andrzej Wojenski
// Version     : 1.0
// Description : Structure for data exchange between main FMC card driver and communication interfaces
//               and some other helpful macros
//============================================================================
#ifndef DATA_H_
#define DATA_H_

#include <vector>
#include <string>
#include <stdint.h>
#include <cstdio>
#include <ios>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <cassert>

#include "wbmaster_unit.h"
#include "wbint_drv.h"

using namespace std;

#define WB_GR_WORD (1 << 0)
#define WB_GR_BYTE (1 << 1)

// Wishbone word size in bytes
#define WB_WORD_SIZE 0x4 // 32 bits

// WORD size shift
//#if WB_WORD_SIZE == 0x8 // 64 bits
//#define WB_WORD_SHIFT 0x3
#if WB_WORD_SIZE == 0x4 // 32 bits
#define WB_WORD_SHIFT 0x2
#elif WB_WORD_SIZE == 0x2 // 16 bits
#define WB_WORD_SHIFT 0x1
#else
#error "Unsupported Wishbone Word Size!"
#endif

// DWORD size shift
//#define WB_DWORD_SHIFT (WB_WORD_SHIFT+1)

// Wishbone granularity shift in bytes
#define WB_GR_SHIFT_BYTE 0x0 // always 0, no shift needed
#define WB_GR_SHIFT_WORD WB_WORD_SHIFT // shift log2(word_size) bits
//#define WB_GR_SHIFT_DWORD WB_DWORD_SHIFT // shift log2(word_size)+1 bits

// Macros for handling BYTE vs. WORD access on Wishbone Bus
// Posiible values are WB_GR_BYTE or WB_GR_WORD
#ifndef WB_GR_ACC // Wishbone Granularity Acess
#define WB_GR_ACC WB_GR_WORD // Default to byte access
#endif

#if (WB_GR_ACC & WB_GR_BYTE)
#define WB_GR_SHIFT WB_GR_SHIFT_WORD // We left shift the addresses by this to get byte granularity
//#error "Selecting Wishbone BYTE granularity"
#elif (WB_GR_ACC & WB_GR_WORD)
#define WB_GR_SHIFT WB_GR_SHIFT_BYTE // We left shift the addresses by this to get word granularity
//#error "Selecting Wishbone WORD granularity"
#else
#error "Unsupported Wishbone Granularity Access Type!"
#endif

// Macros for IODELAY handling
#define IDELAY_LINE(x) (0x01 << 1) << x
#define IDELAY_ALL_LINES (0x01FFF << 1) // maximum 8 + 1 line
#define IDELAY_TAP(x) (x & 0x1F) << 18
#define IDELAY_UPDATE 0x01

struct wb_data {

  vector<uint32_t> data_send; // data to send through Wishbone or interface
  vector<uint32_t> data_read; // data to send read from Wishbone or interface

  uint32_t wb_addr; // Wishbone or interface specific address
  vector<uint32_t> extra; // extra filed, like chip address
  int status;

};

#endif /* DATA_H_ */
