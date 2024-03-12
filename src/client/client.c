#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT "20000"    // The port client will be connecting to 
#define MAXDATASIZE 100 // Max number of bytes we can get at once 
#define BUFFERSIZE 1024 // Max buffer size for sending file contents 
#define MAX_FILES 20    // Max number of files 
#define IPv6 "::1"      // Server address 

void *get_in_addr(struct sockaddr *sa);

int main() {
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // Allow for any IP version 
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets 
    rv = getaddrinfo(IPv6, PORT, &hints, &servinfo);
    if (rv != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    // Loop through all the results and connect to the first we can 
    for (p = servinfo; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) {
            perror("client: socket");
            continue;
        }
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) { // Fixed the connect() function call
            close(sockfd);
            perror("client: connect");
            continue;
        }
        break; // If we get here, we must have connected successfully 
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
    printf("client: connecting to %s\n", s);
    freeaddrinfo(servinfo); // All done with this structure 
    char cmd[MAXDATASIZE];
    while (1) {
        printf("> ");
        fgets(cmd, MAXDATASIZE, stdin);
        cmd[strcspn(cmd, "\n")] = 0; // Remove newline character 
        if (strcmp(cmd, "quit") == 0) {
            break;
        } else if (strcmp(cmd, "@dir") == 0) {
            send(sockfd, cmd, strlen(cmd), 0);
            // Receive list of files 
            char buffer[BUFFERSIZE];
            memset(buffer, 0, BUFFERSIZE);
            recv(sockfd, buffer, BUFFERSIZE - 1, 0);
            printf("Files available for download:\n%s\n", buffer);
        } else if (strncmp(cmd, "@get ", 5) == 0) {
            send(sockfd, cmd, strlen(cmd), 0); // Send the @get command with filename 
            // Prepare to receive the file 
            char file_buffer[BUFFERSIZE];
            memset(file_buffer, 0, BUFFERSIZE);
            // Extract the filename from the command 
            char* filename = cmd + 5; // Skip "@get " to get the filename 
            FILE *fp = fopen(filename, "wb");
            if (fp == NULL) {
                perror("Error opening file");
                continue;
            }
            // Initialize to keep track of total bytes received 
            int total_bytes_received = 0;
            // Receive file contents 
            int bytes_received;
            while ((bytes_received = recv(sockfd, file_buffer, BUFFERSIZE, 0)) > 0) {
                total_bytes_received += bytes_received; // Update total bytes received 
                size_t written = fwrite(file_buffer, 1, bytes_received, fp);
                if (written < (size_t)bytes_received) {
                    perror("Error writing file");
                    break;
                }
                // Check if we've received the file completely (optional, based on your protocol) 
                if (bytes_received < BUFFERSIZE) {
                    break; // Assume end of file if we received less than BUFFERSIZE 
                }
            }
            if (bytes_received < 0) {
                perror("recv error");
            } else {
                printf("File '%s' downloaded successfully, %d bytes received.\n", filename, total_bytes_received);
            }
            fclose(fp);
        } else if (strcmp(cmd, "help") == 0) {
            printf("@dir - List files available for download\n@get <filename> - Download a file\nquit - Exit the client\n");
        } else {
            printf("Invalid command. Type 'help' for a list of commands.\n");
        }
    }
    close(sockfd);
    return 0;
}

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
