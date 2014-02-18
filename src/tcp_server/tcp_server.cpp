//============================================================================
// Author      : Lucas Russo
// Version     : 1.0
// Description : based on http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#simpleserver
//============================================================================
#include <pthread.h>

#include "tcp_server.h"
#include "debug.h"

#define S "bsmp_server: "

#define TRY(name, func)\
    do {\
        enum bsmp_err err = func;\
        if(err) {\
            fprintf(stderr, S name": %s\n", bsmp_error_str(err));\
            return -1;\
        }\
    }while(0)

#define PACKET_SIZE             BSMP_MAX_MESSAGE
#define PACKET_HEADER           BSMP_HEADER_SIZE

/* Our socket */
int sockfd;

/* Our receive packet */
recv_pkt_t recv_pkt;
send_pkt_t send_pkt;

struct bsmp_raw_packet recv_packet = {.data = recv_pkt.data };
struct bsmp_raw_packet send_packet = {.data = send_pkt.data };

tcp_server::tcp_server(string port/*, fmc_config_130m_4ch_board *_fmc_config_130m_4ch_board*/)
{
  this->port = port;
  //this->_fmc_config_130m_4ch_board = _fmc_config_130m_4ch_board;

  bsmp_init();
}

tcp_server::~tcp_server() {
 // Null
}

// get sockaddr, IPv4 or IPv6:
void *tcp_server::get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/***************************************************************/
/********************** Utility functions **********************/
/***************************************************************/

void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

