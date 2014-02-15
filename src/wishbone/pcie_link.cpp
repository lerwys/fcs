//============================================================================
// Author      : Andrzej Wojenski
// Version     : 1.0
// Description : Software driver for PCIe-Wishbone Master IP core
//============================================================================
#include "pcie_link.h"

#define STATUS_OK 0
#define STATUS_ERR 1

class BDA {
public:
	uint32_t pa_h;
	uint32_t pa_l;
	uint32_t ha_h;
	uint32_t ha_l;
	uint32_t length;
	uint32_t control;
	uint32_t next_bda_h;
	uint32_t next_bda_l;
	uint32_t status;

	void write(volatile uint32_t *base) {
		base[0] = pa_h;
		base[1] = pa_l;
		base[2] = ha_h;
		base[3] = ha_l;
		base[4] = next_bda_h;
		base[5] = next_bda_l;
		base[6] = length;
		base[7] = control;		// control is written at the end, starts DMA
	}

	void reset(volatile uint32_t *base) {
		base[7] = 0x0200000A;
	}

	inline void wait_finish(volatile uint32_t *base) {
		//check for END bit
		do {
			status = base[8];
		} while(!(status & 0x1));
	}
};


class DDList {
public:
	DDList(int size) {
		lod = new BDA[size];
		this->count = size;
	}
	~DDList() {
		delete lod;
	}

	int length() { return count; }

	BDA& operator[](int index) {
		return lod[index];
	}
private:
	int count;
	BDA *lod;
};

pcie_link_driver::pcie_link_driver(int num) {

	debug = 0;

	init(num);
	reset();

	cerr << "PCIe Link driver: PCIe Link interface configuration done successfuly!" << endl;

}

pcie_link_driver::~pcie_link_driver() {

	// Unmap BARs
	dev->unmapBAR(0,bar0);
	dev->unmapBAR(2,bar2);
	dev->unmapBAR(4,bar4);

	// Close device
	dev->close();
}

int pcie_link_driver::reset() {

	// reset function
	// not used

	return STATUS_OK;

}

int pcie_link_driver::init(int num) {

	cerr << "PCIe Link driver: Init function - WB Master Component" << endl;
	cerr << "PCIe Link driver: PCIe Link interface configuration in progress..." << endl;

	try {
		cout << "PCIe Link driver: Trying device " << num << " ... ";
		dev = new pciDriver::PciDevice( num );
		cout << "found" << endl;

	} catch (Exception& e) {
		cout << "failed: " << e.toString() << endl;
		return STATUS_ERR;
	}

	// Initializing PCIe interface
	dev->open();

	// Map BARs
	try {
		bar0 = static_cast<uint32_t *>( dev->mapBAR(0) );
		bar2 = static_cast<uint32_t *>( dev->mapBAR(2) );
		bar4 = static_cast<uint64_t *>( dev->mapBAR(4) );
	}
	catch (Exception& e) {
		cerr << "PCIe Link driver: Failed while mapping BARs" << endl;
		return STATUS_ERR;
	}

	// Get BAR sizes
	bar0size = dev->getBARsize(0);
	bar2size = dev->getBARsize(2);
	bar4size = dev->getBARsize(4);

	cerr << "PCIe Link driver: BAR0 size: " << bar0size << endl;
	cerr << "PCIe Link driver: BAR2 size: " << bar2size << endl;
	cerr << "PCIe Link driver: BAR4 size: " << bar4size << endl;

	bar0[REG_SDRAM_PG >> 2] = 0; // set page to 0

	return STATUS_OK;

}

// Set page and return address offset
uint64_t pcie_link_driver::setPage(uint32_t* bar0, uint64_t* bar4, uint64_t bar4_size, uint64_t addr) {

	uint32_t page;
	uint32_t offset;

	bar4_size = bar4_size >> 3;
	page = addr / bar4_size;
	offset = addr % bar4_size;

	bar0[REG_WB_PG >> 2] = page;

	//if (debug == 1)
		//fprintf(stderr, "PCIe Link driver: setPage: addr: %x page: %x offset: %x\n", addr, page, offset);

	return offset;
}

