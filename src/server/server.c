#include <arpa/inet.h>
#include <dirent.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT "20000"
#define BUFFERSIZE 1024
#define MAX_FILES 20
#define BACKLOG 10

void *get_in_addr(struct sockaddr *sa);

char *listFilesInDirectory() {
    DIR *d;
    struct dirent *dir;
    char *files = malloc(BUFFERSIZE);
    if (!files) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }
    files[0] = '\0'; // Initialize the string to be empty
    d = opendir("./@dir/"); // Open the current directory
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_type == DT_REG) { // Check if it's a regular file
                strcat(files, dir->d_name);
                strcat(files, "\n");
            }
        }
        closedir(d);
    }
    return files;
}

int main(void) {
    int sockfd, new_fd;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr;
    socklen_t sin_size;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET6; // Change this to AF_INET6 to force IPv6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
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
        return 2;
    }
    freeaddrinfo(servinfo);
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
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
        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
        printf("server: got connection from %s\n", s);
    
        char buf[BUFFERSIZE]; // Now handle each client request in a loop
        int numbytes;
        while((numbytes = recv(new_fd, buf, BUFFERSIZE-1, 0)) > 0) {
            buf[numbytes] = '\0';
            if (strcmp(buf, ".") == 0) {
                char *fileList = listFilesInDirectory();
                send(new_fd, fileList, strlen(fileList), 0);
                free(fileList); // Make sure to free the allocated memory
            } else if (strcmp(buf, "@dir") == 0) {
                char *fileList = listFilesInDirectory("../@dir"); // Relative path from src to @dir
                send(new_fd, fileList, strlen(fileList), 0);
                free(fileList); // Make sure to free the allocated memory
            } 
            else if (strncmp(buf, "@get ", 5) == 0) {
              // Extract the filename from the command
              char *filename = buf + 5;

              // Open the file in read-only, binary mode to prevent any modification
              FILE *fp = fopen(filename, "rb");
              if (fp == NULL) {
                  perror("Failed to open file");
                  continue;
              }

              // Read file contents and send them to client
              char file_buffer[BUFFERSIZE];
              int bytes_read;
              while ((bytes_read = fread(file_buffer, 1, BUFFERSIZE, fp)) > 0) {
                  send(new_fd, file_buffer, bytes_read, 0);
              }
              fclose(fp); // Close the file after reading
              printf("File sent: %s\n", filename);
            } else {
                // Handle other commands or close connection
                break; // For now, break the loop if the command is not recognized
            }
        }
        close(new_fd);  // parent doesn't need this
    }
    return 0;
}

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
