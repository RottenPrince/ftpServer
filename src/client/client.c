#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT "20000"    // the port client will be connecting to
#define MAXDATASIZE 100 // max number of bytes we can get at once
#define BUFFERSIZE 1024 // max biffer size ofr sending file contents

// client requirements:
// 	client connects to a server
// 	commands:
// 		@dir would display files that are eligible for downdloading
// 		@get would let download file for the client

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa);

int main() {
  int sockfd,
      numbytes; // sockfd = socket file descriptor, numbytes = number of bytes
  char buf[MAXDATASIZE];
  struct addrinfo hints, *servinfo, *p;
  int rv;
  char s[INET6_ADDRSTRLEN];
  char ipaddr[INET6_ADDRSTRLEN];
  int no = 0;

  printf("Enter IP address (v4, v6): ");
  fgets(ipaddr, sizeof(ipaddr), stdin);
  ipaddr[strcspn(ipaddr, "\n")] = 0; // remove newline character

  memset(&hints, 0, sizeof hints); // Initialize hints struct to zero
  hints.ai_family = AF_UNSPEC;     // Specify that the address family is
                               // unspecified, which allows both IPv4 and IPv6
  hints.ai_socktype = SOCK_STREAM; // Specify socket type as stream (TCP)

  rv = getaddrinfo(ipaddr, PORT, &hints,
                   &servinfo); // Retrieve address information for localhost and
                               // specified port

  if (rv != 0) {
    fprintf(stderr, "getaddrinfo: %s\n",
            gai_strerror(rv)); // Print error if getaddrinfo fails
    return 1;
  }

  printf("getaddrinfo is successful: %d\n\n", rv); // Print success message

  // loop through all the results and connect to the first we can
  for (p = servinfo; p != NULL; p = p->ai_next) {
    printf("1");
    sockfd =
        socket(p->ai_family, p->ai_socktype, p->ai_protocol); // Create a socket
    if (sockfd == -1) {         // Check if socket creation failed
      perror("client: socket"); // Print error message
      continue;                 // Continue to the next result
    }

    // Allow the socket address to be reused before TIME_WAIT expires
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &no, sizeof(int)) == -1) {
      perror("setsockopt"); // Print error message
      exit(1);              // Exit program with error code
    }

    if (connect(sockfd, p->ai_addr, p->ai_addrlen) ==
        -1) {                    // Attempt to connect socket to server
      perror("client: connect"); // Print error message if connection fails
      close(sockfd);             // Close the socket
      continue;                  // Continue to the next result
    }
    break; // Break the loop if connection is successful
  }

  if (p == NULL) {
    fprintf(stderr,
            "client: failed to connect\n"); // Print error message if connection
                                            // to server fails
    return 2;                               // Return error code
  }

  // Connection successful
  inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s,
            sizeof s); // Get the IP address of the server and store it in s
  printf("client: connecting to %s\n",
         s); // Print message indicating successful connection
  freeaddrinfo(
      servinfo); // Free the memory allocated for the address information

  // server message verification
  char msg[MAXDATASIZE];
  char verification[] = "verification";
  int verification_result = send(sockfd, verification, strlen(verification) - 1,
                                 0); // send message out
  if (verification_result == -1) {
    perror("send");
    exit(1);
  }
  numbytes = recv(sockfd, verification, sizeof(verification) - 1, 0);
  if (numbytes == -1) {
    perror("recv");
    exit(1);
  }

  // server-client interaction
  int send_result;
  while (strcmp(msg, "quit") != 0) {
    printf(">");
    fgets(msg, sizeof(msg), stdin);
    if (strcmp(msg, "quit")) {
      send_result = send(sockfd, msg, MAXDATASIZE - 1, 0);
      if (send_result == -1) {
        perror("send");
      }
      printf("Job's done!");
      memset(msg, 0, sizeof(msg));
    } else if (strcmp(msg, "@dir")) {
      send_result = send(sockfd, msg, MAXDATASIZE - 1, 0);
      if (send_result == -1) {
        perror("send");
      }

      // receive a list of filenames in @dir directory
      numbytes = recv(sockfd, msg, sizeof(msg) - 1, 0);
      if (numbytes == -1) {
        perror("recv");
        exit(1);
      }
      memset(msg, 0, sizeof(msg));
    }

    else if (strcmp(msg,
                    "@get:")) { // need to pattern match e.g. @get first.txt
      send_result = send(sockfd, msg, MAXDATASIZE - 1, 0);
      if (send_result == -1) {
        perror("send");
      }

      numbytes = recv(sockfd, msg, sizeof(msg) - 1, 0);
      if (numbytes == -1) {
        perror("recv");
        exit(1);
      }

      // get and validate filename before receiving contents of file
      char *buffer[BUFFERSIZE];
      FILE *fp = fopen(filename, "wb");
      if (fp == NULL)
        perror("Error opening file");
      while ((numbytes = read(sockfd, buffer, sizeof(buffer))) > 0) {
        fwrite(buffer, 1, numbytes, filename);
      }
      memset(msg, 0, sizeof(msg));
    } else if (strcmp(msg, "?") || strcmp(msg, "help")) {
      printf("\n\t@dir\n\t@get\n\tquit");
      memset(msg, 0, sizeof(msg));
    } else {
      printf("Invalid command\n\n");
      memset(msg, 0, sizeof(msg));
    }
  }

  // Receive the the content of a file from the server
  // and puts them into the newly created file in client with
  // the same name
  // end result should be
  numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0); // Receive data from server
  if (numbytes == -1) {
    perror("recv"); // Print error message if receiving data fails
    exit(1);        // Exit program with error code
  }

  buf[numbytes] = '\0';                   // Null-terminate the received data
  printf("client: received '%s'\n", buf); // Print the received data
  close(sockfd); // Close the socket connection to the server
  return 0;      // Exit program with success code
}

void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) { // Check if the socket address family is IPv4
    return &(((struct sockaddr_in *)sa)->sin_addr); // Return IPv4 address
  }
  return &(((struct sockaddr_in6 *)sa)->sin6_addr); // Return IPv6 address
}
