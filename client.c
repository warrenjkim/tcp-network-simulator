#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "utils.h"

int main(int argc, char *argv[]) {
  int listen_sockfd, send_sockfd;
  struct sockaddr_in client_addr, server_addr_to, server_addr_from;
  socklen_t addr_size = sizeof(server_addr_to);
  struct timeval tv;

  tv.tv_sec = 0;
  tv.tv_usec = TIMEOUT * 100000;

  // read filename from command line argument
  if (argc != 2) {
    printf("Usage: ./client <filename>\n");
    return 1;
  }
  char *filename = argv[1];

  // Create a UDP socket for listening
  listen_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (listen_sockfd < 0) {
    perror("Could not create listen socket");
    return 1;
  }

  // receive timeout
  if (setsockopt(listen_sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
    perror("couldn't set receive timeout");
    return 1;
  }

  // Create a UDP socket for sending
  send_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (send_sockfd < 0) {
    perror("Could not create send socket");
    return 1;
  }

  // Configure the server address structure to which we will send data
  memset(&server_addr_to, 0, sizeof(server_addr_to));
  server_addr_to.sin_family = AF_INET;
  server_addr_to.sin_port = htons(SERVER_PORT_TO);
  server_addr_to.sin_addr.s_addr = inet_addr(SERVER_IP);

  // Configure the client address structure
  memset(&client_addr, 0, sizeof(client_addr));
  client_addr.sin_family = AF_INET;
  client_addr.sin_port = htons(CLIENT_PORT);
  client_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  // Bind the listen socket to the client address
  if (bind(listen_sockfd, (struct sockaddr *)&client_addr,
           sizeof(client_addr)) < 0) {
    perror("Bind failed");
    close(listen_sockfd);
    return 1;
  }

  // Open file for reading
  FILE *fp = fopen(filename, "rb");
  if (fp == NULL) {
    perror("Error opening file");
    close(listen_sockfd);
    close(send_sockfd);
    return 1;
  }

  // TODO: Read from file, and initiate reliable data transfer to the server

  struct packet pkt;
  struct packet ack_pkt;
  char buffer[PAYLOAD_SIZE];
  unsigned short seq_num = 0;
  unsigned short ack_num = 0;
  char last = 0;
  char ack = 0;

  size_t bytes_read = 0;

  ack_pkt.seqnum = (unsigned short)(-1);
  ack_pkt.acknum = (unsigned short)(-1);

  State state = SLOW_START;

  // fast retransmit
  int count = 0;
  struct packet dup_pkt;

  // window growth
  size_t mss = 0;
  size_t ssthresh = (unsigned short)(-1);

  Queue *cwnd = queue_init();

  ssize_t timeout = 0;

  while (1) {
    if (ack_pkt.last)
      break;

    if (ack_pkt.acknum != (unsigned short)(-1)) {
      if (dup_pkt.acknum == ack_pkt.acknum) {
        count += mss;
      } else {
        mss = queue_pop(cwnd, ack_pkt.acknum);
        dup_pkt = ack_pkt;
        count = 0;
      }
    }

    if (state == SLOW_START) {
      cwnd->max_size += mss;

      if (ssthresh < cwnd->max_size)
        state = CONGESTION_AVOIDANCE;
    }

    else if (state == CONGESTION_AVOIDANCE) {
      cwnd->max_size += ((double)(MSS) / (size_t)(cwnd->max_size));
    }

    else if (state == FAST_RECOVERY) {
      if (dup_pkt.seqnum != ack_pkt.seqnum ||
          dup_pkt.acknum != ack_pkt.acknum) {
        cwnd->max_size = ssthresh;
        state = CONGESTION_AVOIDANCE;
      } else
        cwnd->max_size += MSS;
    }

    if (count == 3) {
      ssthresh = ((size_t)(cwnd->max_size / 2) < (2 * MSS))
                     ? (2 * MSS)
                     : (size_t)(cwnd->max_size / 2);
      cwnd->max_size = ssthresh + (3 * MSS);
      state = FAST_RECOVERY;
      count = 0;

      struct packet *resend = queue_get(cwnd, dup_pkt.acknum);

      sendto(send_sockfd, resend, sizeof(*resend), MSG_CONFIRM,
             (const struct sockaddr *)&server_addr_to, addr_size);
    }

    if (timeout < 0) {
      ssthresh =
          ((size_t)(cwnd->max_size / 2) < 2) ? 2 : (size_t)(cwnd->max_size / 2);
      cwnd->max_size = 1 * MSS;

      state = SLOW_START;

      struct packet *resend = queue_top(cwnd);
      sendto(send_sockfd, resend, sizeof(*resend), MSG_CONFIRM,
             (const struct sockaddr *)&server_addr_to, addr_size);
    }

    while (!queue_full(cwnd) && !last) {
      bytes_read = fread(buffer, sizeof(char), PAYLOAD_SIZE, fp);

      if (bytes_read < PAYLOAD_SIZE)
        last = 1;

      build_packet(&pkt, seq_num, ack_num, last, ack, bytes_read, buffer);
      seq_num += 1;
      ack_num += 1;

      sendto(send_sockfd, &pkt, sizeof(pkt), MSG_CONFIRM,
             (const struct sockaddr *)&server_addr_to, addr_size);

      cwnd = queue_push(cwnd, &pkt);
      // usleep(5);
    }

    timeout = recvfrom(listen_sockfd, &ack_pkt, sizeof(ack_pkt), MSG_WAITALL,
                       (struct sockaddr *)&server_addr_from, &addr_size);

    if (ack_pkt.last)
      break;
  }

  fclose(fp);
  close(listen_sockfd);
  close(send_sockfd);
  return 0;
}
