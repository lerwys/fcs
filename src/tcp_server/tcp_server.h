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
#include <bsmp/server.h>
}

#if defined( __GNUC__)
#define PACKED __attribute__ ((packed))
#else
#error "Unsupported compiler?"
#endif

using namespace std;

#define BACKLOG 10     // how many pending connections queue will hold

PACKED struct _recv_pkt_t {
  uint8_t data[BSMP_MAX_MESSAGE];
};
typedef struct _recv_pkt_t recv_pkt_t;

PACKED struct _send_pkt_t {
  uint8_t data[BSMP_MAX_MESSAGE];
};
typedef struct _send_pkt_t send_pkt_t;

class tcp_server {
public:

	tcp_server(string port/*, fmc_config_130m_4ch_board *_fmc_config_130m_4ch_board*/);
	~tcp_server();

  void *get_in_addr(struct sockaddr *sa);
  enum bsmp_err register_func (struct bsmp_func *bsmp_func);
  enum bsmp_err register_curve (struct bsmp_curve *bsmp_curve);
  int start(void);

  friend void *tcp_thread (void *arg);
  /* BSMP exported functions */
  //friend uint8_t fmc130m_blink_leds(uint8_t *input, uint8_t *output);

private:

  //int sockfd;
  string port;
  bsmp_server_t *bsmp_server;

  /*fmc_config_130m_4ch_board *_fmc_config_130m_4ch_board;*/

  /* Private functions */
  int tcp_server_handle_client(int s, int *disconnected);
  int bsmp_init (void);
  int sendall(int fd, uint8_t *buf, uint32_t *len);
  int recvall(int fd, uint8_t *buf, uint32_t *len);
  void print_packet (char* pre, uint8_t *data, uint32_t size);
  int bpm_send(int fd, uint8_t *data, uint32_t *count);
  int bpm_recv(int fd, uint8_t *data, uint32_t *count);

  /* BSMP exported functions */
  //uint8_t fmc130m_blink_leds(uint8_t *input, uint8_t *output);
};

#endif // __TCP_SERVER_H
