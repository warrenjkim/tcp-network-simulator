#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "utils.h"

int main() {
  int listen_sockfd, send_sockfd;
  struct sockaddr_in server_addr, client_addr_from, client_addr_to;
  struct packet buffer;
  socklen_t addr_size = sizeof(client_addr_from);

  // Create a UDP socket for sending
  send_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (send_sockfd < 0) {
    perror("Could not create send socket");
    return 1;
  }

  // Create a UDP socket for listening
  listen_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (listen_sockfd < 0) {
    perror("Could not create listen socket");
    return 1;
  }

  // Configure the server address structure
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(SERVER_PORT);
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  // Bind the listen socket to the server address
  if (bind(listen_sockfd, (struct sockaddr *)&server_addr,
           sizeof(server_addr)) < 0) {
    perror("Bind failed");
    close(listen_sockfd);
    return 1;
  }

  // Configure the client address structure to which we will send ACKs
  memset(&client_addr_to, 0, sizeof(client_addr_to));
  client_addr_to.sin_family = AF_INET;
  client_addr_to.sin_addr.s_addr = inet_addr(LOCAL_HOST);
  client_addr_to.sin_port = htons(CLIENT_PORT_TO);

  // Open the target file for writing (always write to output.txt)
  FILE *fp = fopen("output.txt", "wb");

  // TODO: Receive file from the client and save it as output.txt

  unsigned short expected_seq_num = 0;
  int recv_len;
  struct packet ack_pkt;

  unsigned short seq_num = 0;
  unsigned short ack_num = 0;
  unsigned short last_seqnum = -1;
  char last = 0;
  char ack = 1;

  Heap *heap = heap_init();

  while (1) {
    recv_len = recvfrom(listen_sockfd, &buffer, sizeof(buffer), MSG_WAITALL,
                        (struct sockaddr *)&client_addr_from, &addr_size);
    // print_recv(&buffer);

    if (buffer.last)
      last_seqnum = buffer.seqnum;

    seq_num = buffer.seqnum < seq_num ? seq_num : buffer.seqnum;

    if (expected_seq_num <= buffer.seqnum)
      heap = heap_push(heap, &buffer);

    // printf("heap size: %ld, ", heap->size);
    // printf("heap capacity: %ld\n", heap->capacity);

    struct packet min_pkt = heap_top(heap);
    // for (size_t i = 0; i < heap->size; i++) {
    //     printf("%d ", heap->data[i].seqnum);
    // }
    // printf("\n");
    while (0 < heap->size && (expected_seq_num == min_pkt.seqnum)) {
      // printf("heap size %ld\n", heap->size);
      expected_seq_num++;
        size_t bytes_written =
          fwrite(min_pkt.payload, sizeof(char), min_pkt.length, fp);
      // printf("\nwrote seqnum %d", min_pkt.seqnum);
      heap = heap_pop(heap);
      min_pkt = heap_top(heap);
    }

    ack_num = expected_seq_num;

    if (last_seqnum <= expected_seq_num)
      last = 1;

    build_packet(&ack_pkt, seq_num, ack_num, last, ack, 0, "");
    sendto(send_sockfd, &ack_pkt, sizeof(ack_pkt), MSG_CONFIRM,
           (const struct sockaddr *)&client_addr_to, addr_size);
    // printf("\n");
    // print_send(&ack_pkt, 0);

    if (last)
      break;

    // printf("\n");
  }

  fclose(fp);
  close(listen_sockfd);
  close(send_sockfd);
  return 0;
}
