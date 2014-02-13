//============================================================================
// Author      : Lucas Russo
// Version     : 1.0
// Description : 
//============================================================================
#ifndef __TCP_SERVER_H
#define __TCP_SERVER_H

#include <iostream>
#include <string>
#include <iomanip>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#include "fmc_config/fmc_config_130m_4ch_board.h"

extern "C" {
#include <sllp/server.h>
}

#if defined( __GNUC__)
#define PACKED __attribute__ ((packed))
#else
#error "Unsupported compiler?"
#endif

using namespace std;

#define BACKLOG 10     // how many pending connections queue will hold

PACKED struct _recv_pkt_t {
  uint8_t data[SLLP_MAX_MESSAGE];
};
typedef struct _recv_pkt_t recv_pkt_t;

PACKED struct _send_pkt_t {
  uint8_t data[SLLP_MAX_MESSAGE];
};
typedef struct _send_pkt_t send_pkt_t;

class tcp_server {
public:

	tcp_server(string port);
	~tcp_server();

  void *get_in_addr(struct sockaddr *sa);
  int start(void); 

private:

	//fmc_config_130m_4ch_board* fmc_config_130m_4ch_board;
  //int sockfd;
  string port;
  sllp_server_t *sllp_server;

  /* Private functions */
  int tcp_server_handle_client(int s, int *disconnected);
  int sllp_init (void);
  int sendall(int fd, uint8_t *buf, uint32_t *len);
  int recvall(int fd, uint8_t *buf, uint32_t *len);
  void print_packet (char* pre, uint8_t *data, uint32_t size);
  int bpm_send(int fd, uint8_t *data, uint32_t *count);
  int bpm_recv(int fd, uint8_t *data, uint32_t *count);

};

#endif // __TCP_SERVER_H
