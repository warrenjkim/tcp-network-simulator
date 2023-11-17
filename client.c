#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>

#include "utils.h"

#define MSG_CONFIRM 0x800

int main(int argc, char *argv[]) {
    int listen_sockfd, send_sockfd;
    struct sockaddr_in client_addr, server_addr_to, server_addr_from;
    socklen_t addr_size = sizeof(server_addr_to);
    struct timeval tv;
    tv.tv_sec = TIMEOUT;
    tv.tv_usec = 0;


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

   if (setsockopt(listen_sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0) {
       perror("couldn't set send timeout");
       return 1;
   }

    // Configure the server address structure to which we will send data
    memset(&server_addr_to, 0, sizeof(server_addr_to));
    server_addr_to.sin_family = AF_INET;
    server_addr_to.sin_port = htons(SERVER_PORT_TO); // 5002
    server_addr_to.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Configure the client address structure
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(CLIENT_PORT); // 6001
    client_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind the listen socket to the client address
    if (bind(listen_sockfd, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
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

    ssize_t bytes_read = 0;

    CQueue *queue = cqueue_init();
    ack_pkt.seqnum = -1;
    ack_pkt.acknum = -1;
    ack_pkt.last = 0;
    

    while (1) {
        if (ack_pkt.last)
            break;
        if (ack_pkt.acknum != (unsigned short)(-1))
            queue = cqueue_pop(queue, ack_pkt.acknum);

        if (queue->size < WINDOW_SIZE) {
            bytes_read = fread(buffer, sizeof(char), PAYLOAD_SIZE, fp);
            if (bytes_read < PAYLOAD_SIZE)
                last = 1;
            printf("bytes read: %ld\n", bytes_read);
            build_packet(&pkt, seq_num++, ack_num++, last, ack, bytes_read, buffer);
            sendto(send_sockfd, &pkt, sizeof(pkt), MSG_CONFIRM, (const struct sockaddr *) &server_addr_to, addr_size);
            queue = cqueue_push(queue, &pkt);
            print_send(&pkt, ack);
        }
        else {
            struct packet *resend = cqueue_get(queue, ack_pkt.acknum);
            if (!resend)
                continue;
            sendto(send_sockfd, resend, sizeof(*resend), MSG_CONFIRM, (const struct sockaddr *) &server_addr_to, addr_size);
            print_send(resend, last);
        }

        recvfrom(listen_sockfd, &ack_pkt, sizeof(ack_pkt), MSG_WAITALL, (struct sockaddr *) &server_addr_from, &addr_size);
        print_recv(&ack_pkt);
        printf("\n");
    }









    // int i = 0;


    // while (1) {
    //     if ((ack_pkt.ack && i < WINDOW_SIZE) || seq_num == 0) {
    //         bytes_read = fread(buffer, sizeof(char), PAYLOAD_SIZE, fp);
    //         build_packet(&pkt, seq_num, ack_num, last, ack, sizeof(buffer) - 1, buffer);

    //         sendto(send_sockfd, &pkt, sizeof(pkt), MSG_CONFIRM, (const struct sockaddr *) &server_addr_to, addr_size);
    //         print_send(&pkt, last);

    //         swnd[i++ % WINDOW_SIZE] = pkt;
    //         seq_num++;
    //         ack_num++;
    //     }
    //     else if (!ack_pkt.ack) {
    //         sendto(send_sockfd, &swnd[ack_pkt.seqnum % WINDOW_SIZE], sizeof(swnd[ack_pkt.seqnum % WINDOW_SIZE]), MSG_CONFIRM, (const struct sockaddr *) &server_addr_to, addr_size);
    //         print_send(&pkt, last);
    //         printf("waiting...\n");
    //     } else{

    //         sendto(send_sockfd, &pkt, sizeof(pkt), MSG_CONFIRM, (const struct sockaddr *) &server_addr_to, addr_size);
    //         print_send(&pkt, last);
    //     }

    //     recvfrom(listen_sockfd, &ack_pkt, sizeof(ack_pkt), MSG_WAITALL, (struct sockaddr *) &server_addr_from, &addr_size);
    //     print_recv(&ack_pkt);
    // }























































    // struct packet swnd[1000];

    // do {
    //     bytes_read = fread(buffer, sizeof(char), PAYLOAD_SIZE, fp);

    //     build_packet(&pkt, seq_num, ack_num, last, ack, sizeof(buffer) - 1, buffer);
    //     swnd[i++] = pkt;

    //     sendto(send_sockfd, &pkt, sizeof(pkt), MSG_CONFIRM, (const struct sockaddr *) &server_addr_to, addr_size);
    //     print_send(&pkt, last);

    //     recvfrom(listen_sockfd, &ack_pkt, sizeof(ack_pkt), MSG_WAITALL, (struct sockaddr *) &server_addr_from, &addr_size);
    //     print_recv(&ack_pkt);

    //     while (ack_pkt.last) {
    //         if (ack_num <= ack_pkt.acknum)
    //             break;
    //         sendto(send_sockfd, &swnd[ack_pkt.acknum], sizeof(swnd[ack_pkt.acknum]), MSG_CONFIRM, (const struct sockaddr *) &server_addr_to, addr_size);
    //         print_send(&swnd[ack_pkt.acknum], last);

    //         recvfrom(listen_sockfd, &ack_pkt, sizeof(ack_pkt), MSG_WAITALL, (struct sockaddr *) &server_addr_from, &addr_size);
    //         print_recv(&ack_pkt);
    //     }
    //     
    //     seq_num = ack_pkt.seqnum;
    //     ack_num = ack_pkt.acknum;

    //     printf("\n");
    // } while (0 < bytes_read);

    // sendto(send_sockfd, NULL, 0, 0, (const struct sockaddr *) &server_addr_to, addr_size);


    // struct packet cwnd;
    // ssize_t bytes_read = 0;
    // while (0 < (bytes_read = fread(buffer, sizeof(char), sizeof(buffer) - 1, fp))) {
    //     build_packet(&pkt, seq_num, ack_num, last, ack, sizeof(buffer) - 1, buffer);
    //     sendto(send_sockfd, &pkt, sizeof(pkt), 0, (const struct sockaddr *) &server_addr_to, addr_size);
    //     print_send(&pkt, 0);

    //     while (ack_pkt.last) {
    //         sendto(send_sockfd, &cwnd, sizeof(cwnd), 0, (const struct sockaddr *) &server_addr_to, addr_size);
    //         print_send(&pkt, 1);

    //         recvfrom(listen_sockfd, &ack_pkt, sizeof(ack_pkt), MSG_WAITALL, (struct sockaddr *) &server_addr_from, &addr_size);
    //         seq_num += 1;
    //         ack_num = ack_pkt.acknum;
    //         print_recv(&ack_pkt);
    //     }

    //     recvfrom(listen_sockfd, &ack_pkt, sizeof(ack_pkt), MSG_WAITALL, (struct sockaddr *) &server_addr_from, &addr_size);
    //     seq_num += 1;
    //     ack_num = ack_pkt.acknum + 1;
    //     print_recv(&ack_pkt);
    //     

    //     cwnd = pkt;
    // }

    // sendto(send_sockfd, NULL, 0, 0, (const struct sockaddr *) &server_addr_to, addr_size);

 
    
    fclose(fp);
    close(listen_sockfd);
    close(send_sockfd);
    return 0;
}

