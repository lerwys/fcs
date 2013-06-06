//============================================================================
// Author      : Andrzej Wojenski
// Version     : 1.0
// Description : Software driver for Wishbone SPI_BIDIR_1_0 IP core
//               (modification of SPI Master Controller from OpenCores
//                with modification to handle bidirectional transfer on MOSI line)
//============================================================================
#include "spi.h"

enum { MODE_WRITE, MODE_READ, MODE_WRITE_READ };

#define MAX_REPEAT 10
#define REGS_ADDR_MULTIPLY 4

spi_int::spi_int() {

	//cout << showbase << internal << setfill('0') << setw(8);

}

int spi_int::int_reg(WBMaster_unit* wb_master, uint32_t core_addr) {

	this->wb_master = wb_master;
    this->core_addr = core_addr;
    data_.data_send.resize(1);
    data_.data_read.clear();

    return 0;

}

int spi_int::spi_init(int sys_freq, int spi_freq, int config)
{
	/* set frequency of SPI to SPI_BIDIR_FREQ (from SYS_FREQ) */
	float f_freq = (float)sys_freq/(2.0 * (float)spi_freq) - 1.0;
	uint16_t freq = f_freq;

	//cout << "freq: " << f_freq << " " << freq << " " << hex << freq << endl;

	data_.data_send[0] = freq;
	data_.wb_addr = core_addr | SPI_BIDIR_DIVIDER;
	wb_master->wb_send_data(&data_);

	data_.wb_addr = core_addr | SPI_BIDIR_DIVIDER;
	wb_master->wb_read_data(&data_);

	cout << "spi_drv: spi_divider: 0x" << hex << data_.data_read[0] << endl;

	// write config data
	data_.data_send[0] = config;
	data_.wb_addr = core_addr | SPI_BIDIR_CTRL;
	wb_master->wb_send_data(&data_);

	data_.wb_addr = core_addr | SPI_BIDIR_CTRL;
	wb_master->wb_read_data(&data_);

	cout << "spi_drv: spi_ctrl: 0x" << hex << data_.data_read[0] << endl;

	// turn off bidir mode
	data_.data_send[0] = 0x00;
	data_.wb_addr = core_addr | SPI_BIDIR_CFG_BIDIR;
	wb_master->wb_send_data(&data_);

	return 0;
}

int spi_int::spi_transfer(int mode, struct wb_data* data) {

	unsigned int i, err, repeat = 0;
	data->data_read.clear();

	data_.wb_addr = core_addr | SPI_BIDIR_CTRL;
	wb_master->wb_read_data(&data_);
	uint32_t config = data_.data_read[0];

//cout << "cfg: " << hex << config << endl;

	// chip_addr - SS line
	data_.data_send[0] = data->extra[0];
	data_.wb_addr = core_addr | SPI_BIDIR_SS;
	wb_master->wb_send_data(&data_);

	if (data->extra[1] > 0x7F) {
		cout << "spi_drv: spi char len error" << endl;
		return 1;
	}

	//cout << "data send spi1: " << hex << data->data_send[0] << endl;
	//cout << "size " << data->data_send.size() << endl;

	// write data to TX regs
	if (mode == MODE_WRITE || mode == MODE_WRITE_READ) {
		for (i = 0; i < data->data_send.size(); i++) {
			//cout << "data send spi: " << hex << data->data_send[i] << endl;
			data_.data_send[0] = data->data_send[i];
			data_.wb_addr = core_addr | (SPI_BIDIR_TX0 + REGS_ADDR_MULTIPLY*i); // i or 4*i, depending on address space (0,1,2,3 or 0,4,8,c)
			wb_master->wb_send_data(&data_);
		}
	}

	// char_len is 7 bit
	// config already done	
	config &= 0xFFFFFF80;
	//cout << "cfg num " << hex << (config | (uint8_t)data->extra[1]) << endl;
	data_.data_send[0] = config | (uint8_t)data->extra[1];
	data_.wb_addr = core_addr | SPI_BIDIR_CTRL;
	wb_master->wb_send_data(&data_);

	// transfer start
	data_.wb_addr = core_addr | SPI_BIDIR_CTRL;
	wb_master->wb_read_data(&data_);
	config = data_.data_read[0];


	data_.data_send[0] = config | SPI_BIDIR_CTRL_GO_BSY;

	//cout << "cfg start " << hex << data_.data_send[0] << endl;

	data_.wb_addr = core_addr | SPI_BIDIR_CTRL;
	wb_master->wb_send_data(&data_);

	// check if done (GO == 0)
	while(1) {

		data_.wb_addr = core_addr | SPI_BIDIR_CTRL;
		wb_master->wb_read_data(&data_);

		if ( (data_.data_read[0] & SPI_BIDIR_CTRL_GO_BSY) == 0)
			break;

		usleep(1000);
		repeat++;

		if (repeat > MAX_REPEAT) {
			// write status register
			cout << "spi_drv: spi write transfer error (timeout)" << endl;
			err = 1;
			return err;
		}
	}

	//cout << "spi_drv: spi transfer done" << endl;
	data_.data_read.clear();
	data->data_read.clear();
	// get data
	for (i = 0; i < 4; i++) {
		//data_.wb_addr = core_addr | (SPI_BIDIR_RX0 + REGS_ADDR_MULTIPLY*i); // i or 4*i, depending on address space (0,1,2,3 or 0,4,8,c)
		data_.wb_addr = core_addr | (SPI_RX_MISO_0 + REGS_ADDR_MULTIPLY*i); // i or 4*i, depending on address space (0,1,2,3 or 0,4,8,c)
		wb_master->wb_read_data(&data_);
		data->data_read.push_back(data_.data_read[0]);
		//cout << "data spi: 0x" << hex << data_.data_read[0] << endl;
	}

	return 0;

}

int spi_int::int_send_data(struct wb_data* data) {

	return spi_transfer(MODE_WRITE, data);

}
int spi_int::int_read_data(struct wb_data* data) {

	return spi_transfer(MODE_READ, data);

}

int spi_int::int_send_read_data(struct wb_data* data) {

	return spi_transfer(MODE_WRITE_READ, data);

}
