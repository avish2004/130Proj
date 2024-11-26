//============================================================================
// Name        : combined.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream> // Provides input and output stream (e.g., std::cout for console output)
#include <fstream>  // Provides file handling capabilities (e.g., for logging events to a file)
#include <cstring>  // Provides C-style string manipulation functions like strlen()
#include <ctime>    // Provides time-related functions to get current time (used for logging)
#include <sys/socket.h> // Provides definitions for sockets used in network communication
#include <arpa/inet.h>  // Provides functions for handling IP addresses (e.g., inet_pton for address conversion)
#include <unistd.h>     // Provides access to POSIX OS API (e.g., close() to close sockets)
#include <netinet/in.h> // Provides definitions for internet addresses (used for socket address structures)
#include <errno.h>      // Provides error handling functionality (e.g., errno to get error codes)
#include <pthread.h>    // Provides threading capabilities using POSIX threads (pthreads)
#include <vector>       // Provides the vector container from the C++ Standard Library
#include <cstdlib>      // Provides general purpose functions, including random number generation

#define NULL 0 // Define NULL to represent a null pointer (standard practice before C++11)

const int PORT = 8080; // Port number to which the server will bind and listen
const int BUFFER_SIZE = 1024; // Buffer size for receiving messages

// Open a log file in append mode to keep track of server and client messages
std::ofstream log_file("log.txt", std::ios::app);

// Vector to keep track of client threads (so they can be joined or managed later)
std::vector<pthread_t> client_threads;

// Function to log messages to both a log file and the console
void log_message(const std::string& message) {
    // Get current time
    time_t now = time(0);
    char* dt = ctime(&now); // Convert to string representation of current time
    dt[strlen(dt) - 1] = '\0'; // Remove newline character from the end of the time string

    // Write the log message to the file and also print to the console
    log_file << "[" << dt << "] " << message << std::endl;
    std::cout << message << std::endl;
}

// Helper function to convert an integer to a string (as an alternative to std::to_string in pre-C++11)
std::string int_to_string(int number) {
    char buffer[50]; // Buffer to hold the converted number
    sprintf(buffer, "%d", number); // Use sprintf to convert number to string format
    return std::string(buffer); // Return the buffer as a std::string
}

// Function to handle communication with a connected client
void* handle_client(void* arg) {
    int client_socket = *(int*)arg; // Extract the client socket from the argument
    delete (int*)arg; // Free the allocated memory for the client socket
    char buffer[BUFFER_SIZE] = {0}; // Buffer to hold the incoming message
    log_message("Client connected."); // Log the client connection

    while (1) {
        // Read message from client
        int bytes_read = read(client_socket, buffer, BUFFER_SIZE);
        if (bytes_read > 0) {
            log_message("Message received from client: " + std::string(buffer)); // Log the received message
        } else if (bytes_read == 0) {
            log_message("Client disconnected."); // Log when client disconnects
            break; // Exit loop since the client disconnected
        } else {
            log_message("ERROR: Read failed. Error code: " + int_to_string(errno)); // Log error if read fails
            break; // Exit loop due to error
        }

        // Send response back to client
        std::string response = "Hello from server";
        send(client_socket, response.c_str(), response.size(), 0); // Send response to client
        log_message("Response sent to client."); // Log the sent response
    }

    // Close the connection with the client
    close(client_socket);
    log_message("Client connection closed."); // Log that the connection is closed
    return NULL; // Return NULL since the function returns void*
}

// Function to start and run the server
void run_server() {
    int server_fd; // File descriptor for server socket
    struct sockaddr_in address; // Structure to hold server address information
    int addrlen = sizeof(address); // Length of the address structure

    log_message("Starting server...");

    // Create socket file descriptor (IPv4, TCP)
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        log_message("ERROR: Socket creation failed. Error code: " + int_to_string(errno));
        return; // Exit if socket creation fails
    }
    log_message("Socket created successfully.");

    // Set socket options to reuse address
    const int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        log_message("ERROR: setsockopt failed. Error code: " + int_to_string(errno));
        close(server_fd);
        return; // Exit if setting socket options fails
    }

    // Bind the socket to the port and IP address
    address.sin_family = AF_INET; // Address family: IPv4
    address.sin_addr.s_addr = INADDR_ANY; // Bind to all available network interfaces
    address.sin_port = htons(PORT); // Convert port number to network byte order

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        log_message("ERROR: Bind failed. Error code: " + int_to_string(errno));
        close(server_fd);
        return; // Exit if binding fails
    }
    log_message("Socket bound to port.");

    // Start listening for incoming connections (max queue size 3)
    if (listen(server_fd, 3) < 0) {
        log_message("ERROR: Listen failed. Error code: " + int_to_string(errno));
        close(server_fd);
        return; // Exit if listen fails
    }

    log_message("Server is listening on port " + int_to_string(PORT) + "...");

    // Accept client connections in a loop
    while (1) {
        int* client_socket = new int; // Allocate memory for client socket
        *client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (*client_socket < 0) {
            log_message("ERROR: Accept failed. Error code: " + int_to_string(errno));
            delete client_socket; // Free memory if accept fails
            continue; // Continue to the next iteration if accept fails
        }

        // Create a new thread to handle the client
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, client_socket) != 0) {
            log_message("ERROR: Failed to create thread. Error code: " + int_to_string(errno));
            close(*client_socket);
            delete client_socket; // Clean up if thread creation fails
        } else {
            client_threads.push_back(thread_id); // Store the thread ID for later management
        }
    }

    // Close the server socket when done
    close(server_fd);
    log_message("Server closed connection.");
}

