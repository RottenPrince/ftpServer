#ifndef FILE_SERVER_H
#define FILE_SERVER_H

#include <stdbool.h>

#define PORT "20000"    // the port users will be connecting to
#define BUFFERSIZE 1024 // max buffer size for sending file contents
#define MAX_FILES 20    // max number of files
#define BACKLOG 10      // how many pending connections queue will hold
#define MSG_SIZE                                                               \
  100 // max size of the message, 100 because the client can only receive 100
      // bytes of data

// Function declarations
bool find_file(char *name);
char **listFilesInDirectory();

#endif /* FILE_SERVER_H */
