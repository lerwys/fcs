//============================================================================
// Author      : Andrzej Wojenski
// Version     : 1.0
// Description : Virtual class for communication interfaces
//============================================================================

#ifndef WBINT_DRV_H_
#define WBINT_DRV_H_

#include <vector>
#include <string>

using namespace std;

class WBInt_drv {
public:
	WBInt_drv() {};
	virtual ~WBInt_drv() {};

	virtual int int_reg(WBMaster_unit* wb_master, uint32_t core_addr) =0;

	virtual int int_send_data(struct wb_data* data) =0;
	virtual int int_read_data(struct wb_data* data) =0;
	virtual int int_send_read_data(struct wb_data* data) =0;

};

#endif /* WBINT_DRV_H_ */
