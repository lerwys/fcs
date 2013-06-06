//============================================================================
// Author      : Andrzej Wojenski
// Version     : 1.0
// Description : Software driver for Wishbone I2C IP core
//               (standard I2C Master Controller core from OpenCores)
//============================================================================
#include "i2c.h"

#define MAX_REPEAT 10

i2c_int::i2c_int() {

	//cout << showbase << internal << setfill('0') << setw(8);

}

int i2c_int::int_reg(WBMaster_unit* wb_master, uint32_t core_addr) {

	this->wb_master = wb_master;
    this->core_addr = core_addr;
    data_.data_send.resize(1);
    data_.data_read.clear();

    return 0;

}
 
int i2c_int::i2c_check_transfer(int ack_check) {

	int err = 0;
	int repeat = 0;

	// wait for TIP to negate
	while(1) { // TIP = 0, transfer complete

		data_.wb_addr = core_addr | I2C_SR;
		wb_master->wb_read_data(&data_);

		if ( (data_.data_read[0] & I2C_SR_TIP) == 0)
			break;

		usleep(1000);
		repeat++;

		if (repeat > MAX_REPEAT) {
			// write status register
			cout << "i2c_drv: i2c TIP error" << endl;
			err = 1;
			return err;
		}
	}

	if (ack_check == 0) // not checking if core is in reading mode
		return err;

	// check RxAck (should be 0)
	data_.wb_addr = core_addr | I2C_SR;
	wb_master->wb_read_data(&data_);

	if ((data_.data_read[0] & I2C_SR_RXACK) != 0) {
		cout << "i2c_drv: i2c ack err" << endl;
		return 1;
	}

	return err;

}

int i2c_int::i2c_init(int sys_freq, int i2c_freq)
{
	/* set frequency of i2c to I2C_FREQ (from SYS_FREQ) */
	float f_freq = (float)sys_freq/(5.0 * (float)i2c_freq) - 1.0;
	uint16_t freq = f_freq;

	//cout << showbase << internal << setfill('0');

	printf("i2c_drv: freq: 0x%04x, core_addr: 0x%08x\n", freq, core_addr);

	data_.data_send[0] = freq & 0xFF;
	data_.wb_addr = core_addr | I2C_PRER_LO;
	wb_master->wb_send_data(&data_);

	data_.data_send[0] = (freq & 0xFF00) >> 8;
	data_.wb_addr = core_addr | I2C_PRER_HI;
	wb_master->wb_send_data(&data_);

	data_.wb_addr = core_addr | I2C_PRER_LO;
	wb_master->wb_read_data(&data_);
	cout << "i2c_drv: I2C_PRER_LO: 0x" << std::hex << data_.data_read[0] << endl;

	data_.wb_addr = core_addr | I2C_PRER_HI;
	wb_master->wb_read_data(&data_);
	cout << "i2c_drv: I2C_PRER_HI: 0x" << std::hex << data_.data_read[0] << endl;

	// enable core
	data_.data_send[0] = I2C_CTR_EN;
	data_.wb_addr = core_addr | I2C_CTR;
	wb_master->wb_send_data(&data_);

	data_.wb_addr = core_addr | I2C_CTR;
	wb_master->wb_read_data(&data_);
	cout << "i2c_drv: I2C_CTR: 0x" << std::hex << data_.data_read[0] << endl;

	data_.wb_addr = core_addr | I2C_SR;
	wb_master->wb_read_data(&data_);
	cout << "i2c_drv: I2C_SR: 0x" << std::hex << data_.data_read[0] << endl;

	i2c_mode_stop = 1;

	return 0;
}

// send data to i2c chip
// err = 0, everything ok
// err != 0, there was error (mostly -EIO)
int i2c_int::int_send_data(struct wb_data* data) {
	
	unsigned int i, err = 0, num_data;

	num_data = data->data_send.size();

	// send address in write mode (write = 0x0, read = 0x1)
	data_.data_send[0] = (data->extra[0] << 1) & 0xFE;
	data_.wb_addr = core_addr | I2C_TXR;
	wb_master->wb_send_data(&data_);

	// start transfer
	data_.data_send[0] = I2C_CR_STA | I2C_CR_WR;
	data_.wb_addr = core_addr | I2C_CR;
	wb_master->wb_send_data(&data_);

	err = i2c_check_transfer(1);
	if (err)
		return err;	

	// send data
	for (i = 0; i < num_data; i++) {

		// write data to transmit register
		data_.data_send[0] = data->data_send[i];
		data_.wb_addr = core_addr | I2C_TXR;
		wb_master->wb_send_data(&data_);

		// if this is last byte, then stop transfer
		if (i == (num_data - 1) && i2c_mode_stop == 1) {// used for repeated start
			data_.data_send[0] = I2C_CR_STO | I2C_CR_WR;
			data_.wb_addr = core_addr | I2C_CR;
			wb_master->wb_send_data(&data_);
		}
		else {
			data_.data_send[0] = I2C_CR_WR;
			data_.wb_addr = core_addr | I2C_CR;
			wb_master->wb_send_data(&data_);
		}

		err = i2c_check_transfer(1);
		if (err)
			return err;

	}

	// transmission done
	//cout << "i2c_drv: i2c send transfer done" << endl;

	return err;
}

int i2c_int::int_read_data(struct wb_data* data) {

	unsigned int i, err = 0, num_data;
	data->data_read.clear();

	num_data = data->extra[1];
	
	// send address in read mode (write = 0x0, read = 0x1)
	data_.data_send[0] = (data->extra[0] << 1)  | 0x1;
	data_.wb_addr = core_addr | I2C_TXR;
	wb_master->wb_send_data(&data_);

	// start transfer
	data_.data_send[0] = I2C_CR_STA | I2C_CR_WR;
	data_.wb_addr = core_addr | I2C_CR;
	wb_master->wb_send_data(&data_);

	err = i2c_check_transfer(1);
	if (err)
		return err;	

	// read data and store it in data_array
	for (i = 0; i < num_data; i++) {

		// if this is last byte, then stop transfer
		if (i == (num_data - 1)) {
			data_.data_send[0] = I2C_CR_STO | I2C_CR_RD | I2C_CR_ACK;
			data_.wb_addr = core_addr | I2C_CR;
			wb_master->wb_send_data(&data_);
		}
		else {
			data_.data_send[0] = I2C_CR_RD | !I2C_CR_ACK;
			data_.wb_addr = core_addr | I2C_CR;
			wb_master->wb_send_data(&data_);
		}

		err = i2c_check_transfer(0);
		if (err)
			return err;

		// store data
		data_.wb_addr = core_addr | I2C_RXR;
		wb_master->wb_read_data(&data_);
		data->data_read.push_back(data_.data_read[0]);

	}
	
	//dev_info(fmc->hwdev, "i2c_drv: i2c read transfer end\n");

	return err;
}

// write reg pointer and then read data
int i2c_int::int_send_read_data(struct wb_data* data) {

	int err = 0; 

	i2c_mode_stop = 0;

	err = int_send_data(data);
	if (err)
		return err;

    err = int_read_data(data);
	if (err)
		return err;

	i2c_mode_stop = 1;	

	// transfer done
	//dev_info(fmc->hwdev, "i2c_drv: i2c write read transfer done\n");

	return err;

}
