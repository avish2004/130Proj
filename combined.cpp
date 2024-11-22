#include <iostream>
#include <fstream>
#include <cstring>
#include <ctime>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>

const int PORT = 8080;
const int BUFFER_SIZE = 1024;

std::ofstream log_file("log.txt", std::ios::app); // Open log file in append mode

void log_message(const std::string& message) {
    // Get current time
    time_t now = time(0);
    char* dt = ctime(&now); // Convert to string
    dt[strlen(dt) - 1] = '\0'; // Remove newline character
    log_file << "[" << dt << "] " << message << std::endl;
    std::cout << message << std::endl; // Also print to console
}

void run_server() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    log_message("Starting server...");

    // Create socket file descriptor
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        log_message("ERROR: Socket creation failed.");
        return;
    }
    log_message("Socket created successfully.");

    // Set socket options
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        log_message("ERROR: setsockopt failed.");
        close(server_fd);
        return;
    }

    // Bind the socket to the port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        log_message("ERROR: Bind failed.");
        close(server_fd);
        return;
    }
    log_message("Socket bound to port.");

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        log_message("ERROR: Listen failed.");
        close(server_fd);
        return;
    }

    log_message("Server is listening on port " + std::to_string(PORT) + "...");

    // Accept a client connection
    client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
    if (client_socket < 0) {
        log_message("ERROR: Accept failed.");
        close(server_fd);
        return;
    }
    log_message("Client connected.");

    // Read message from client
    int bytes_read = read(client_socket, buffer, BUFFER_SIZE);
    if (bytes_read > 0) {
        log_message("Message received from client: " + std::string(buffer));
    }

    // Send response to client
    std::string response = "Hello from server";
    send(client_socket, response.c_str(), response.size(), 0);
    log_message("Response sent to client.");

    // Close the connection
    close(client_socket);
    close(server_fd);
    log_message("Server closed connection.");
}

void run_client() {
    int client_socket;
    struct sockaddr_in server_address;
    char buffer[BUFFER_SIZE] = {0};

    log_message("Starting client...");

    // Create socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        log_message("ERROR: Socket creation failed.");
        return;
    }
    log_message("Socket created successfully.");

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);

    // Convert IPv4 address from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) <= 0) {
        log_message("ERROR: Invalid address or address not supported.");
        return;
    }

    // Connect to server
    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        log_message("ERROR: Connection failed. Please make sure the server is running.");
        close(client_socket);
        return;
    }
    log_message("Connected to server.");

    // Send message to server
    std::string message = "Hello from client";
    send(client_socket, message.c_str(), message.size(), 0);
    log_message("Message sent to server: " + message);

    // Read response from server
    int bytes_read = read(client_socket, buffer, BUFFER_SIZE);
    if (bytes_read > 0) {
        log_message("Message received from server: " + std::string(buffer));
    }

    // Close the socket
    close(client_socket);
    log_message("Client closed connection.");
}

int main() {
    int choice;
    log_message("Application started.");
    std::cout << "Enter 1 to run as server, 2 to run as client: ";
    std::cin >> choice;

    if (choice == 1) {
        run_server();
    } else if (choice == 2) {
        run_client();
    } else {
        log_message("ERROR: Invalid choice.");
        std::cerr << "Invalid choice." << std::endl;
    }

    log_message("Application exiting.");
    return 0;
}
