//============================================================================
// Author      : Andrzej Wojenski
// Version     : 1.0
// Description : Software driver for EEPROM 24A64 chip
//============================================================================
#include "eeprom_24a64.h"

#define MAX_REPEAT 10

wb_data EEPROM_drv::data_;
commLink* EEPROM_drv::commLink_;
string EEPROM_drv::i2c_id_;

void EEPROM_drv::EEPROM_setCommLink(commLink* comm, string i2c_id) {

	commLink_ = comm;
	i2c_id_ = i2c_id;
	data_.data_send.resize(1);
	data_.extra.resize(1);

}

int EEPROM_drv::EEPROM_switch(uint32_t chip_addr) {

	int err = 0;

	// send data to change i2c switch
	data_.extra[0] = 0x74; // switch addr
	data_.extra[1] = 1;
	data_.data_send[0] = chip_addr;

	err = commLink_->fmc_send(i2c_id_, &data_);
	if (err != 0)
		return err;

	return 0;
}

int EEPROM_drv::EEPROM_sendData(uint32_t chip_addr) {

	int err = 0;

	data_.extra[0] = chip_addr; // chip addr
	data_.extra[1] = 1;

	data_.data_send[0] = 0x00;

	err = commLink_->fmc_send(i2c_id_, &data_);
	if (err != 0) {

		cout << "EEPROM 24A64T chip is not present!!!" << endl;
		return err;
	}

	cout << "EEPROM 24A64T chip is present!" << endl;

	return 0;
}
