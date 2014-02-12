//============================================================================
// Author      : Andrzej Wojenski
// Version     : 1.0
// Description : Software driver for PCIe-Wishbone Master IP core
//============================================================================
#ifndef __PCIE_LINK_H
#define __PCIE_LINK_H

#include "../data.h"

#include "../wishbone/lib/pciDriver.h"
#include <iostream>
#include <iomanip>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <stdint.h>

#include <pthread.h>

using namespace pciDriver;
using namespace std;

#define REG_SDRAM_PG 0x1C
#define REG_GSR 0x20
#define GSR_BIT_DDR_RDY (0x1 << 7)
#define REG_WB_PG 0x24

class pcie_link_driver : public WBMaster_unit {
public:

	pcie_link_driver(int num); // PCIe device number
	~pcie_link_driver();

	// return - 1 error, 0 ok
	int wb_send_data(struct wb_data* data);
	// return - 1 error, 0 ok
	int wb_read_data(struct wb_data* data);

private:

	pciDriver::PciDevice *dev;

	uint32_t *bar0, *bar2;
	uint64_t  *bar4;

	uint32_t bar0size, bar2size;
	uint64_t bar4size;

	uint64_t addr, offset;

	int debug;

	int init(int num);
	int reset();

	uint64_t setPage(uint32_t* bar0, uint64_t* bar4, uint64_t bar4_size, uint64_t addr);
	uint32_t setPageRAM(uint32_t* bar0, uint32_t* bar2, uint32_t bar2_size, uint32_t addr);

};

#endif // __PCIE_LINK_H
