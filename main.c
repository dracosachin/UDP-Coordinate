#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <time.h>
#include <netdb.h>

#define PORT 12345
#define BUFFER_SIZE 1024
#define HEARTBEAT_INTERVAL 2  // Send a heartbeat every 2 seconds
#define TIMEOUT 30  // Timeout in seconds for waiting for all heartbeats

int ready_count = 0;
int total_processes = 0;

// Function to read the hostfile and store hostnames
void read_hostfile(const char *hostfile, char hostnames[][BUFFER_SIZE], int *total_processes) {
    FILE *file = fopen(hostfile, "r");
    if (!file) {
        fprintf(stderr, "Error opening hostfile\n");
        exit(EXIT_FAILURE);
    }

    char hostname[BUFFER_SIZE];
    int count = 0;
    while (fscanf(file, "%s", hostname) != EOF) {
        strcpy(hostnames[count], hostname);
        count++;
    }

    fclose(file);
    *total_processes = count;
}

// Function to send heartbeat messages to other processes
void send_heartbeat(const char *hostname, int sockfd, const char hostnames[][BUFFER_SIZE], int total_processes) {
    char message[BUFFER_SIZE];
    snprintf(message, sizeof(message), "HEARTBEAT from %s", hostname);

    for (int i = 0; i < total_processes; i++) {
        if (strcmp(hostnames[i], hostname) != 0) {
            struct sockaddr_in peer_addr;
            memset(&peer_addr, 0, sizeof(peer_addr));
            peer_addr.sin_family = AF_INET;
            peer_addr.sin_port = htons(PORT);

            // Resolve the hostname to an IP address
            struct hostent *he = gethostbyname(hostnames[i]);
            if (he == NULL) {
                fprintf(stderr, "Error resolving hostname %s\n", hostnames[i]);
                continue;
            }

            memcpy(&peer_addr.sin_addr, he->h_addr_list[0], he->h_length);

            char ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(peer_addr.sin_addr), ip_str, INET_ADDRSTRLEN);

            fprintf(stderr, "Sending heartbeat to %s (IP: %s)\n", hostnames[i], ip_str);
            sendto(sockfd, message, strlen(message), 0, (const struct sockaddr *)&peer_addr, sizeof(peer_addr));
        }
    }
}

// Function to receive heartbeat messages from other processes
void receive_heartbeat(int sockfd, char received_hosts[][BUFFER_SIZE], int *ready_count, int total_processes) {
    char buffer[BUFFER_SIZE];
    struct sockaddr_in cli_addr;
    socklen_t len = sizeof(cli_addr);

    int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&cli_addr, &len);
    if (n > 0) {
        buffer[n] = '\0';

        char sender[BUFFER_SIZE];
        sscanf(buffer, "HEARTBEAT from %s", sender);

        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(cli_addr.sin_addr), ip_str, INET_ADDRSTRLEN);

        fprintf(stderr, "Received heartbeat from %s (IP: %s)\n", sender, ip_str);

        // Check if this heartbeat has already been received
        int already_received = 0;
        for (int i = 0; i < *ready_count; i++) {
            if (strcmp(received_hosts[i], sender) == 0) {
                already_received = 1;
                break;
            }
        }

        if (!already_received) {
            strcpy(received_hosts[*ready_count], sender);
            (*ready_count)++;
            fprintf(stderr, "Logged heartbeat from %s\n", sender);
        }
    }
}

int main(int argc, char *argv[]) {
    // Validate arguments
    if (argc != 3 || strcmp(argv[1], "-h") != 0) {
        fprintf(stderr, "Usage: %s -h <hostfile>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *hostfile = argv[2];
    char hostname[BUFFER_SIZE];
    
    FILE *f = fopen("/etc/hostname", "r");
    if (f == NULL) {
        fprintf(stderr, "Error reading hostname\n");
        exit(EXIT_FAILURE);
    }
    fscanf(f, "%s", hostname);
    fclose(f);

    // Array to store the hostnames
    char hostnames[BUFFER_SIZE][BUFFER_SIZE];
    char received_hosts[BUFFER_SIZE][BUFFER_SIZE];  // Track received heartbeats

    // Read the hostfile and get the list of other hosts
    read_hostfile(hostfile, hostnames, &total_processes);

    // Create a UDP socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        fprintf(stderr, "Socket creation failed\n");
        exit(EXIT_FAILURE);
    }

    // Server address setup
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    // Bind socket to port
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        fprintf(stderr, "Socket bind failed\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    fd_set read_fds;
    struct timeval timeout;
    time_t last_heartbeat_time = time(NULL);
    time_t start_time = time(NULL);

    int all_ready = 0;

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(sockfd, &read_fds);

        // Timeout for select to check for heartbeats and send them periodically
        timeout.tv_sec = HEARTBEAT_INTERVAL;
        timeout.tv_usec = 0;

        // Check for incoming heartbeat messages
        int activity = select(sockfd + 1, &read_fds, NULL, NULL, &timeout);
        if (activity > 0 && FD_ISSET(sockfd, &read_fds)) {
            receive_heartbeat(sockfd, received_hosts, &ready_count, total_processes);
        }

        // Check if all heartbeats have been received
        if (ready_count >= total_processes - 1) {
            fprintf(stderr, "READY\n");
            all_ready = 1;
            break;
        }

        // Send heartbeat if enough time has passed since the last heartbeat
        if (difftime(time(NULL), last_heartbeat_time) >= HEARTBEAT_INTERVAL) {
            send_heartbeat(hostname, sockfd, hostnames, total_processes);
            last_heartbeat_time = time(NULL);
        }

        // Check if timeout has been reached
        if (difftime(time(NULL), start_time) >= TIMEOUT) {
            fprintf(stderr, "Timeout reached. Exiting.\n");
            break;
        }
    }

    // Clean up
    close(sockfd);

    return 0;
}