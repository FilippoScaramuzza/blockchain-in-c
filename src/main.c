// HTTP Server implementation with multiple threads

// Send request to server with telnet command:
// telnet localhost 8080

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>

#include "blockchain/blockchain.h"

#define MAX_PEERS 2

void api_server_init(int api_port);
void api_server_run();
void api_server_handle_request(int client_sockfd);

// Global API socket descriptor
int api_server_sockfd;

// Global P2P socket descriptor
int p2p_server_sockfd;

// Global blockchain pointer
blockchain_t *blockchain;

// Main function
int main(int argc, char *argv[])
{
    // Blockchain initialization
    blockchain = create_blockchain();
    // Add 100 blocks to the blockchain
    for (int i = 0; i < 3; i++)
    {
        char *data = malloc(sizeof(char) * 1024);
        sprintf(data, "Data %d", i);
        add_block(blockchain, data);
    }

    // Check arguments count
    if (argc != 3)
    {
        printf("Usage: %s <api_port> <p2p_port>\n", argv[0]);
        exit(1);
    }

    // Check correctness of port number
    int api_port = atoi(argv[1]);
    int p2p_port = atoi(argv[2]);
    if (api_port < 1024 || api_port > 65535 || p2p_port < 1024 || p2p_port > 65535)
    {
        printf("Invalid port number\n");
        exit(1);
    }

    // Initialize servers
    servers_init(api_port, p2p_port);

    // Run API server
    api_server_run();

    // Run P2P server
    p2p_server_run();
}

void servers_init(int api_port, int p2p_port)
{
    // Create API socket
    api_server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (api_server_sockfd < 0)
    {
        printf("Error opening socket\n");
        exit(1);
    }

    // Create P2P socket
    p2p_server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (p2p_server_sockfd < 0)
    {
        printf("Error opening socket\n");
        exit(1);
    }

    // Set socket options
    int optval = 1;
    setsockopt(api_server_sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));
    setsockopt(p2p_server_sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));

    // Bind socket to port
    struct sockaddr_in server_addr;
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(api_port);
    if (bind(api_server_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("Error binding socket\n");
        exit(1);
    }
    server_addr.sin_port = htons(p2p_port);
    if (bind(p2p_server_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("Error binding socket\n");
        exit(1);
    }

    printf("API server socket bound to port %d\n", api_port);
    printf("P2P server socket bound to port %d\n", p2p_port);
}

void api_server_run()
{
    // Listen for connections
    if (listen(api_server_sockfd, 5) < 0)
    {
        printf("Error listening\n");
        exit(1);
    }

    printf("API server listening\n");

    // Accept connections and spawn child processes
    while (1)
    {
        // Accept connection
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_sockfd = accept(api_server_sockfd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_sockfd < 0)
        {
            printf("Error accepting connection\n");
            exit(1);
        }

        printf("Connection from %s\n", inet_ntoa(client_addr.sin_addr));

        // Create thread for handling request
        pthread_t thread;

        if (pthread_create(&thread, NULL, (void *)api_server_handle_request, (void *)client_sockfd) < 0)
        {
            printf("Error creating thread\n");
            exit(1);
        }

        // Wait for thread to finish
        pthread_join(thread, NULL);

        // Close socket
        close(client_sockfd);
    }
}

void api_server_handle_request(int client_sockfd)
{
    // Receive request
    char request[1024];
    int n = read(client_sockfd, request, 1024);

    // Check for error
    if (n < 0)
    {
        printf("Error reading from socket\n");
        exit(1);
    }

    // Handle GET request
    if (strncmp(request, "GET", 3) == 0)
    {
        // Get resource requested (path)
        char path[1024];
        sscanf(request, "GET %s", path);

        if (strncmp(path, "/blocks\0", 8) == 0)
        {
            printf("Client requested block list\n");

            // Send 200 OK response with JSON
            char response[1024];
            sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n");
            n = write(client_sockfd, response, strlen(response));
            if (n < 0)
            {
                printf("Error writing to socket\n");
                exit(1);
            }

            // JSON representation of blockchain
            char *json = blockchain_to_json(blockchain);
            n = write(client_sockfd, json, strlen(json));
            if (n < 0)
            {
                printf("Error writing to socket\n");
                exit(1);
            }
            free(json);

            printf("GET /blocks response sent\n");
        }
        else
        {
            // Send 404 response
            char response[1024];
            sprintf(response, "HTTP/1.1 404 Not Found\r\n\r\n");
            n = write(client_sockfd, response, strlen(response));
            if (n < 0)
            {
                printf("Error writing to socket\n");
                exit(1);
            }

            printf("404 Not Found response sent\n");
        }
    }
    // Handle POST request
    else if (strncmp(request, "POST", 4) == 0)
    {
        // Get resource requested (path)
        char path[1024];
        sscanf(request, "POST %s", path);

        // TODO - Filter out malicious requests

        if (strncmp(path, "/mine\0", 5) == 0)
        {
            printf("Client requested to mine a block\n");

            // Get data from request body
            char *data;
            data = (char *)malloc(sizeof(char) * 1024);
            // Read request line by line until /r/n/r/n is found
            for (int i = 0; i < 1024; i++)
            {
                if (request[i] == '\r' && request[i + 1] == '\n' && request[i + 2] == '\r' && request[i + 3] == '\n')
                {
                    strncpy(data, &request[i + 4], 1024);
                    break;
                }
            }
    
            // Add block to blockchain
            add_block(blockchain, data);

            // Send 200 OK response with JSON
            char response[1024];
            sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n");
            n = write(client_sockfd, response, strlen(response));
            if (n < 0)
            {
                printf("Error writing to socket\n");
                exit(1);
            }

            printf("POST /mine response sent\n");
        }
        else
        {
            printf("POST request to unrecognized path\n");
            // Send 404 response
            char response[1024];
            sprintf(response, "HTTP/1.1 404 Not Found\r\n\r\n");
            n = write(client_sockfd, response, strlen(response));
            if (n < 0)
            {
                printf("Error writing to socket\n");
                exit(1);
            }

            printf("404 Not Found response sent\n");
        }
    }
}