// Set page and return address offset
uint32_t pcie_link_driver::setPageRAM(uint32_t* bar0, uint32_t* bar2, uint32_t bar2_size, uint32_t addr) {

	uint32_t page;
	uint32_t offset;

	bar2_size = bar2_size >> 3;
	page = addr / bar2_size;
	offset = addr % bar2_size;

	bar0[REG_SDRAM_PG >> 2] = page;

	//if (debug == 1)
		//fprintf(stderr, "PCIe Link driver: setPage: addr: %x page: %x offset: %x\n", addr, page, offset);

	return offset;
}

// if more data is to be sent, addr is automatically incremented by 1
int pcie_link_driver::wb_send_data(struct wb_data* data) {

	addr =  data->wb_addr;

    // check extra vector before using it
    if (data->extra.size() == 0) {
      data->extra.resize(2);
    }
    // extra field 0 for number of data to be read
    else if (data->extra[0] == 0) {
      data->extra[0] = 1;
    }

	for (unsigned int i = 0; i < data->extra[0]; i++) {
		offset = setPage(bar0, bar4, bar4size, addr);
		bar4[offset] = data->data_send[0];

		if (debug == 1)
			fprintf(stderr, "PCIe Link driver: Write function, offset: %x value: %x\n", offset, data->data_send[0]);

		addr++;
	}

	data->extra[0] = 1; // reset counter
	data->extra[1] = 0; // default is Wishbone mode

	return STATUS_OK;

}

int pcie_link_driver::wb_read_data(struct wb_data* data) {

	uint64_t val;

	addr =  data->wb_addr;
	data->data_read.clear();

    // check extra vector before using it
    if (data->extra.size() == 0) {
      data->extra.resize(2);
    }
    // extra field 0 for number of data to be read
    else if (data->extra[0] == 0) {
      data->extra[0] = 1;
    }

	for (unsigned int i = 0; i < data->extra[0]; i++) {

		if (data->extra[1] == 1) { // change to ram
			offset = setPageRAM(bar0, bar2, bar2size, addr);
			val = bar2[offset];
		}
		else {
			offset = setPage(bar0, bar4, bar4size, addr);
			val = bar4[offset];
		}

		data->data_read.push_back(val);

		if (debug == 1)
			fprintf(stderr, "PCIe Link driver: Read function, offset: %lx value: %x\n", offset, val);

		addr++;
	}

	data->extra[0] = 1; // reset counter
	data->extra[1] = 0; // default is Wishbone mode

	return STATUS_OK;

}

/* This should be used with caution!!! */
int pcie_link_driver::wb_read_data_unsafe(struct wb_data* data, uint32_t *data_out)
{
    // check extra vector before using it
    if (data->extra.size() == 0) {
      data->extra.resize(1);
    }
    // extra field 0 for number of data to be read
    else if (data->extra[0] == 0) {
      data->extra[0] = 1;
    }

    fprintf (stderr, "wb_read_data_unsafe: reading %d bytes from addr %lX\n", data->extra[0],
            data->wb_addr);

    uint32_t num_bytes_page;
    uint32_t num_bytes = data->extra[0];/**sizeof(uint32_t);*/
    uint32_t num_pages = (num_bytes < bar2size) ? 1 : num_bytes/bar2size;
    uint32_t page_start = data->wb_addr / bar2size;
    uint32_t offset = data->wb_addr % bar2size;
    uint32_t num_bytes_rem = num_bytes;

	for (unsigned int i = page_start; i < page_start+num_pages; ++i) {
        bar0[REG_SDRAM_PG >> 2] = i;
        num_bytes_page = (num_bytes_rem > bar2size) ? 
				(bar2size-offset) : (num_bytes_rem-offset);
        num_bytes_rem -= num_bytes_page;
        
        memcpy ((uint8_t *)data_out,
                (uint8_t *)bar2 + offset,
                num_bytes_page);
        data_out = (uint32_t *)((uint8_t *)data_out + num_bytes_page);
        offset = 0; // after the first page this will always be 0
	}

	data->extra[0] = 1; // reset counter
	data->extra[1] = 0; // default is Wishbone mode

	return STATUS_OK;
}
