// serverM.cpp
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sstream>
#include <map>

#define TCP_PORT 25207
#define UDP_PORT 24207
#define BUFFER_SIZE 1024

void handleLookupRequest(int udpSocket, int clientSocket, const std::string& request, 
                        const std::string& username, bool isGuest);
void handlePushRequest(int udpSocket, int clientSocket, const std::string& request,
                      const std::string& username);
void handleDeployRequest(int udpSocket, int clientSocket, const std::string& request,
                        const std::string& username);
void handleRemoveRequest(int udpSocket, int clientSocket, const std::string& request,
                        const std::string& username);

// Structure to track client information
struct ClientInfo {
    bool isGuest;
    std::string username;
};
// Global map to track authenticated clients
std::map<int, ClientInfo> authenticatedClients;

int main() {
    // Create UDP socket for backend servers
    int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket < 0) {
        std::cerr << "UDP socket creation failed" << std::endl;
        return 1;
    }

    // Create TCP socket for client communication
    int tcpSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcpSocket < 0) {
        std::cerr << "TCP socket creation failed" << std::endl;
        return 1;
    }

    // Setup UDP address
    struct sockaddr_in udpAddr;
    memset(&udpAddr, 0, sizeof(udpAddr));
    udpAddr.sin_family = AF_INET;
    udpAddr.sin_port = htons(UDP_PORT);
    udpAddr.sin_addr.s_addr = INADDR_ANY;

    // Setup TCP address
    struct sockaddr_in tcpAddr;
    memset(&tcpAddr, 0, sizeof(tcpAddr));
    tcpAddr.sin_family = AF_INET;
    tcpAddr.sin_port = htons(TCP_PORT);
    tcpAddr.sin_addr.s_addr = INADDR_ANY;

    // Bind sockets
    if (bind(udpSocket, (struct sockaddr*)&udpAddr, sizeof(udpAddr)) < 0) {
        std::cerr << "UDP bind failed" << std::endl;
        return 1;
    }

    if (bind(tcpSocket, (struct sockaddr*)&tcpAddr, sizeof(tcpAddr)) < 0) {
        std::cerr << "TCP bind failed" << std::endl;
        return 1;
    }

    // Listen for TCP connections
    if (listen(tcpSocket, 5) < 0) {
        std::cerr << "TCP listen failed" << std::endl;
        return 1;
    }

    std::cout << "Server M is up and running using UDP on port 24207" << std::endl;



    


    // Main server loop
    while (true) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(udpSocket, &readfds);
        FD_SET(tcpSocket, &readfds);
        int maxfd = std::max(udpSocket, tcpSocket);

        // Wait for activity on either socket
        int activity = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        
        if (activity < 0) {
            std::cerr << "Select error" << std::endl;
            continue;
        }

        // Check UDP messages from backend servers
        if (FD_ISSET(udpSocket, &readfds)) {
            char buffer[BUFFER_SIZE] = {0};
            struct sockaddr_in senderAddr;
            socklen_t senderLen = sizeof(senderAddr);
            
            ssize_t udpBytes = recvfrom(udpSocket, buffer, sizeof(buffer), 0, 
                                    (struct sockaddr*)&senderAddr, &senderLen);
            
            if (udpBytes > 0) {
                buffer[udpBytes] = '\0';
                int senderPort = ntohs(senderAddr.sin_port);
 
                // Process responses from different servers
                if (senderPort == 21207) { // Server A
                    std::cout << "The main server has received the response from server A using UDP over "
                             << UDP_PORT << std::endl;
                }
                else if (senderPort == 22207) { // Server R
                    std::cout << "The main server has received the response from server R using UDP over "
                             << UDP_PORT << std::endl;
                }
                else if (senderPort == 23207) { // Server D
                    std::cout << "The user's repository has been deployed at server D." << std::endl;
                }
            }
        }

        // Check TCP connections from clients
        if (FD_ISSET(tcpSocket, &readfds)) {
            struct sockaddr_in clientAddr;
            socklen_t clientLen = sizeof(clientAddr);
            int clientSocket = accept(tcpSocket, (struct sockaddr*)&clientAddr, &clientLen);
            
            if (clientSocket < 0) {
                std::cerr << "Accept failed" << std::endl;
                continue;
            }
            std::cout << "Server M received connection from client" << std::endl;

            // Keep connection alive for multiple commands
            while (true) {
                // Handle client message
                char buffer[BUFFER_SIZE] = {0};
                ssize_t tcpBytes = recv(clientSocket, buffer, sizeof(buffer), 0);
                if (tcpBytes <= 0) {
                    std::cout << "Client disconnected" << std::endl;
                    authenticatedClients.erase(clientSocket);
                    break;
                }
                buffer[tcpBytes] = '\0';
                std::string message(buffer);
                std::istringstream iss(message);
                std::string first_word;
                iss >> first_word;    
                
                // Debug
                // std::cout << "\n=== Command Debug Info ===" << std::endl;
                // std::cout << "Raw message: '" << message << "'" << std::endl;
                // std::cout << "Command: '" << first_word << "'" << std::endl;
                // std::cout << "======================" << std::endl;

                // Check if this is a command or authentication request
                if (first_word == "lookup" || first_word == "push" || 
                    first_word == "deploy" || first_word == "remove") {
                    // Verify client is authenticated
                    if (authenticatedClients.find(clientSocket) == authenticatedClients.end()) {
                        std::string error = "Error: Not authenticated";
                        send(clientSocket, error.c_str(), error.length(), 0);
                        continue;
                    }
                    ClientInfo& client = authenticatedClients[clientSocket];
                    // Handle repository operations
                    std::string param;
                    iss >> param;  // Get second parameter (username or filename)

                    if (first_word == "lookup") {
                        if (param.empty()) {
                            std::string error = "Error: Username is required. Please specify a username to lookup.";
                            send(clientSocket, error.c_str(), error.length(), 0);
                        } else {
                            handleLookupRequest(udpSocket, clientSocket, message, param, client.isGuest); // Set isGuest based on authentication
                        }
                    }
                    // Member-only commands
                    else if (!client.isGuest) {
                        if (first_word == "push") {
                            if (param.empty()) {
                                std::string error = "Error: Filename is required. Please specify a filename to push.";
                                send(clientSocket, error.c_str(), error.length(), 0);
                            } else {
                                handlePushRequest(udpSocket, clientSocket, message, "username"); // Replace with actual username
                            }
                        }
                        else if (first_word == "deploy") {
                            handleDeployRequest(udpSocket, clientSocket, message, "username"); // Replace with actual username
                        }
                        else if (first_word == "remove") {
                            if (param.empty()) {
                                std::string error = "Error: Filename is required for remove operation.";
                                send(clientSocket, error.c_str(), error.length(), 0);
                            } else {
                                handleRemoveRequest(udpSocket, clientSocket, message, "username"); // Replace with actual username
                            }
                        }
                    }
                    else {
                        std::string error = "Guests can only use the lookup command";
                        send(clientSocket, error.c_str(), error.length(), 0);
                    }
                } 
                else {
                    // authentication request
                    size_t space = message.find(' ');
                    if (space == std::string::npos) {
                        std::string error = "Invalid credentials format";
                        send(clientSocket, error.c_str(), error.length(), 0);
                        continue;
                    }

                    std::string username = message.substr(0, space);
                    std::string password = message.substr(space + 1);
                    
                    // Print received credentials (hide password)
                    std::string hidden_password(password.length(), '*');
                    std::cout << "Server M has received username " << username 
                            << " and password " << hidden_password << "." << std::endl;

                    // Handle guest authentication
                    if (username == "guest" && password == "guest") {
                        ClientInfo client;
                        client.isGuest = true;
                        client.username = "guest";
                        authenticatedClients[clientSocket] = client;
                        send(clientSocket, "guest", 5, 0);
                        continue;
                    }

                    // Forward authentication request to Server A
                    std::cout << "Server M has sent authentication request to Server A" << std::endl;
                    
                    struct sockaddr_in serverA_addr;
                    serverA_addr.sin_family = AF_INET;
                    serverA_addr.sin_port = htons(21207);
                    serverA_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

                    sendto(udpSocket, buffer, strlen(buffer), 0, 
                        (struct sockaddr*)&serverA_addr, sizeof(serverA_addr));

                    // Wait for Server A's response
                    struct sockaddr_in responseAddr;
                    socklen_t responseLen = sizeof(responseAddr);
                    char response[BUFFER_SIZE] = {0};
                    
                    ssize_t bytes = recvfrom(udpSocket, response, sizeof(response), 0,
                                            (struct sockaddr*)&responseAddr, &responseLen);
                    
                    std::cout << "The main server has received the response from server A using UDP over "
                            << UDP_PORT << std::endl;

                    // Store client info if authentication successful
                    if (strcmp(response, "member") == 0) {
                        ClientInfo client;
                        client.isGuest = false;
                        client.username = username;
                        authenticatedClients[clientSocket] = client;
                    }

                    // Forward response to client
                    send(clientSocket, response, bytes, 0);
                    
                    std::cout << "The main server has sent the response from server A to client using TCP over port "
                            << TCP_PORT << "." << std::endl;
                }
            }
            close(clientSocket);
        }

    }

    // Cleanup (this part won't be reached unless you break the loop)
    close(udpSocket);
    close(tcpSocket);
    return 0;
}


