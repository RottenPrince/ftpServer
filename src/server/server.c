#include <arpa/inet.h>
#include <dirent.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define PORT "20000" // the port users will be connecting to
#define BACKLOG 10   // how many pending connections queue will hold
#define MSG_SIZE                                                               \
  100 // max size of the message, 100 because the client can only recieve 100
      // bytes of data

// Server must become a FTP server
// on command:
// 	@dir would provide the list of existing files that are eligible for
// download
// 	@get would fetch file or files and send to the client that requested
// them

void *get_in_addr(struct sockaddr *sa);
int validate_requested_file(FILE *fp);

int main() {
  int sockfd, new_fd; // listens on sockfd, new connections on new_fd
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_storage their_addr; // connector's address information
  socklen_t sin_size;
  int yes = 1;
  int no = 0;
  char s[INET6_ADDRSTRLEN];
  int rv;

  memset(&hints, 0, sizeof hints); // Initialize hints struct to zero

  // hints.ai_family = AF_UNSPEC; // Specify that the address family is
  // unspecified, which allows both IPv4 and IPv6
  hints.ai_family = AF_INET6;
  hints.ai_socktype = SOCK_STREAM; // Specify socket type as stream (TCP)
  hints.ai_flags = AI_PASSIVE;     // Use localhost

  rv = getaddrinfo(NULL, PORT, &hints,
                   &servinfo); // Retrieve address information for the localhost
                               // and specified port
  if (rv != 0) {
    fprintf(stderr, "getaddrinfo: %s\n",
            gai_strerror(rv)); // Print error if getaddrinfo fails
    return 1;                  // Exit program with error code
  }

  // Loop through all the results and bind to the first available
  for (p = servinfo; p != NULL; p = p->ai_next) {
    sockfd =
        socket(p->ai_family, p->ai_socktype, p->ai_protocol); // Create a socket

    if (sockfd == -1) {
      perror("server: socket"); // Print error messages
      continue;                 // Continue to the next result
    }

    // Set socket option to allow IPv4 and IPv6 on the same socket
    if (p->ai_family == AF_INET6 &&
        setsockopt(sockfd, IPPROTO_IPV6, IPV6_V6ONLY, &no, sizeof(yes)) == -1) {
      perror("setsockopt");
      close(sockfd);
      freeaddrinfo(servinfo);
      return -1;
    }

    // Allow the socket address to be reused before TIME_WAIT expires
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
      perror("setsockopt"); // Print error message
      exit(1);              // Exit program with error code
    }

    // Bind the socket to the port
    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);          // Close the socket
      perror("server: bind"); // Print error message
      continue;               // Continue to the next result
    }

    break; // Break the loop if binding is successful
  }

  freeaddrinfo(
      servinfo); // Free the memory allocated for the address information

  if (p == NULL) {
    fprintf(stderr,
            "server: failed to bind\n"); // Print error message if binding fails
    exit(1);                             // Exit program with error code
  }

  // Set up the socket for listening
  if (listen(sockfd, BACKLOG) == -1) {
    perror("listen"); // Print error message
    exit(1);          // Exit program with error code
  }

  printf("server: waiting for a connection...\n"); // Print message indicating
                                                   // waiting for connection

  sin_size = sizeof their_addr;
  new_fd = accept(sockfd, (struct sockaddr *)&their_addr,
                  &sin_size); // Accept a connection

  // Connection unsuccessful
  if (new_fd == -1) {
    perror("accept"); // Print error message
  }

  // Connection successful
  inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s); // Get the IP address of the client and store it in s
  printf("Server: got connection from %s\n",
         s); // Print message indicating successful connection

  close(sockfd); // Done listening for new connections
                 // message exchange
  char msg[MSG_SIZE];

  // Receive message from the client
  int receive_result = recv(new_fd, msg, sizeof(msg) - 1, 0);
  if (receive_result == -1) {
    perror("recv");
    exit(1);
  }
  // In here there should be a validator of inputs from users
  //

  msg[receive_result] = '\0';             // Null-terminate the received data
  printf("Server received: '%s'\n", msg); // Print the received message

  // Send the received message back to the client
  int send_result =
      send(new_fd, msg, strlen(msg), 0); // Send the message back to client
  if (send_result == -1) {
    perror("send"); // Print error message if sending data fails
  }

  close(new_fd); // Done sending message, close connection to client return 0;
                 // // Exit program with success code
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in *)sa)->sin_addr); // Return IPv4 address
  }
  return &(((struct sockaddr_in6 *)sa)->sin6_addr); // Return IPv6 address
}
