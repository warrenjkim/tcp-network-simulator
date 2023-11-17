#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "utils.h"

int main() {
    int listen_sockfd, send_sockfd;
    struct sockaddr_in server_addr, client_addr_from, client_addr_to;
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
    if (bind(listen_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
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

    if (!fp) {
        printf("error");
        return 1;
    }

    // TODO: Receive file from the client and save it as output.txt
    struct packet buffer;
    struct packet ack_pkt;
    int expected_seq_num = 0;
    unsigned short ack_num = 0;
    unsigned char ack = 0;
    unsigned char last = 0;

    int recv_len = 0;
    Heap *heap = heap_init();
    struct packet expected;

    
    while (1) {
         recv_len = recvfrom(listen_sockfd, &buffer, sizeof(buffer), MSG_WAITALL, (struct sockaddr *) &client_addr_from, &addr_size);
         print_recv(&buffer);

         if (recv_len <= 0)
             break;

         heap = heap_push(heap, &buffer);

         if (heap->size == WINDOW_SIZE) {
             printf("window full. writing to file\n");
             while (0 < heap->size) {
                 if (expected_seq_num == heap_top(heap).seqnum) {
                     expected = heap_top(heap);
                     expected_seq_num++;
                     fprintf(fp, "%s", heap_top(heap).payload);
                     printf("wrote seqnum: %d\n", expected.seqnum);
                     heap = heap_pop(heap);
                     ack_num = expected_seq_num;
                 }
                 else {
                     ack = 0;
                     ack_num = expected.seqnum; 
                     build_packet(&ack_pkt, expected_seq_num, ack_num, last, ack, 0, "");

                     sendto(send_sockfd, &ack_pkt, sizeof(ack_pkt), 0, (const struct sockaddr *) &client_addr_to, addr_size);
                     print_send(&ack_pkt, last);

                     recv_len = recvfrom(listen_sockfd, &buffer, sizeof(buffer), MSG_WAITALL, (struct sockaddr *) &client_addr_from, &addr_size);
                     print_recv(&buffer);
                 }
             }
         }

         if (ack_num == buffer.seqnum) {
             ack_num++;
             ack = 1;
         }
         else {
             ack = 0;
         }
         build_packet(&ack_pkt, ack_num, ack_num, last, ack, 0, "");
         sendto(send_sockfd, &ack_pkt, sizeof(ack_pkt), 0, (const struct sockaddr *) &client_addr_to, addr_size);
         print_send(&ack_pkt, last);
         printf("\n");
    }


























    // do {
    //     recv_len = recvfrom(listen_sockfd, &buffer, sizeof(buffer), MSG_WAITALL, (struct sockaddr *) &client_addr_from, &addr_size);
    //     print_recv(&buffer);

    //     if (buffer.seqnum == expected_seq_num) {
    //         expected_seq_num = buffer.seqnum + 1;
    //         ack_num = buffer.acknum + 1;
    //         ack = 1;
    //         last = 0;
    //         fprintf(fp, "%s", buffer.payload);
    //     }
    //     else {
    //         ack = 0;
    //         last = 1;
    //     }
    //     
    //     build_packet(&ack_pkt, expected_seq_num, ack_num, last, ack, 0, "");

    //     sendto(send_sockfd, &ack_pkt, sizeof(ack_pkt), 0, (const struct sockaddr *) &client_addr_to, addr_size);
    //     print_send(&ack_pkt, last);
    //     printf("\n");
    // } while (0 < recv_len);

    // while (1) {
    //     recv_len = recvfrom(listen_sockfd, &buffer, sizeof(buffer), MSG_WAITALL, (struct sockaddr *) &client_addr_from, &addr_size);
    //     print_recv(&buffer);
    //     unsigned short ack_num = expected_seq_num;
    //     unsigned char last = 0;
    //     unsigned char ack = 0;

    //     if (recv_len <= 0) {
    //         break;
    //     }

    //     if (buffer.seqnum <= expected_seq_num) {
    //         ack = 1;
    //         expected_seq_num++;
    //         ack_num = expected_seq_num;
    //         fprintf(fp, "%s", buffer.payload);
    //     }
    //     else {
    //         last = 1;
    //         printf("\nbuffer: %d\n", buffer.seqnum);
    //         printf("expected: %d\n\n", expected_seq_num);
    //     }

    //     build_packet(&ack_pkt, expected_seq_num, ack_num, last, ack, 0, "");
    //     sendto(send_sockfd, &ack_pkt, sizeof(ack_pkt), 0, (const struct sockaddr *) &client_addr_to, addr_size);
    //     print_send(&ack_pkt, last);
    // }





    fclose(fp);
    close(listen_sockfd);
    close(send_sockfd);
    return 0;
}
