#include "test_common.h"
#include "main.h"
#include "options.h"
#include "mypaths.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <stdarg.h>


// Global variables
pid_t server_pid = -1;
int client_socket = -1;

// Helper function to read FTP response
int read_ftp_response(int sock, char *buffer, int size) {
    int bytes_read = 0;
    int total_read = 0;
    char *ptr = buffer;
    
    memset(buffer, 0, size);
    
    while (total_read < size - 1) {
        bytes_read = recv(sock, ptr, 1, 0);
        if (bytes_read <= 0) {
            return -1;
        }
        
        if (*ptr == '\n') {
            *(ptr + 1) = '\0';
            return total_read + 1;
        }
        
        ptr++;
        total_read++;
    }
    
    buffer[size - 1] = '\0';
    return total_read;
}

// Helper function to send FTP command
int send_ftp_command(int sock, const char *cmd) {
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "%s\r\n", cmd);
    return send(sock, buffer, strlen(buffer), 0);
}

// Test suite initialization
int init_test_suite(void) {
    fprintf(stderr, "Initializing test suite...\n");
    return 0;
}

// Test suite cleanup
int clean_test_suite(void) {
    fprintf(stderr, "Cleaning up test suite...\n");
    if (client_socket != -1) {
        close(client_socket);
        client_socket = -1;
    }
    if (server_pid != -1) {
        stop_ftp_server();
        server_pid = -1;
    }
    return 0;
}

// Start FTP server
int start_ftp_server(void) {
    // First, make sure any existing server is stopped
    stop_ftp_server();
    
    server_pid = fork();
    if (server_pid == -1) {
        return -1;
    }
    
    if (server_pid == 0) {  // Child process (server)
        char *argv[] = {
            "bftpd",
            "-c",
            TEST_CONFIG_FILE,
            "-D",
            NULL
        };
        int argc = 4;
        
        int result = start_ftp(argc, argv);
        exit(result);
    }
    
    // Parent process - wait for server to start
    sleep(2);
    
    // Verify server is running
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        stop_ftp_server();
        return -1;
    }
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(TEST_PORT);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        close(sock);
        stop_ftp_server();
        return -1;
    }
    
    close(sock);
    return 0;
}

// Stop FTP server
void stop_ftp_server(void) {
    if (server_pid != -1) {
        // First try SIGTERM
        kill(server_pid, SIGTERM);
        
        // Wait for process to terminate
        int status;
        pid_t result = waitpid(server_pid, &status, WNOHANG);
        
        // If process is still running after a short wait, use SIGKILL
        if (result == 0) {
            sleep(1);
            kill(server_pid, SIGKILL);
            waitpid(server_pid, &status, 0);
        }
        
        server_pid = -1;
    }
}

// Connect to FTP server
int connect_to_server(int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        return -1;
    }
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        close(sock);
        return -1;
    }
    
    return sock;
}

// Disconnect from FTP server
void disconnect_from_server(int sock) {
    if (sock != -1) {
        close(sock);
    }
}
