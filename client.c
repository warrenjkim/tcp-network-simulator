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
    if (setsockopt(listen_sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) <
        0) {
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
    struct packet ack_pkt = {0};
    char buffer[PAYLOAD_SIZE];
    unsigned short seq_num = 0;
    unsigned short ack_num = 0;
    char last = 0;
    char ack = 1;

    State state = SLOW_START;

    ssize_t mss = 0;
    size_t ssthresh = (unsigned short)(-1);
    int dup_count = 0;

    Queue *cwnd = queue_init();
    ssize_t recv_len = 0;

    struct stat info;
    if (stat(filename, &info) != 0) {
        perror("stat error");
        exit(1);
    }

    char *file = (char *)malloc(sizeof(char) * info.st_size);
    int i = -1;
    if (!file) {
        perror("malloc error");
        exit(1);
    }

    if (fread(file, sizeof(char), info.st_size, fp) !=
        (unsigned long)info.st_size) {
        perror("fread error");
        exit(1);
    }

    while (1) {
        if (0 <= recv_len) {
            mss = queue_pop_cum(cwnd, ack_pkt.acknum) +
                  queue_pop(cwnd, ack_pkt.seqnum);
        }

        if (mss == 0) {
            dup_count++;
        }

        while (0 < mss--) {
            switch (state) {
            case SLOW_START:
                cwnd->max_size += MSS;
                if (ssthresh < cwnd->max_size) {
                    state = CONGESTION_AVOIDANCE;
                }
                break;
            case CONGESTION_AVOIDANCE:
                cwnd->max_size += (double)(MSS) / cwnd->max_size;
                break;
            case FAST_RECOVERY:
                cwnd->max_size = ssthresh;
                state = SLOW_START;
                break;
            }

            dup_count = 0;
        }

        if (state == FAST_RECOVERY && 3 < dup_count) {
            cwnd->max_size += MSS;
        }

        if (dup_count == 3) {
            state = FAST_RECOVERY;
            ssthresh = (size_t)(cwnd->max_size) / 2 < 2
                           ? 2
                           : (size_t)(cwnd->max_size) / 2;
            cwnd->max_size = ssthresh + 3;

            struct packet *resend = queue_top(cwnd);
            sendto(send_sockfd, resend, sizeof(struct packet), 0,
                   (const struct sockaddr *)&server_addr_to, addr_size);

            recv_len =
                recvfrom(listen_sockfd, &ack_pkt, sizeof(struct packet), 0,
                         (struct sockaddr *)&server_addr_from, &addr_size);
            continue;
        }

        // timeout
        if (recv_len < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            ssthresh = (size_t)(cwnd->max_size) / 2 < 2
                           ? 2
                           : (size_t)(cwnd->max_size) / 2;
            cwnd->max_size = MSS;
            state = SLOW_START;

            struct packet *resend = queue_top(cwnd);
            sendto(send_sockfd, resend, sizeof(struct packet), 0,
                   (const struct sockaddr *)&server_addr_to, addr_size);

            recv_len =
                recvfrom(listen_sockfd, &ack_pkt, sizeof(struct packet), 0,
                         (struct sockaddr *)&server_addr_from, &addr_size);
            continue;
        }

        // send while cwnd is not full
        while (!queue_full(cwnd) && !last) {
            off_t bytes_read = info.st_size - (++i * PAYLOAD_SIZE);
            if (bytes_read < PAYLOAD_SIZE) {
                memcpy(buffer, file + (i * PAYLOAD_SIZE), bytes_read);
                last = 1;
            } else {
                bytes_read = PAYLOAD_SIZE;
                memcpy(buffer, file + (i * PAYLOAD_SIZE), PAYLOAD_SIZE);
            }

            build_packet(&pkt, seq_num++, ack_num++, last, ack, bytes_read,
                         buffer);
            sendto(send_sockfd, &pkt, sizeof(struct packet), 0,
                   (const struct sockaddr *)&server_addr_to, addr_size);

            cwnd = queue_push(cwnd, &pkt);
        }

        recv_len = recvfrom(listen_sockfd, &ack_pkt, sizeof(ack_pkt), 0,
                            (struct sockaddr *)&server_addr_from, &addr_size);

        if (last) {
            if (ack_pkt.last) {
                break;
            }
        }
    }

    fclose(fp);
    close(listen_sockfd);
    close(send_sockfd);
    return 0;
}
