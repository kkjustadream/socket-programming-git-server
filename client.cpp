// client.cpp
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>
#include <sstream>

#define BUFFER_SIZE 1024
#define SERVER_M_PORT 25207

int main(int argc, char *argv[]) {
    // Check command line arguments
    if (argc != 3) {
        std::cerr << "Usage: ./client <username> <password>" << std::endl;
        return 1;
    }

    // Store credentials
    std::string username = argv[1];
    std::string password = argv[2];

    // Create TCP socket
    int tcpSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcpSocket < 0) {
        std::cerr << "Socket creation failed" << std::endl;
        return 1;
    }

    // Setup server address to connect to
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_M_PORT); // Main server's TCP port
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to main server
    if (connect(tcpSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Connection failed" << std::endl;
        close(tcpSocket);
        return 1;
    }

    // Get client port number after connection
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    if (getsockname(tcpSocket, (struct sockaddr*)&clientAddr, &clientLen) < 0) {
        std::cerr << "Failed to get client port" << std::endl;
        close(tcpSocket);
        return 1;
    }
    int clientPort = ntohs(clientAddr.sin_port);

    std::cout << "The client is up and running." << std::endl;
    // std::cout << "clientPort: " << clientPort << std::endl;

    // Send credentials to server
    std::string auth_msg = username + " " + password;
    send(tcpSocket, auth_msg.c_str(), auth_msg.length(), 0);

    // Receive authentication response
    char buffer[BUFFER_SIZE] = {0};
    ssize_t bytes = recv(tcpSocket, buffer, sizeof(buffer), 0);

    if (bytes <= 0) {
        std::cerr << "Failed to receive authentication response" << std::endl;
        close(tcpSocket);
        return 1;
    }

    std::string response(buffer);


    // Process authentication response
    if (strcmp(buffer, "guest") == 0) {
        std::cout << "You have been granted guest access." << std::endl;
        
        // Guest command loop
        while (true) {
            std::cout << "Please enter the command: <lookup <username>>" << std::endl;
            std::string command;
            std::getline(std::cin, command);

            // Validate guest command
            if (command.substr(0, 6) != "lookup") {
                std::cout << "Guests can only use the lookup command" << std::endl;
                std::cout << "----Start a new request----" << std::endl;
                continue;
            }

            // Send command to server
            send(tcpSocket, command.c_str(), command.length(), 0);

            // Receive response
            memset(buffer, 0, sizeof(buffer));
            bytes = recv(tcpSocket, buffer, sizeof(buffer), 0);
            if (bytes > 0) {
                std::cout << "The client received the response from the main server using TCP over port "
                         << clientPort << std::endl;
                std::cout << buffer << std::endl;
                std::cout << "----Start a new request----" << std::endl;
            }
        }
    } 
    else if (strcmp(buffer, "member") == 0) {
        std::cout << "You have been granted member access" << std::endl;
        
        // Member command loop
        while (true) {
            std::cout << "Please enter the command: <lookup <username>> , <push <filename>> , "
                     << "<remove <filename>> , <deploy>, <log>" << std::endl;
            std::string command;
            std::getline(std::cin, command);

            // Send command to serverM
            send(tcpSocket, command.c_str(), command.length(), 0);

            // Extract filename and username
            std::istringstream iss(command);
            std::string cmd, filename;
            iss >> cmd >> filename;

            if (cmd == "lookup") {
                if (filename.empty()) {
                    std::cout << "Username is not specified. Will lookup " << username << "." << std::endl;
                }
                std::cout << username << " sent a lookup request to the main server." << std::endl;
            }

            // Receive response
            memset(buffer, 0, sizeof(buffer));
            bytes = recv(tcpSocket, buffer, sizeof(buffer), 0);

            if (bytes > 0) {            
                std::string response(buffer);
                if (response == "exists") {
                    // Print overwrite confirmation prompt
                    std::cout << filename << " exists in " << username 
                        << "'s repository, do you want to overwrite (Y/N)? ";
                
                    // Get user's response
                    std::string confirm;
                    std::getline(std::cin, confirm);

                    // Send confirmation back to serverM
                    send(tcpSocket, confirm.c_str(), confirm.length(), 0);

                    // Wait for final response
                    memset(buffer, 0, sizeof(buffer));
                    bytes = recv(tcpSocket, buffer, sizeof(buffer), 0);
                    response = buffer;
                }

                if (cmd == "lookup") {
                    std::cout << "The client received the response from the main server using TCP over port " << clientPort << std::endl;
                    std::cout << response << std::endl;
                }
                else if (cmd == "push") {
                    if (filename.empty()) {
                        std::cout << "Error: Filename is required. Please specify a filename to push." << std::endl;
                    }
                    else if (response == "success") {
                        std::cout << filename << " pushed successfully" << std::endl;
                    } 
                    else {
                        std::cout << filename << " was not pushed successfully" << std::endl;
                    }
                }
                else if (cmd == "deploy") {
                    std::cout << username << " sent a lookup request to the main server." << std::endl;
                    std::cout << "The client received the response from the main server using TCP over port " << clientPort << std::endl;
                    if (response == "Empty repository.") {
                        std::cout << response << std::endl;
                    } else {
                        // Response already contains "The following files..." header
                        // Parse and print each line
                        std::istringstream iss(response);
                        std::string line;
                        
                        // Print the header line
                        std::getline(iss, line);
                        std::cout << line << std::endl;  // Prints "The following files..."
                        
                        // Print each filename
                        while (std::getline(iss, line)) {
                            if (!line.empty()) {
                                std::cout << line << std::endl;
                            }
                        }
                    }
                }
                else if (cmd == "remove") {
                    std::cout << username << " sent a remove request to the main server." << std::endl;
                    std::cout << response << std::endl;
                }
                else if (cmd == "log") {
                    std::cout << username << " sent a log request to the main server." << std::endl;
                    std::cout << "The client received the response from the main server using TCP over port "
                            << clientPort << std::endl;
                    std::cout << response << std::endl;
                }
                else {
                    std::cout << response << std::endl;
                }

                std::cout << "----Start a new request----" << std::endl;
            }
        }
    }
    else {
        std::cout << "The credentials are incorrect. Please try again." << std::endl;
    }

    close(tcpSocket);
    return 0;
}