void handleLookupRequest(int udpSocket, int clientSocket, const std::string& request, 
                        const std::string& username, bool isGuest) {
    // Print appropriate message based on user type
    if (isGuest) {
        std::cout << "The main server has received a lookup request from Guest to lookup "
                  << username << "'s repository using TCP over port " << TCP_PORT << "." << std::endl;
    } else {
        std::cout << "The main server has received a lookup request from " << username 
                  << " to lookup " << username << "'s repository using TCP over port " 
                  << TCP_PORT << "." << std::endl;
    }

    struct sockaddr_in serverR_addr;
    serverR_addr.sin_family = AF_INET;
    serverR_addr.sin_port = htons(22207);  // Server R port
    serverR_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    std::cout << "The main server has sent the lookup request to server R." << std::endl;

    // Forward request to Server R
    sendto(udpSocket, request.c_str(), request.length(), 0,
           (struct sockaddr*)&serverR_addr, sizeof(serverR_addr));

    // Get response from Server R
    char buffer[BUFFER_SIZE] = {0};
    struct sockaddr_in responseAddr;
    socklen_t responseLen = sizeof(responseAddr);
    
    ssize_t bytes = recvfrom(udpSocket, buffer, BUFFER_SIZE, 0,
                            (struct sockaddr*)&responseAddr, &responseLen);

    std::cout << "The main server has received the response from server R using UDP over "
              << UDP_PORT << std::endl;

    // Forward response to client
    send(clientSocket, buffer, bytes, 0);
    std::cout << "The main server has sent the response to the client." << std::endl;
}

