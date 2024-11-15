// serverA.cpp
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <fstream>
#include <sstream>
#include <string>

#define SERVER_A_PORT 21207

// Function to encrypt password according to the scheme
std::string encryptPassword(const std::string& password) {
    std::string encrypted = password;
    for (size_t i = 0; i < encrypted.length(); i++) {
        char& c = encrypted[i];
        if (isalpha(c)) {
            // Handle alphabetic characters (A-Z, a-z)
            char base = isupper(c) ? 'A' : 'a';
            c = base + (c - base + 3) % 26;
        }
        else if (isdigit(c)) {
            // Handle digits (0-9)
            c = '0' + (c - '0' + 3) % 10;
        }
        // Special characters remain unchanged
    }
    return encrypted;
}

// Function to authenticate user
bool authenticateUser(const std::string& username, const std::string& password) {
    std::ifstream file("members.txt");
    if (!file.is_open()) {
        std::cerr << "Error opening members.txt" << std::endl;
        return false;
    }

    std::string line;
    std::string encrypted_pass = encryptPassword(password);
    // std::cout << "Debug: Original password: " << password << std::endl;
    // std::cout << "Debug: Encrypted password: " << encrypted_pass << std::endl;

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string stored_username, stored_password;
        
        if (iss >> stored_username >> stored_password) {
            if (stored_username == username && stored_password == encrypted_pass) {
                file.close();
                return true;
            }
        }
    }
    file.close();
    return false;
}

int main()
{
    // Create UDP socket
    int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket < 0)
    {
        std::cerr << "UDP socket creation failed" << std::endl;
        return 1;
    }

    // Setup address
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_A_PORT); // Your port based on USC ID
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    // Bind socket
    if (bind(udpSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        std::cerr << "Bind failed" << std::endl;
        close(udpSocket);
        return 1;
    }

    std::cout << "Server A is up and running using UDP on port 21207" << std::endl;

    // Main server loop
    while (true)
    {
        char buffer[1024] = {0};
        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);

        // Wait for messages from Main Server
        ssize_t bytesReceived = recvfrom(udpSocket, buffer, sizeof(buffer), 0,
                                         (struct sockaddr *)&clientAddr, &clientLen);

        if (bytesReceived > 0)
        {
            buffer[bytesReceived] = '\0';
            // Process authentication request
            // 1. Read from members.txt
            // Parse username and password from received message
            std::string msg(buffer);
            std::istringstream iss(msg);
            std::string username, password;
            iss >> username >> password;            
            // Print received authentication request
            std::string hidden_password(password.length(), '*');
            std::cout << "ServerA received username " << username 
                      << " and password " << hidden_password << std::endl;

            // 2. Check credentials
            // Authenticate user
            bool isAuthenticated = authenticateUser(username, password);
            // 3. Send response back to Main Server            
            std::string response;
            if (isAuthenticated) {
                std::cout << "Member " << username << " has been authenticated" << std::endl;
                response = "member";
            } else {
                std::cout << "The username " << username << " or password "
                         << hidden_password << " is incorrect" << std::endl;
                response = "invalid";
            }
            sendto(udpSocket, response.c_str(), response.length(), 0,
                    (struct sockaddr*)&clientAddr, clientLen);
        }
    }

    // Cleanup (this won't be reached unless you break the loop)
    close(udpSocket);
    return 0;
}