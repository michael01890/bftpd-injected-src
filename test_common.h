#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/time.h>

// Test configuration
#define TEST_CONFIG_FILE "bftpd.test"
#define TEST_PORT 18522 
#define TEST_HOST "127.0.0.1"

// Global variables
extern pid_t server_pid;
extern int client_socket;

// Helper functions
int read_ftp_response(int sock, char *buffer, int size);
int send_ftp_command(int sock, const char *cmd);

// Server control functions
int start_ftp_server(void);
void stop_ftp_server(void);

// Connection management
int connect_to_server(int port);
void disconnect_from_server(int sock);

#endif /* TEST_COMMON_H */ 