void handlePushRequest(int udpSocket, int clientSocket, const std::string& request,
                      const std::string& username) {
    struct sockaddr_in serverR_addr;
    serverR_addr.sin_family = AF_INET;
    serverR_addr.sin_port = htons(22207);
    serverR_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    std::cout << "The main server has received a push request from " << username 
              << ", using TCP over port " << TCP_PORT << "." << std::endl;

    std::cout << "The main server has sent the push request to server R." << std::endl;

    // Forward request to Server R
    sendto(udpSocket, request.c_str(), request.length(), 0,
           (struct sockaddr*)&serverR_addr, sizeof(serverR_addr));

    // Get response from Server R
    char buffer[BUFFER_SIZE] = {0};
    struct sockaddr_in responseAddr;
    socklen_t responseLen = sizeof(responseAddr);
    
    ssize_t bytes = recvfrom(udpSocket, buffer, BUFFER_SIZE, 0,
                            (struct sockaddr*)&responseAddr, &responseLen);

    std::string response(buffer);
    std::cout << "The main server has received the response from server R using UDP over "
              << UDP_PORT << std::endl;

    if (response == "exists") {
        // Handle overwrite confirmation
        std::cout << "The main server has sent the overwrite confirmation request to the client." << std::endl;
        send(clientSocket, buffer, bytes, 0);

        // Wait for client's response
        memset(buffer, 0, BUFFER_SIZE);
        bytes = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        
        std::cout << "The main server has received the overwrite confirmation response from "
                  << username << " using TCP over port " << TCP_PORT << std::endl;

        // Forward confirmation to Server R
        std::cout << "The main server has sent the overwrite confirmation response to server R." << std::endl;
        sendto(udpSocket, buffer, bytes, 0,
               (struct sockaddr*)&serverR_addr, sizeof(serverR_addr));
    } else {
        // Forward response to client
        send(clientSocket, buffer, bytes, 0);
        std::cout << "The main server has sent the response to the client." << std::endl;
    }
}

