// client.cpp
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>

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
    serverAddr.sin_port = htons(25207); // Main server's TCP port
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to main server
    if (connect(tcpSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Connection failed" << std::endl;
        close(tcpSocket);
        return 1;
    }

    std::cout << "The client is up and running." << std::endl;

    // Send credentials to server
    std::string auth_msg = username + " " + password;
    send(tcpSocket, auth_msg.c_str(), auth_msg.length(), 0);

    // Receive authentication response
    char buffer[1024] = {0};
    recv(tcpSocket, buffer, sizeof(buffer), 0);

    // Process authentication response
    if (strcmp(buffer, "guest") == 0) {
        std::cout << "You have been granted guest access." << std::endl;
        
        // Guest command loop
        while (true) {
            std::cout << "Please enter the command: <lookup <username>>" << std::endl;
            std::string command;
            std::getline(std::cin, command);

            // Send command to server
            send(tcpSocket, command.c_str(), command.length(), 0);

            // Receive response
            memset(buffer, 0, sizeof(buffer));
            recv(tcpSocket, buffer, sizeof(buffer), 0);
            std::cout << buffer << std::endl;
            std::cout << "----Start a new request----" << std::endl;
        }
    } 
    else if (strcmp(buffer, "member") == 0) {
        std::cout << "You have been granted member access" << std::endl;
        
        // Member command loop
        while (true) {
            std::cout << "Please enter the command: <lookup <username>> , <push <filename>> , "
                     << "<remove <filename>> , <deploy>" << std::endl;
            std::string command;
            std::getline(std::cin, command);

            // Send command to server
            send(tcpSocket, command.c_str(), command.length(), 0);

            // Receive response
            memset(buffer, 0, sizeof(buffer));
            recv(tcpSocket, buffer, sizeof(buffer), 0);
            std::cout << buffer << std::endl;
            std::cout << "----Start a new request----" << std::endl;
        }
    }
    else {
        std::cout << "The credentials are incorrect. Please try again." << std::endl;
        close(tcpSocket);
        return 1;
    }

    close(tcpSocket);
    return 0;
}