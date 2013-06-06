//============================================================================
// Author      : Andrzej Wojenski
// Version     : 1.0
// Description : Virtual class for Wishbone master software driver
//============================================================================

#ifndef WBMASTER_UNIT_H_
#define WBMASTER_UNIT_H_

#include <vector>
#include <string>

using namespace std;

class WBMaster_unit {
public:
	WBMaster_unit() {};
	virtual ~WBMaster_unit() {};

	virtual int wb_send_data(struct wb_data* data) =0;
	virtual int wb_read_data(struct wb_data* data) =0;

};

#endif /* WBMASTER_UNIT_H_ */