int tcp_server::sendall(int fd, uint8_t *buf, uint32_t *len)
{
    uint32_t total = 0;        // how many bytes we've sent
    uint32_t bytesleft = *len; // how many we have left to send
    uint32_t n;

    while(total < *len) {
        n = send(fd, (char *)buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    *len = total; // return number actually sent here

    return n==-1?-1:0; // return -1 on failure, 0 on success
}

int tcp_server::recvall(int fd, uint8_t *buf, uint32_t *len)
{
    uint32_t total = 0;        // how many bytes we've recv
    uint32_t bytesleft = *len; // how many we have left to recv
    uint32_t n;

    while(total < *len) {
        n = recv(fd, (char *)buf+total, bytesleft, 0);
        if (n == -1 || n == 0) { break; }
        total += n;
        bytesleft -= n;
    }

    *len = total; // return number actually sent here

    return n==-1?-1:0; // return -1 on failure, 0 on success
}

void tcp_server::print_packet (char* pre, uint8_t *data, uint32_t size)
{
#ifdef DEBUG
    printf("%s: [", pre);

    if(size < 32)
    {
        unsigned int i;
        for(i = 0; i < size; ++i)
            printf("%02X ", data[i]);
        printf("]\n");
    }
    else
        printf("%d bytes ]\n", size);
#endif
}

int tcp_server::bpm_send(int fd, uint8_t *data, uint32_t *count)
{
    uint8_t  packet[BSMP_MAX_MESSAGE];
    uint32_t packet_size = *count;
    uint32_t len = *count;

    memcpy (packet, data, *count);

    print_packet("SEND", packet, packet_size);

    int ret = sendall(fd, packet, &len);
    if(len != packet_size) {
        if(ret < 0)
            perror("send");
        return -1;
    }

    return 0;
}

int tcp_server::bpm_recv(int fd, uint8_t *data, uint32_t *count)
{
    uint8_t packet[PACKET_SIZE] = {0};
    uint32_t packet_size;
    uint32_t len = PACKET_HEADER;

    int ret = recvall(fd, packet, &len);

    if(ret < 0) {
      perror("recv");
      return -1;
    }

    // Client disconnected
    if (len == 0) {
       *count = 0;
       return 0;
    }

    if(len != PACKET_HEADER) {
       return -1;
    }

    uint32_t remaining = (packet[1] << 8) + packet[2];
    len = remaining;

    //printf ("bpm_recv: remaining bytes = %d\n", len);

    ret = recvall(fd, packet + PACKET_HEADER, &len);
    if(len != remaining) {
        if(ret < 0)
          perror("recv");
        return -1;
    }

    packet_size = PACKET_HEADER + remaining;

    print_packet("RECV", packet, packet_size);

    *count = packet_size;
    memcpy(data, packet, *count);

    return 0;
}

int tcp_server::tcp_server_handle_client(int s, int *disconnected)
{
  uint32_t bytes_recv;
  uint32_t bytes_sent;
  int ret;

  *disconnected = 0;

  /* receive packet */
  ret = bpm_recv(s, (uint8_t *)&recv_pkt, &bytes_recv);
  if (ret < 0) {
    fprintf(stderr, "recv err\n");
    return -1;
  }
  // Client disconnected
  if (bytes_recv == 0) {
    *disconnected = 1;
    return 0;
  }

   //uint32_t len_recv = PACKET_HEADER + (recv_packet.data[2] << 8) + recv_packet.data[3];
   uint32_t len_recv = PACKET_HEADER + (recv_packet.data[1] << 8) + recv_packet.data[2];
   //fprintf(stderr, "bytes received check = %d\n", len_recv);
   //print_packet("RECV_CHECK", recv_packet.data, len_recv);

  /* Pass it to bsmp */
  recv_packet.len = len_recv;
  bsmp_process_packet(this->bsmp_server, &recv_packet, &send_packet);

  /* Calculate how many bytes to send */
   uint32_t len = send_packet.len;
  //fprintf(stderr, "bytes sent check = %d\n", len);
  //print_packet("SEND_CHECK", send_packet.data, len);

  if ((ret = bpm_send(s, (uint8_t *)&send_pkt, &len)) < 0) {
    fprintf(stderr, "recv err\n");
    return -1;
  }

  //if (bytes_sent = send(s, (char *)&send_pkt, len, 0) == -1) {
  //  fprintf(stderr, "send err\n");
  //  return -1;
  //}

  //fprintf(stderr, "bytes sent = %d\n", bytes_sent);

  return 0;
}

int tcp_server::bsmp_init (void)
{
    /*
     * This call malloc's a new server instance. If it returns NULL, the
     * allocation failed. Probably there's not enough memory.
     */
    bsmp_server = bsmp_server_new();

    if(!bsmp_server)
    {
        fprintf(stderr, S"Couldn't allocate a BSMP Server instance\n");
        return -1;
    }

    /*
     * Register all Functions
     */
    //TRY("reg_func", bsmp_register_function(bsmp_server, &ad_convert_func));  // ID 0
    //TRY("reg_func", bsmp_register_function(bsmp_server, &fmc130m_blink_leds_func));  // ID 1

    /*
     * Great! Now our server is up and ready to receive some commands.
     * This will be done in the function server_process_message.
     */
    fprintf(stdout, S"Initialized!\n");
    return 0;
}

typedef struct _tcp_server_hdr_t {
    int fd;
    tcp_server *server;
} tcp_server_hdr_t;

/* Thread loop */
void *tcp_thread (void *arg)
{
    tcp_server_hdr_t *tcp_server_hdr =
        (tcp_server_hdr_t *) arg;

    int fd = tcp_server_hdr->fd;
    tcp_server * tcp_server = tcp_server_hdr->server;
    int disconnected;
    int ret;

    while (1) {
        ret = tcp_server->tcp_server_handle_client(fd, &disconnected);

        if (ret == -1) {
            fprintf(stderr, "server: failed to handle client\n");
            break;
        }

        // Client disconnected
        if (disconnected) {
            fprintf(stderr, "server: client disconnected\n");
            break;
        }
    }

    pthread_exit(&ret);
    return NULL;
}

/***************************************************************/
/**********************   Class methods  **********************/
/***************************************************************/

int tcp_server::register_func (struct bsmp_func *bsmp_func)
{
  return bsmp_register_function(bsmp_server, bsmp_func);
}

int tcp_server::register_curve (struct bsmp_curve *bsmp_curve)
{
  return bsmp_register_curve(bsmp_server, bsmp_curve);
}

int tcp_server::start(void)
{
  int new_fd;  // listen on sock_fd, new connection on new_fd
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_storage their_addr; // connector's address information
  socklen_t sin_size;
  struct sigaction sa;
  int yes=1;
  char s[INET6_ADDRSTRLEN];
  int rv;
  tcp_server_hdr_t tcp_server_hdr;
  pthread_t tid;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE; // use my IP

  if ((rv = getaddrinfo(NULL, this->port.c_str(), &hints, &servinfo)) != 0) {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
      return -1;
  }

  // loop through all the results and bind to the first we can
  for(p = servinfo; p != NULL; p = p->ai_next) {
      if ((sockfd = socket(p->ai_family, p->ai_socktype,
              p->ai_protocol)) == -1) {
          perror("server: socket");
          continue;
      }

      if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
              sizeof(int)) == -1) {
          perror("setsockopt");
          exit(1);
      }

      /* This is important for correct behaviour */
      if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &yes,
              sizeof(int)) == -1) {
          perror("setsockopt");
          exit(1);
      }

      if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
          close(sockfd);
          perror("server: bind");
          continue;
      }

      break;
  }

  if (p == NULL)  {
      fprintf(stderr, "server: failed to bind\n");
      return -2;
  }

  freeaddrinfo(servinfo); // all done with this structure

  if (listen(sockfd, BACKLOG) == -1) {
      perror("listen");
      exit(1);
  }

  sa.sa_handler = sigchld_handler; // reap all dead processes
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
      perror("sigaction");
      exit(1);
  }

  printf("server: waiting for connections...\n");

  while(1) {  // main accept() loop
      sin_size = sizeof their_addr;
      new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
      if (new_fd == -1) {
          perror("accept");
          continue;
      }

      inet_ntop(their_addr.ss_family,
          get_in_addr((struct sockaddr *)&their_addr),
          s, sizeof s);
      printf("server: got connection from %s\n", s);

      tcp_server_hdr.fd = new_fd;
      tcp_server_hdr.server = this;

      // Create thread
      if (pthread_create (&tid, NULL, tcp_thread, (void *)&tcp_server_hdr) != 0) {
          fprintf(stderr, "server: error creating tcp thread\n");
      }

      /*if (!fork()) { // this is the child process
          close(sockfd); // child doesn't need the listener

          // Receive packet
          //if (send(new_fd, "Hello, world!", 13, 0) == -1) {
          //    perror("send");
          //}
          while (1) {
            int disconnected;
            int ret = tcp_server_handle_client(new_fd, &disconnected);
            if (ret == -1) {
                fprintf(stderr, "server: failed to handle client\n");
                return -3;
            }

            // Client disconnected
            if (disconnected) {
              fprintf(stderr, "server: client disconnected\n");
              break;
            }
          }

          close(new_fd);
          exit(0);
      }*/
      //close(new_fd);  // parent doesn't need this
  }

  return 0;
}

/*******************************************************************************
 * tcp_server functions
 * *****************************************************************************/
