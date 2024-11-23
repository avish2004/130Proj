//============================================================================
// Name        : combined.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <fstream>
#include <cstring>
#include <ctime>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>
#include <pthread.h>
#include <vector>
#include <cstdlib>

#define NULL 0

const int PORT = 8080;
const int BUFFER_SIZE = 1024;

std::ofstream log_file("log.txt", std::ios::app); // Open log file in append mode
std::vector<pthread_t> client_threads;

void log_message(const std::string& message) {
    // Get current time
    time_t now = time(0);
    char* dt = ctime(&now); // Convert to string
    dt[strlen(dt) - 1] = '\0'; // Remove newline character
    log_file << "[" << dt << "] " << message << std::endl;
    std::cout << message << std::endl; // Also print to console
}

void* handle_client(void* arg) {
    int client_socket = *(int*)arg;
    delete (int*)arg; // Free the allocated memory
    char buffer[BUFFER_SIZE] = {0};
    log_message("Client connected.");

    while (1) {
        // Read message from client
        int bytes_read = read(client_socket, buffer, BUFFER_SIZE);
        if (bytes_read > 0) {
            log_message("Message received from client: " + std::string(buffer));
        } else if (bytes_read == 0) {
            log_message("Client disconnected.");
            break;
        } else {
            log_message("ERROR: Read failed. Error code: " + std::to_string(errno));
            break;
        }

        // Send response to client
        std::string response = "Hello from server";
        send(client_socket, response.c_str(), response.size(), 0);
        log_message("Response sent to client.");
    }

    // Close the connection
    close(client_socket);
    log_message("Client connection closed.");
    return NULL;
}

void run_server() {
    int server_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    log_message("Starting server...");

    // Create socket file descriptor
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        log_message("ERROR: Socket creation failed. Error code: " + std::to_string(errno));
        return;
    }
    log_message("Socket created successfully.");

    // Set socket options
    const int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        log_message("ERROR: setsockopt failed. Error code: " + std::to_string(errno));
        close(server_fd);
        return;
    }

    // Bind the socket to the port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        log_message("ERROR: Bind failed. Error code: " + std::to_string(errno));
        close(server_fd);
        return;
    }
    log_message("Socket bound to port.");

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        log_message("ERROR: Listen failed. Error code: " + std::to_string(errno));
        close(server_fd);
        return;
    }

    log_message("Server is listening on port " + std::to_string(PORT) + "...");

    while (1) {
        // Accept a client connection
        int* client_socket = new int;
        *client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (*client_socket < 0) {
            log_message("ERROR: Accept failed. Error code: " + std::to_string(errno));
            delete client_socket;
            continue;
        }

        // Create a new thread to handle the client
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, client_socket) != 0) {
            log_message("ERROR: Failed to create thread. Error code: " + std::to_string(errno));
            close(*client_socket);
            delete client_socket;
        } else {
            client_threads.push_back(thread_id);
        }
    }

    // Close the server socket
    close(server_fd);
    log_message("Server closed connection.");
}

void run_client(int client_number) {
    int client_socket;
    struct sockaddr_in server_address;
    char buffer[BUFFER_SIZE] = {0};

    log_message("Starting client " + std::to_string(client_number) + "...");

    // Create socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        log_message("ERROR: Socket creation failed. Error code: " + std::to_string(errno));
        return;
    }
    log_message("Socket created successfully.");

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);

    // Convert IPv4 address from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) <= 0) {
        log_message("ERROR: Invalid address or address not supported. Error code: " + std::to_string(errno));
        return;
    }

    // Connect to server
    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        log_message("ERROR: Connection failed. Please make sure the server is running. Error code: " + std::to_string(errno));
        close(client_socket);
        return;
    }
    log_message("Client " + std::to_string(client_number) + " connected to server.");

    if (client_number == 1) {
        // Client 1 functionality: Send messages to server
        while (1) {
            std::string message;
            std::cout << "Client " << client_number << " - Enter message to send to server (type 'exit' to disconnect): ";
            std::getline(std::cin, message);
            if (message == "exit") {
                break;
            }
            send(client_socket, message.c_str(), message.size(), 0);
            log_message("Client " + std::to_string(client_number) + " sent message to server: " + message);

            int bytes_read = read(client_socket, buffer, BUFFER_SIZE);
            if (bytes_read > 0) {
                log_message("Client " + std::to_string(client_number) + " received message from server: " + std::string(buffer));
            }
        }
    } else if (client_number == 2) {
        // Client 2 functionality: Send random numbers to server
        srand(time(0));
        while (1) {
            int random_number = rand() % 100;
            std::string message = "Random number: " + std::to_string(random_number);
            send(client_socket, message.c_str(), message.size(), 0);
            log_message("Client " + std::to_string(client_number) + " sent message to server: " + message);

            int bytes_read = read(client_socket, buffer, BUFFER_SIZE);
            if (bytes_read > 0) {
                log_message("Client " + std::to_string(client_number) + " received message from server: " + std::string(buffer));
            }
            sleep(1);
        }
    } else if (client_number == 3) {
        // Client 3 functionality: Send current timestamp to server
        while (1) {
            time_t now = time(0);
            char* dt = ctime(&now);
            dt[strlen(dt) - 1] = '\0';
            std::string message = "Current timestamp: " + std::string(dt);
            send(client_socket, message.c_str(), message.size(), 0);
            log_message("Client " + std::to_string(client_number) + " sent message to server: " + message);

            int bytes_read = read(client_socket, buffer, BUFFER_SIZE);
            if (bytes_read > 0) {
                log_message("Client " + std::to_string(client_number) + " received message from server: " + std::string(buffer));
            }
            sleep(2);
        }
    }

    // Keep the connection open
    while (1) {
        sleep(1); // Keep the client connected without sending data
    }

    // Close the socket (this line won't be reached due to the infinite loop)
    close(client_socket);
    log_message("Client " + std::to_string(client_number) + " closed connection.");
}

int main() {
    int choice;
    log_message("Application started.");
    std::cout << "Enter 1 to run as server, 2 as client 1, 3 as client 2, 4 as client 3: ";
    std::cin >> choice;
    std::cin.ignore(); // To consume the newline character left by std::cin

    if (choice == 1) {
        run_server();
    } else if (choice >= 2 && choice <= 4) {
        run_client(choice - 1);
    } else {
        log_message("ERROR: Invalid choice.");
        std::cerr << "Invalid choice." << std::endl;
    }

    // Join all client threads before exiting
    for (unsigned int i = 0; i < client_threads.size(); i++) {
        pthread_join(client_threads[i], NULL);
    }

    log_message("Application exiting.");
    return 0;
}
