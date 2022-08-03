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

#include "blockchain/blockchain.h"

#define MAX_PEERS 2

void servers_init(int api_port, int p2p_port);
void api_server_run();
void api_server_handle_request(int client_sockfd, int pipefd[]);
void p2p_server_run();
void update_peers_connections(int p2p_port);

// Global API socket descriptor
int api_server_sockfd;

// Global P2P socket descriptor
struct peers
{
    int connected;
    char *addr;
    int port;
    int sockfd;
} peers[MAX_PEERS];

int p2p_server_sockfd;
// List of peer sockets
struct sockaddr_in peer_addr[MAX_PEERS];

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

    // Check for correct number of arguments
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <api port> <p2p port>\n", argv[0]);
        exit(1);
    }

    // Check correctness of port number
    int api_port = atoi(argv[1]);
    int p2p_port = atoi(argv[2]);
    if (api_port < 1024 || api_port > 65535 || p2p_port < 1024 || p2p_port > 65535)
    {
        fprintf(stderr, "Invalid port number\n");
        exit(1);
    }

    // Initialize servers
    servers_init(api_port, p2p_port);

    // Run API server
    api_server_run();

    // Run P2P server
    p2p_server_run(p2p_port);
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

    // Init list of peers from file peers.txt
    FILE *fp = fopen("src/peers.txt", "r");
    if (fp == NULL)
    {
        printf("Error opening file\n");
        exit(1);
    }
    char line[1024];
    int i = 0;
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        char *addr = strtok(line, ":");
        int port = atoi(strtok(NULL, ":"));
        peers[i].connected = 0;
        peers[i].addr = addr;
        peers[i].port = port;
        i++;
    }

    // Close file
    fclose(fp);

    // Update list of peers connections
    update_peers_connections(p2p_port);
}

void api_server_run()
{
    // Listen for connections
    if (listen(api_server_sockfd, 5) < 0)
    {
        printf("Error listening\n");
        exit(1);
    }

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

        // Pipe initialization
        int pipefd[2];
        if (pipe(pipefd) < 0)
        {
            printf("Error creating pipe\n");
            exit(1);
        }

        // Spawn child process
        pid_t pid = fork();
        if (pid < 0)
        {
            printf("Error forking child process\n");
            exit(1);
        }
        else if (pid == 0)
        {
            // Child process
            close(api_server_sockfd);
            api_server_handle_request(client_sockfd, pipefd);
            exit(0);
        }
        else
        {
            // Parent process
            close(client_sockfd);
            close(pipefd[1]);

            // Read from pipe if there is data
            char buffer[1024];
            int n = read(pipefd[0], buffer, 1024);
            if (n > 0)
            {
                add_block(blockchain, buffer);
            }
        }
    }
}

// Server-side request handler
void api_server_handle_request(int client_sockfd, int pipefd[])
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

    // Print request to stdout
    printf("%s\n", request);

    // Handle GET request
    if (strncmp(request, "GET", 3) == 0)
    {
        // Get resource requested (path)
        char path[1024];
        sscanf(request, "GET %s", path);

        if (strncmp(path, "/blocks\0", 8) == 0)
        {
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
            // Get data from request body
            char data[1024];
            // Read request line by line until /r/n/r/n is found
            for (int i = 0; i < 1024; i++)
            {
                if (request[i] == '\r' && request[i + 1] == '\n' && request[i + 2] == '\r' && request[i + 3] == '\n')
                {
                    strncpy(data, &request[i + 4], 1024);
                    break;
                }
            }

            // Write data on pipe to parent process
            n = write(pipefd[1], data, strlen(data));
            if (n < 0)
            {
                printf("Error writing to pipe\n");
                exit(1);
            }

            // Send 200 OK response with JSON
            char response[1024];
            sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n");
            n = write(client_sockfd, response, strlen(response));
            if (n < 0)
            {
                printf("Error writing to socket\n");
                exit(1);
            }
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
        }
    }
}

void p2p_server_run(int p2p_port)
{
    // Listen for connections
    if (listen(p2p_server_sockfd, 5) < 0)
    {
        printf("Error listening\n");
        exit(1);
    }

    // Accept connections and spawn child processes
    while (1)
    {
        // Accept connection
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_sockfd = accept(p2p_server_sockfd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_sockfd < 0)
        {
            printf("Error accepting connection\n");
            exit(1);
        }

        // Spawn child process
        pid_t pid = fork();
        if (pid < 0)
        {
            printf("Error forking child process\n");
            exit(1);
        }
        else if (pid == 0)
        {
            // Child process
            close(p2p_server_sockfd);
            
            exit(0);
        }
        else
        {
            // Parent process
            close(client_sockfd);
        }
    }
}

void update_peers_connections(int p2p_port)
{
    for (int i = 0; i < MAX_PEERS; i++)
    {
        if (peers[i].connected == 0 && peers[i].port != p2p_port)
        {
            printf("Trying to connect to %s:%d\n", peers[i].addr, peers[i].port);
            // Try to connect to peer
            int sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0)
            {
                printf("Error creating socket\n");
                exit(1);
            }
            struct sockaddr_in server_addr;
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(peers[i].port);
            server_addr.sin_addr.s_addr = inet_addr(peers[i].addr);
            if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
            {
                printf("Error connecting to peer\n");
            }
            else
            {
                peers[i].connected = 1;
                peers[i].sockfd = sockfd;

                printf("Connected to peer %s:%d\n", peers[i].addr, peers[i].port);
            }
        }
    }
}