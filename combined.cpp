//============================================================================
// Name        : combined.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

// combined.cpp
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>

const int PORT = 8080;
const int BUFFER_SIZE = 1024;

void run_server() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    // Create socket file descriptor
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        std::cerr << "Socket creation failed." << std::endl;
        return;
    }

    // Bind the socket to the port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed." << std::endl;
        close(server_fd);
        return;
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        std::cerr << "Listen failed." << std::endl;
        close(server_fd);
        return;
    }

    std::cout << "Server is listening on port " << PORT << "..." << std::endl;

    // Accept a client connection
    client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
    if (client_socket < 0) {
        std::cerr << "Accept failed." << std::endl;
        close(server_fd);
        return;
    }

    std::cout << "Client connected." << std::endl;

    // Read message from client
    int bytes_read = read(client_socket, buffer, BUFFER_SIZE);
    std::cout << "Message from client: " << buffer << std::endl;

    // Send response to client
    std::string response = "Hello from server";
    send(client_socket, response.c_str(), response.size(), 0);

    // Close the connection
    close(client_socket);
    close(server_fd);
    std::cout << "Server closed connection." << std::endl;
}

void run_client() {
    int client_socket;
    struct sockaddr_in server_address;
    char buffer[BUFFER_SIZE] = {0};

    // Create socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        std::cerr << "Socket creation failed." << std::endl;
        return;
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);

    // Convert IPv4 address from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) <= 0) {
        std::cerr << "Invalid address or address not supported." << std::endl;
        return;
    }

    // Connect to server
    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        std::cerr << "Connection failed. Please make sure the server is running and listening on port " << PORT << "." << std::endl;
        close(client_socket);
        return;
    }

    std::cout << "Connected to server." << std::endl;

    // Send message to server
    std::string message = "Hello from client";
    send(client_socket, message.c_str(), message.size(), 0);

    // Read response from server
    int bytes_read = read(client_socket, buffer, BUFFER_SIZE);
    std::cout << "Message from server: " << buffer << std::endl;

    // Close the socket
    close(client_socket);
    std::cout << "Client closed connection." << std::endl;
}

int main() {
    int choice;
    std::cout << "Enter 1 to run as server, 2 to run as client: ";
    std::cin >> choice;

    if (choice == 1) {
        run_server();
    } else if (choice == 2) {
        run_client();
    } else {
        std::cerr << "Invalid choice." << std::endl;
    }

    return 0;
}
