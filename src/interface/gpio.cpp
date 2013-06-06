//============================================================================
// Author      : Andrzej Wojenski
// Version     : 1.0
// Description : Software driver for standard input/ output data flow
//============================================================================
#include "gpio.h"

gpio_int::gpio_int() {

	//cout << showbase << internal << setfill('0') << setw(8);

}

// core_addr not used
int gpio_int::int_reg(WBMaster_unit* wb_master, uint32_t core_addr) {

	this->wb_master = wb_master;
    //this->core_addr = core_addr;

    return 0;

}

// err = 0, everything ok
// err != 0, there was error (mostly -EIO)
int gpio_int::int_send_data(struct wb_data* data) {
	
	wb_master->wb_send_data(data);

	return 0;
}

int gpio_int::int_read_data(struct wb_data* data) {

	wb_master->wb_read_data(data);
	
	return 0;
}
