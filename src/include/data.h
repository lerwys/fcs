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
