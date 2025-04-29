#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "main.h"
#include "options.h"
#include "mypaths.h"
#include "commands.h"
#include "test_common.h"


// Function declarations from main.c
extern int start_ftp(int argc, char **argv);

// Helper function to perform FTP login
int ftp_login(const char *username, const char *password) {
    int sock;
    char buffer[1024];
    char cmd[1024];
    int result;
    
    // Connect to server using helper function
    sock = connect_to_server(TEST_PORT);
    if (sock < 0) return -1;
    
    // Read welcome message
    result = read_ftp_response(sock, buffer, sizeof(buffer));
    if (result <= 0) {
        close(sock);
        return -1;
    }
    
    // Send USER command
    snprintf(cmd, sizeof(cmd), "USER %s", username);
    result = send_ftp_command(sock, cmd);
    if (result <= 0) {
        close(sock);
        return -1;
    }
    
    // Read USER response
    result = read_ftp_response(sock, buffer, sizeof(buffer));
    // Assert that the response is 331
    CU_ASSERT(result > 0);
    CU_ASSERT(strncmp(buffer, "331", 3) == 0);
    if (result <= 0 || strncmp(buffer, "331", 3) != 0) {
        close(sock);
        return -1;
    }
    
    // Send PASS command
    snprintf(cmd, sizeof(cmd), "PASS %s", password);
    result = send_ftp_command(sock, cmd);
    if (result <= 0) {
        close(sock);
        return -1;
    }
    
    // Read PASS response
    result = read_ftp_response(sock, buffer, sizeof(buffer));
    // Assert that the response is 230
    CU_ASSERT(result > 0);
    CU_ASSERT(strncmp(buffer, "230", 3) == 0);
    if (result <= 0 || strncmp(buffer, "230", 3) != 0) {
        close(sock);
        return -1;
    }
    
    return sock;
}

// Test case for login functionality
void test_login(void) {
    // Start server for this test
    CU_ASSERT(start_ftp_server() == 0);
    
    int sock = ftp_login("cs408", "cs408");
    CU_ASSERT(sock >= 0);
    if (sock >= 0) {
        close(sock);
    }
    
    // Stop server after test
    stop_ftp_server();
}


// Test case for LIST command
// It's similar to retr command, but it's used to list the files in the current directory
void test_list(void) {
    // Start server for this test
    CU_ASSERT(start_ftp_server() == 0);

    int sock = ftp_login("cs408", "cs408");
    char buffer[1024];
    int result;

    CU_ASSERT(sock >= 0);
    if (sock < 0) return;

    // send pasv command
    result = send_ftp_command(sock, "PASV");
    CU_ASSERT(result > 0);
    if (result <= 0) {
        close(sock);
        stop_ftp_server();
        return;
    }

    // read pasv response
    result = read_ftp_response(sock, buffer, sizeof(buffer));
    CU_ASSERT(result > 0);
    if (result <= 0) {
        close(sock);
        stop_ftp_server();
        return;
    }

    // parse port numbers from response
    int port_p1, port_p2;
    int data_port;
    if (sscanf(buffer, "227 Entering Passive Mode (%*d,%*d,%*d,%*d,%d,%d)", &port_p1, &port_p2) == 2) {
        data_port = port_p1 * 256 + port_p2;
        fprintf(stderr, "Data port: %d\n", data_port);
    } else {
        CU_ASSERT(0); // Failed to parse port numbers
        close(sock);
        stop_ftp_server();
        return;
    }

    // connect to data port using helper function
    int data_sock = connect_to_server(data_port);
    CU_ASSERT(data_sock >= 0);
    if (data_sock < 0) {
        close(sock);
        stop_ftp_server();
        return;
    }

    // send list command
    result = send_ftp_command(sock, "LIST");
    CU_ASSERT(result > 0);
    if (result <= 0) {
        close(sock);
        stop_ftp_server();
        return;
    }

    // read list response
    result = read_ftp_response(data_sock, buffer, sizeof(buffer));
    CU_ASSERT(result > 0);
    if (result <= 0) {
        close(sock);
        stop_ftp_server();
        return;
    }

    // check if the response is correct
    // CU_ASSERT(strncmp(buffer, "150", 3) == 0);

    close(data_sock);
    close(sock);
    stop_ftp_server();
} 

// Test suite initialization (empty since we handle server in each test)
int init_suite(void) {
    return 0;
}

// Test suite cleanup (empty since we handle server in each test)
int clean_suite(void) {
    return 0;
}

// Test suite entry point
int main(void) {
    CU_pSuite pSuite = NULL;
    
    // Initialize CUnit
    if (CUE_SUCCESS != CU_initialize_registry()) {
        return CU_get_error();
    }
    
    // Add the test suite
    pSuite = CU_add_suite("FTP Tests", init_suite, clean_suite);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    
    // Add the tests
    if ((NULL == CU_add_test(pSuite, "test_login", test_login))) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    
    if ((NULL == CU_add_test(pSuite, "test_list", test_list))) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    
    // Run all tests
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
} 
