//============================================================================
// Author      : Andrzej Wojenski
// Version     : 1.0
// Description : Software driver for RS232-Wishbone Master IP core
//============================================================================
#include "rs232_syscon.h"

enum { STATUS_NONE, STATUS_SEND_ERR, STATUS_OK, STATUS_UNKNOWN_CMD_ERR, STATUS_ADDR_ERR, STATUS_DATA_ERR,
  STATUS_Q_ERR, STATUS_TIMEOUT_ERR, STATUS_CMD_ERR, STATUS_ACK_ERR, STATUS_PORT_ERR, STATUS_REPEAT };

enum { TRYB_READ, TRYB_WRITE };

#define MAX_REPEAT 10

#define STATUS_OK 0
#define STATUS_ERR 1

#define MODE_READ 0
#define MODE_WRITE 1

rs232_syscon_driver::rs232_syscon_driver() {

  debug = 0;

  init();
  reset();

  mode = MODE_READ;

}

rs232_syscon_driver::~rs232_syscon_driver() {

  serial_port.Close();
}

int rs232_syscon_driver::reset() {

  cout << "RS232_syscon: Reset function" << endl;

  cout << "RS232_syscon: Setting connection speed..." << endl;
  send_interface((char*)"i\r", &dane_);

  cout << "RS232_syscon: Wishbone component reset..." << endl;
  send_interface((char*)"i\r", &dane_);

  cout << "RS232_syscon: RS-232 initialization is done" << endl;

  init_state = 0;
  debug = 0;

  return STATUS_OK;

}

int rs232_syscon_driver::__reset2() {

  send_interface((char*)"i\r", &dane_);

  init_state = 0;
  debug = 0;

  return 0;
}

int rs232_syscon_driver::wb_rst() {
    return __reset2();
}

int rs232_syscon_driver::init() {

  string rs232_port;

  cout << "RS232_syscon: Init function - WB Master Component" << endl;
  cout << "RS232_syscon: RS-232 interface configuration in progress..." << endl;

  rs232_port = RS232_PORT;

  serial_port.Open(rs232_port.c_str()) ;
  if ( ! serial_port.good() )
  {
    cout << "[" << __FILE__ << ":" << __LINE__ << "] "
        << "RS232_syscon: Error - can't open RS-232 port. Maybe you did not run as sudo?"
        << std::endl ;
    exit(1) ;
  }
  //
  // Set the baud rate of the serial port.
  //
  //serial_port.SetBaudRate( SerialStreamBuf::BAUD_115200 ) ; // not working on my machine
  serial_port.SetBaudRate( SerialStreamBuf::BAUD_57600  ) ;
  if ( ! serial_port.good() )
  {
    cout << "RS232_syscon: Error - can't set transmission speed" << std::endl ;
    exit(1) ;
  }
  //
  // Set the number of data bits.
  //
  serial_port.SetCharSize( SerialStreamBuf::CHAR_SIZE_8 ) ;
  if ( ! serial_port.good() )
  {
    cout << "RS232_syscon: Error - can't set number of bits" << std::endl ;
    exit(1) ;
  }
  //
  // Disable parity.
  //
  serial_port.SetParity( SerialStreamBuf::PARITY_NONE ) ;
  if ( ! serial_port.good() )
  {
    cout << "RS232_syscon: Error - can't set parity bit" << std::endl ;
    exit(1) ;
  }
  //
  // Set the number of stop bits.
  //
  serial_port.SetNumOfStopBits( 1 ) ;
  if ( ! serial_port.good() )
  {
    cout << "RS232_syscon: Error - can't set number of stop bits"
        << std::endl ;
    exit(1) ;
  }
  //
  // Turn off hardware flow control.
  //
  serial_port.SetFlowControl( SerialStreamBuf::FLOW_CONTROL_NONE ) ;
  if ( ! serial_port.good() )
  {
    cout << "RS232_syscon: Error - can't set flow control (handshaking)"
        << std::endl ;
    exit(1) ;
  }

  init_state = 1;

  return STATUS_OK;

}

int rs232_syscon_driver::wb_send_data(struct wb_data* data) {

  ostringstream string_addr, string_data;

  mode = MODE_WRITE;

  string_addr << hex << data->wb_addr;
  string_data << hex << data->data_send[0];

  if (debug == 1)
    cout << "RS232_syscon: Write function: " << endl;

  polecenie.clear();
  polecenie = std::string("w " + string_addr.str() + " " + string_data.str() + "\r");

  if (debug == 1)
    cout << "RS232_syscon: Data sent: " << polecenie;

  send_interface(polecenie, data);

  //debug = 0;

  return STATUS_OK;

}

int rs232_syscon_driver::wb_read_data(struct wb_data* data) {

  string txt;
  ostringstream string_addr;

  mode = MODE_READ;
  data->data_read.clear();

  string_addr << hex << data->wb_addr;

  if (debug == 1)
    cout << "RS232_syscon: Read function: " << endl;

  polecenie.clear();
  polecenie = std::string("r " + string_addr.str() + "\r");

  txt = polecenie;
  std::replace(txt.begin(), txt.end(), '\r', ' ');
  //cout << txt << flush;

  if (debug == 1)
    cout << "Data sent on Wishbone bus: " << txt;

  send_interface(polecenie, data);

  //debug = 0;

  return STATUS_OK;

}