// Function to start and run a client
void run_client(int client_number) {
    int client_socket; // File descriptor for client socket
    struct sockaddr_in server_address; // Structure to hold server address
    char buffer[BUFFER_SIZE] = {0}; // Buffer to hold server response

    log_message("Starting client " + int_to_string(client_number) + "...");

    // Create socket (IPv4, TCP)
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        log_message("ERROR: Socket creation failed. Error code: " + int_to_string(errno));
        return; // Exit if socket creation fails
    }
    log_message("Socket created successfully.");

    server_address.sin_family = AF_INET; // Set address family to IPv4
    server_address.sin_port = htons(PORT); // Set port number

    // Convert server IP address from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) <= 0) {
        log_message("ERROR: Invalid address or address not supported. Error code: " + int_to_string(errno));
        return; // Exit if address conversion fails
    }

    // Connect to the server
    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        log_message("ERROR: Connection failed. Please make sure the server is running. Error code: " + int_to_string(errno));
        close(client_socket);
        return; // Exit if connection fails
    }
    log_message("Client " + int_to_string(client_number) + " connected to server.");

    // Send a message to the server
    std::string message = "Hello from client " + int_to_string(client_number);
    send(client_socket, message.c_str(), message.size(), 0);
    log_message("Client " + int_to_string(client_number) + " sent message to server: " + message);

    // Read the server's response
    int bytes_read = read(client_socket, buffer, BUFFER_SIZE);
    if (bytes_read > 0) {
        log_message("Client " + int_to_string(client_number) + " received message from server: " + std::string(buffer));
    }

    // Close the socket
    close(client_socket);
    log_message("Client " + int_to_string(client_number) + " closed connection.");
}

// Wrapper function to start the server in a new thread
void* start_server(void* arg) {
    run_server(); // Start the server
    return NULL; // Return NULL since the function returns void*
}

// Wrapper function to start the client in a new thread
void* start_client(void* arg) {
    int client_number = *(int*)arg; // Get the client number from argument
    run_client(client_number); // Start the client
    return NULL; // Return NULL since the function returns void*
}

// Main function to run the application
int main() {
    log_message("Application started.");

    int choice;
    // Ask the user whether to run the server, client, or both
    std::cout << "Enter 1 to run server and client concurrently, 2 to run as server only, 3 as client only: ";
    std::cin >> choice;
    std::cin.ignore(); // Consume the newline left by std::cin

    pthread_t server_thread, client_thread; // Thread identifiers for server and client

    // Run the server if user chooses to run server or both
    if (choice == 1 || choice == 2) {
        if (pthread_create(&server_thread, NULL, start_server, NULL) != 0) {
            log_message("ERROR: Failed to create server thread.");
            return 1; // Exit with an error code if thread creation fails
        }
        log_message("Server thread started.");
    }

    // Run the client if user chooses to run client or both
    if (choice == 1 || choice == 3) {
        int client_number; // Get the client number from the user
        std::cout << "Enter client number (1, 2, or 3): ";
        std::cin >> client_number;
        std::cin.ignore();

        if (pthread_create(&client_thread, NULL, start_client, &client_number) != 0) {
            log_message("ERROR: Failed to create client thread.");
            return 1; // Exit with an error code if thread creation fails
        }
        log_message("Client thread started.");
    }

    // Wait for the server thread to complete
    if (choice == 1 || choice == 2) {
        pthread_join(server_thread, NULL);
    }

    // Wait for the client thread to complete
    if (choice == 1 || choice == 3) {
        pthread_join(client_thread, NULL);
    }

    log_message("Application exiting.");
    return 0; // Return success
}
