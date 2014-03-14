//============================================================================
// Author      : Andrzej Wojenski
// Version     : 1.0
//============================================================================
#include "commLink.h"

commLink::commLink() {

}

WBMaster_unit* commLink::regWBMaster(WBMaster_unit* wb_master_unit) {
	wb_master = wb_master_unit;
	return wb_master_unit;
}

int commLink::fmc_rst_int() {
    wb_master->wb_rst();
    return 0;
}

WBInt_drv* commLink::regIntDrv(string interfaceName, uint32_t core_addr, WBInt_drv* interfaceDrv) {

	// check if driver isn't already registered
	if (fmc_interface.find(interfaceName) != fmc_interface.end()) {
		cout << "Interface is already registered!" << endl;
		return NULL;
	}

	// add driver
	interfaceDrv->int_reg(wb_master, core_addr);
	fmc_interface.insert(pair<string, WBInt_drv* >(interfaceName, interfaceDrv));

	return interfaceDrv;
}

int commLink::fmc_config_send(struct wb_data* data) {
	// just send data through Wishbone master
	return wb_master->wb_send_data(data);
}

int commLink::fmc_config_read(struct wb_data* data) {

	data->data_read.clear();

	return wb_master->wb_read_data(data);
}

int commLink::fmc_config_read_unsafe(struct wb_data* data, uint32_t *data_out) {

	data->data_read.clear();

	return wb_master->wb_read_data_unsafe(data, data_out);
}

int commLink::fmc_send(string intName, struct wb_data* data) {

	data->data_read.clear();

	// Search for interface driver
	interface = searchIntDrv(intName);

	if (interface == NULL) {
		cout << "Interface not found!" << endl;
		return 1;
	}

	// Send data
	return interface->int_send_data(data);

}

int commLink::fmc_send_read(string intName, struct wb_data* data) {

	data->data_read.clear();

	// Search for interface driver
	interface = searchIntDrv(intName);

	if (interface == NULL) {
		cout << "Interface not found!" << endl;
		return 1;
	}

	// Send and read data
	return interface->int_send_read_data(data);

}

int commLink::fmc_read(string intName, struct wb_data* data) {

	// Search for interface driver
	interface = searchIntDrv(intName);

	if (interface == NULL) {
		cout << "Interface not found!" << endl;
		return 1;
	}

	// Read data
	return interface->int_read_data(data);

}

WBInt_drv* commLink::searchIntDrv(string intName) {

	interface_it = fmc_interface.find(intName);

	if (interface_it == fmc_interface.end())
		return NULL;
	else
		return interface_it->second;


}