int rs232_syscon_driver::send_interface(string polecenie, struct wb_data* data) {

  static int i_repeat = 0;

  serial_port.write(polecenie.c_str(), polecenie.size());

  usleep(10000);//usleep(50000);
  //sleep(1);

  if (serial_port.bad()) {
    cout << "RS232_syscon: write() failed!" << endl;
    cout << "Check connection and restart application" << endl;

    data->status = STATUS_SEND_ERR;
    exit(1);
  }


  // Status read (RS232_syscon: OK, ERR itp)
  while (read_interface(data) == STATUS_REPEAT) {

    if (i_repeat >= MAX_REPEAT) {
      cout << endl << "RS232_syscon: Exceeded maximum number of read requests to Wishbone Master driver" << endl <<
          "RS232_syscon: Communication lost (no communication)..." << endl <<
          "Application exits" << endl;
      exit(1);
    }

    cout << endl << "RS232_syscon: Error while reading data from Wishbone bus, repeating request...";
    i_repeat++;
    send_interface(polecenie, data);
  }

  i_repeat = 0;


  return 0;
}

int rs232_syscon_driver::read_interface(struct wb_data* data) {

  string dane_temp;
  char next_byte;
  string data_str;
  string data_read;
  size_t data_pos;
  string data_status;
  unsigned int i;
  stringstream str_addr;
  uint32_t data_int;

  data_str.clear();

  while( serial_port.rdbuf()->in_avail() > 0  )
  {
    serial_port.get(next_byte);
    data_str += next_byte;

    //std::cout << std::hex << static_cast<int>(next_byte) << " ";
    usleep(500);
  }

  if (init_state == 1)
    return 0;

  // Wskaznik na poczatek wlasciwych danych
  if ((data_pos = data_str.find("\r\r\n")) == string::npos)
    return STATUS_REPEAT;

  data_read.clear();
  data_read.assign(data_str, data_pos + 3, data_str.size() - data_pos);

  if (mode == MODE_READ) {
    // odczyt danych
    dane_temp.clear();

    str_addr.clear();
    str_addr.str("");
    str_addr << hex << data->wb_addr;
    //sprintf(addr, "%x", data->wb_addr);
    dane_temp = str_addr.str();

    for (i = 0; i < dane_temp.size(); i++)
      dane_temp[i] = toupper((int)dane_temp[i]);

    dane_temp += " : ";

    if ((data_pos = data_read.find(dane_temp)) == string::npos)
        return STATUS_REPEAT;

    data_read.erase(0 , data_pos + dane_temp.size()); // erase beginning of string with address
    // erase end of string with status
    data_read.erase(8);
    //data->data_read.push_back(data_read);
    //istringstream string_data(data_read);
    //string_data >> dec >> data_int;
    //sscanf(data_read.c_str(), "%x", &data_int);
    data_int = strtoul(data_read.c_str(), NULL, 16);

    data->data_read.push_back(data_int);


    if (debug == 1)
      cout << "RS232_syscon: Data read from Wishbone bus: " << data_read << endl;
  }

  // Check if no error
  //err = strstr(buffer, "OK");
  if (data_str.find("OK") != string::npos) {

    if (data != NULL)
      data->status = STATUS_OK;

    if (debug == 1)
      cout << "RS232_syscon: Correct data transfer" << endl;

    return 0;
  }

  //err = strstr(buffer, "C?");
  if (data_str.find("C?") != string::npos) {

    if (data != NULL)
      data->status = STATUS_UNKNOWN_CMD_ERR;

    cout << "RS232_syscon: Unknown command" << endl;

    return 1;
  }

  //err = strstr(buffer, "A?");
  if (data_str.find("A?") != string::npos) {

    if (data != NULL)
      data->status = STATUS_ADDR_ERR;

    cout << "RS232_syscon: Invalid address field" << endl;

    return 1;
  }

  //err = strstr(buffer, "D?");
  if (data_str.find("D?") != string::npos) {

    if (data != NULL)
      data->status = STATUS_DATA_ERR;

    cout << "RS232_syscon: Invalid data field" << endl;

    return 1;
  }

  //err = strstr(buffer, "Q?");
  if (data_str.find("Q?") != string::npos) {

    if (data != NULL)
      data->status = STATUS_Q_ERR;

    cout << "RS232_syscon: Invalid number of data field" << endl;

    return 1;
  }

  //err = strstr(buffer, "B!");
  if (data_str.find("B!") != string::npos) {

    if (data != NULL)
      data->status = STATUS_TIMEOUT_ERR;

    cout << "RS232_syscon: Timeout error (no access to Wishbone bus)" << endl;

    return 1;
  }

  //err = strstr(buffer, "?");
  if (data_str.find("?") != string::npos) {

    if (data != NULL)
      data->status = STATUS_CMD_ERR;

    cout << "RS232_syscon: Command which was sent was too long" << endl;

    return 1;
  }

  //err = strstr(buffer, "!");
  if (data_str.find("!") != string::npos) {

    if (data != NULL)
      data->status = STATUS_ACK_ERR;

    cout << "RS232_syscon: Error (err_i) or watchdog alert (no ack_i) - could be caused by invalid IP core address" << endl;

    return 1;
  }

  return 0;
}