void handleDeployRequest(int udpSocket, int clientSocket, const std::string& request,
                        const std::string& username) {
    // First contact Server R to get files
    struct sockaddr_in serverR_addr;
    serverR_addr.sin_family = AF_INET;
    serverR_addr.sin_port = htons(22207);
    serverR_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    std::cout << "The main server has received a deploy request from member " << username 
              << " TCP over port " << TCP_PORT << "." << std::endl;

    // Forward request to Server R
    std::cout << "The main server has sent the lookup request to server R." << std::endl;
    sendto(udpSocket, request.c_str(), request.length(), 0,
           (struct sockaddr*)&serverR_addr, sizeof(serverR_addr));

    // Get response from Server R
    char buffer[BUFFER_SIZE] = {0};
    struct sockaddr_in responseAddr;
    socklen_t responseLen = sizeof(responseAddr);
    
    ssize_t bytes = recvfrom(udpSocket, buffer, BUFFER_SIZE, 0,
                            (struct sockaddr*)&responseAddr, &responseLen);

    std::cout << "The main server received the lookup response from server R." << std::endl;

    // Forward to Server D if files exist
    if (strcmp(buffer, "empty") != 0) {
        struct sockaddr_in serverD_addr;
        serverD_addr.sin_family = AF_INET;
        serverD_addr.sin_port = htons(23207);
        serverD_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

        std::string deployRequest = username + " " + std::string(buffer);
        std::cout << "The main server has sent the deploy request to server D." << std::endl;
        
        sendto(udpSocket, deployRequest.c_str(), deployRequest.length(), 0,
               (struct sockaddr*)&serverD_addr, sizeof(serverD_addr));

        // Get confirmation from Server D
        memset(buffer, 0, BUFFER_SIZE);
        bytes = recvfrom(udpSocket, buffer, BUFFER_SIZE, 0,
                        (struct sockaddr*)&responseAddr, &responseLen);

        std::cout << "The user " << username << "'s repository has been deployed at server D." << std::endl;
    }

    // Send final response to client
    send(clientSocket, buffer, bytes, 0);
}

void handleRemoveRequest(int udpSocket, int clientSocket, const std::string& request,
                        const std::string& username) {
    // Print initial message
    std::cout << "The main server has received a remove request from member " 
              << username << " TCP over port " << TCP_PORT << "." << std::endl;

    // Forward request to Server R
    struct sockaddr_in serverR_addr;
    serverR_addr.sin_family = AF_INET;
    serverR_addr.sin_port = htons(22207);  // Server R port
    serverR_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Extract filename from request
    std::istringstream iss(request);
    std::string command, filename;
    iss >> command >> filename;

    // Send request to Server R
    sendto(udpSocket, request.c_str(), request.length(), 0,
           (struct sockaddr*)&serverR_addr, sizeof(serverR_addr));

    // Wait for Server R's response
    char buffer[1024] = {0};
    struct sockaddr_in responseAddr;
    socklen_t responseLen = sizeof(responseAddr);
    
    ssize_t bytes = recvfrom(udpSocket, buffer, sizeof(buffer), 0,
                            (struct sockaddr*)&responseAddr, &responseLen);

    if (bytes > 0) {
        buffer[bytes] = '\0';
        std::string response(buffer);

        std::cout << "The main server has received confirmation of the remove request done by the server R" 
                  << std::endl;

        // Forward response to client
        if (response == "success") {
            std::string successMsg = "The remove request was successful.";
            send(clientSocket, successMsg.c_str(), successMsg.length(), 0);
        } else {
            // File not found or other error
            std::string errorMsg = filename + " does not exist in your repository.";
            send(clientSocket, errorMsg.c_str(), errorMsg.length(), 0);
        }
    }
}
