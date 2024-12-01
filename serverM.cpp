// serverM.cpp
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sstream>
#include <map>
#include <set>
#include <vector>
#include <ctime>

#define TCP_PORT 25207
#define UDP_PORT 24207
#define SERVER_R_PORT 22207
#define BUFFER_SIZE 1024

void handleLookupRequest(int udpSocket, int clientSocket, const std::string& request, 
                        const std::string& username, bool isGuest);
void handlePushRequest(int udpSocket, int clientSocket, const std::string& request,
                      const std::string& username);
void handleDeployRequest(int udpSocket, int clientSocket, const std::string& request,
                        const std::string& username);
void handleRemoveRequest(int udpSocket, int clientSocket, const std::string& request,
                        const std::string& username);
void handleLogRequest(int clientSocket, const std::string& username);
void addToLog(const std::string& username, const std::string& operation);

// Structure to track client information
struct ClientInfo {
    bool isGuest;
    std::string username;
};

// Track authenticated clients
std::map<int, ClientInfo> authenticatedClients;

struct LogEntry {
    std::string username;
    std::string operation;
    std::string timestamp;
};
std::map<std::string, std::vector<LogEntry>> userLogs;  // username -> vector of operations

int main() {
    // Create UDP socket for backend servers(from Beej's)
    int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket < 0) {
        std::cerr << "UDP socket creation failed" << std::endl;
        return 1;
    }

    // Create TCP socket for client communication(from Beej's)
    int tcpSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcpSocket < 0) {
        std::cerr << "TCP socket creation failed" << std::endl;
        return 1;
    }

    // Setup UDP address(from Beej's)
    struct sockaddr_in udpAddr;
    memset(&udpAddr, 0, sizeof(udpAddr));
    udpAddr.sin_family = AF_INET;
    udpAddr.sin_port = htons(UDP_PORT);
    udpAddr.sin_addr.s_addr = INADDR_ANY;

    // Setup TCP address(from Beej's)
    struct sockaddr_in tcpAddr;
    memset(&tcpAddr, 0, sizeof(tcpAddr));
    tcpAddr.sin_family = AF_INET;
    tcpAddr.sin_port = htons(TCP_PORT);
    tcpAddr.sin_addr.s_addr = INADDR_ANY;

    // Bind sockets(from Beej's)
    if (bind(udpSocket, (struct sockaddr*)&udpAddr, sizeof(udpAddr)) < 0) {
        std::cerr << "UDP bind failed" << std::endl;
        return 1;
    }

    if (bind(tcpSocket, (struct sockaddr*)&tcpAddr, sizeof(tcpAddr)) < 0) {
        std::cerr << "TCP bind failed: " << strerror(errno) << std::endl;
        return 1;
    }

    // Listen for TCP connections(from Beej's)
    if (listen(tcpSocket, 5) < 0) {
        std::cerr << "TCP listen failed" << std::endl;
        return 1;
    }

    std::cout << "Server M is up and running using UDP on port 24207" << std::endl;


    std::set<int> clientSockets;  // To track all client connections


    // Main server loop
    while (true) {
        // (from Beej's)
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(udpSocket, &readfds);
        FD_SET(tcpSocket, &readfds);

        // Add all client sockets to the set
        for (int sock : clientSockets) {
            FD_SET(sock, &readfds);
        }
        
        // Calculate max fd including client sockets for the usage of select
        int maxfd = std::max(udpSocket, tcpSocket);
        for (int sock : clientSockets) {
            maxfd = std::max(maxfd, sock);
        }

        // Wait for activity on either socket(from Beej's)
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
            
            // Stores sender's address information in senderAddr(from Beej's)
            ssize_t udpBytes = recvfrom(udpSocket, buffer, sizeof(buffer), 0, 
                                    (struct sockaddr*)&senderAddr, &senderLen);
            
            if (udpBytes > 0) {
                buffer[udpBytes] = '\0';
                int senderPort = ntohs(senderAddr.sin_port);
 
                // Process responses from different servers(R's responses are in handler)
                if (senderPort == 21207) {      // Server A
                    std::cout << "The main server has received the response from server A using UDP over "
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

            // Debug: Get client's port for logging
            // int clientPort = ntohs(clientAddr.sin_port);
            // std::cout << "Server M received connection from client on port: " << clientPort << std::endl;

            // Add new client socket to set
            clientSockets.insert(clientSocket);
        }

        // Check existing client connections for data
        std::set<int> socketsToRemove;
        for (int sock : clientSockets) {
            if (FD_ISSET(sock, &readfds)) {
                char buffer[BUFFER_SIZE] = {0};
                // (from Beej's)
                ssize_t tcpBytes = recv(sock, buffer, sizeof(buffer), 0);
                if (tcpBytes <= 0) {
                    // Client disconnected
                    std::cout << "Client disconnected" << std::endl;
                    socketsToRemove.insert(sock);
                    authenticatedClients.erase(sock);
                    continue;
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
                // std::cout << "socket: " << sock << std::endl;
                // std::cout << "======================" << std::endl;

                // Handle commands or authentication
                if (first_word == "lookup" || first_word == "push" || 
                    first_word == "deploy" || first_word == "remove" || first_word == "log") {
                    
                    // Verify client is authenticated
                    if (authenticatedClients.find(sock) == authenticatedClients.end()) {
                        std::string error = "Error: Not authenticated";
                        send(sock, error.c_str(), error.length(), 0);
                        continue;
                    }

                    ClientInfo& client = authenticatedClients[sock];
                    std::string param;
                    iss >> param;  // Get second parameter (username or filename)

                    if (first_word == "lookup") {
                        if (param.empty() && !client.isGuest) {
                            // For members, if no username specified, use their own username
                            addToLog(client.username, "lookup " + param);
                            std::string newMessage(buffer);
                            newMessage = "lookup " + client.username;
                            handleLookupRequest(udpSocket, sock, newMessage, client.username, false);
                        }
                        else {
                            // Username is specified for either guest or member
                            handleLookupRequest(udpSocket, sock, message, param, client.isGuest);
                        }
                    }
                    else if (!client.isGuest) {  // Member-only commands
                        if (first_word == "push") {
                            // Create new message with username for serverR usage
                            std::string serverR_message = "push " + client.username + " " + param;
                            addToLog(client.username, "push " + param);
                            handlePushRequest(udpSocket, sock, serverR_message, client.username); 
                        }
                        else if (first_word == "deploy") {
                            addToLog(client.username, "deploy");
                            handleDeployRequest(udpSocket, sock, message, client.username);
                        }
                        else if (first_word == "remove") {
                            handleRemoveRequest(udpSocket, sock, message, client.username);
                        }
                        else if (first_word == "log") {
                            addToLog(client.username, "log ");
                            handleLogRequest(sock, client.username);
                        }
                    }
                    else {
                        std::string error = "Guests can only use the lookup command";
                        send(sock, error.c_str(), error.length(), 0);
                    }
                }
                // Handle authentication
                else {
                    size_t space = message.find(' ');
                    if (space == std::string::npos) {
                        std::string error = "Invalid credentials format";
                        send(sock, error.c_str(), error.length(), 0);
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
                        authenticatedClients[sock] = client;
                        send(sock, "guest", 5, 0);
                        continue;
                    }

                    // Forward authentication request to Server A
                    std::cout << "Server M has sent authentication request to Server A" << std::endl;
                    
                    // from Beej's
                    struct sockaddr_in serverA_addr;
                    serverA_addr.sin_family = AF_INET;
                    serverA_addr.sin_port = htons(21207);
                    serverA_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

                    sendto(udpSocket, buffer, strlen(buffer), 0, 
                        (struct sockaddr*)&serverA_addr, sizeof(serverA_addr));

                    // Wait for Server A's response(from Beej's)
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
                        authenticatedClients[sock] = client;
                    }

                    // Forward response to client(from Beej's)
                    send(sock, response, bytes, 0);
                    
                    std::cout << "The main server has sent the response from server A to client using TCP over port "
                            << TCP_PORT << "." << std::endl;
                }
            }
        }
        // Remove disconnected clients
        for (int sock : socketsToRemove) {
            close(sock);
            clientSockets.erase(sock);
        }
    }
    // Cleanup (this part won't be reached unless break the loop)
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

    // from Beej's
    struct sockaddr_in serverR_addr;
    serverR_addr.sin_family = AF_INET;
    serverR_addr.sin_port = htons(SERVER_R_PORT);
    serverR_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    std::cout << "The main server has sent the lookup request to server R." << std::endl;

    // Forward request to Server R(from Beej's)
    sendto(udpSocket, request.c_str(), request.length(), 0,
           (struct sockaddr*)&serverR_addr, sizeof(serverR_addr));

    // Get response from Server R(from Beej's)
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
    // from Beej's                      
    struct sockaddr_in serverR_addr;
    serverR_addr.sin_family = AF_INET;
    serverR_addr.sin_port = htons(SERVER_R_PORT);
    serverR_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    std::cout << "The main server has received a push request from " << username 
              << ", using TCP over port " << TCP_PORT << "." << std::endl;

    std::cout << "The main server has sent the push request to server R." << std::endl;

    // Forward request to Server R(from Beej's)
    sendto(udpSocket, request.c_str(), request.length(), 0,
           (struct sockaddr*)&serverR_addr, sizeof(serverR_addr));

    // Get response from Server R(from Beej's)
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
        std::cout << "The main server has received the response from server R using UDP over" 
                << UDP_PORT << ", asking for overwrite confirmation" << std::endl;
        std::cout << "The main server has sent the overwrite confirmation request to the client." << std::endl;
        send(clientSocket, buffer, bytes, 0);

        // Wait for client's Y/N response(from Beej's)
        memset(buffer, 0, BUFFER_SIZE);
        bytes = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        
        std::cout << "The main server has received the overwrite confirmation response from "
                  << username << " using TCP over port " << TCP_PORT << std::endl;

        // Parse the original push request to get filename
        std::istringstream iss(request);
        std::string command, username_from_req, filename;
        iss >> command >> username_from_req >> filename;

        // Create overwrite command message for Server R
        std::string overwriteMsg = "overwrite " + username + " " + filename + " " + std::string(buffer);

        // Forward confirmation to Server R(from Beej's)
        std::cout << "The main server has sent the overwrite confirmation response to server R." << std::endl;
        sendto(udpSocket, overwriteMsg.c_str(), overwriteMsg.length(), 0,
               (struct sockaddr*)&serverR_addr, sizeof(serverR_addr));

        // Get final response from Server R(from Beej's)
        memset(buffer, 0, BUFFER_SIZE);
        bytes = recvfrom(udpSocket, buffer, BUFFER_SIZE, 0,
                        (struct sockaddr*)&responseAddr, &responseLen);

        // Forward final response to client(from Beej's)
        send(clientSocket, buffer, bytes, 0);
        std::cout << "The main server has sent the response to the client." << std::endl;
    } else {
        // Forward response to client
        send(clientSocket, buffer, bytes, 0);
        std::cout << "The main server has sent the response to the client." << std::endl;
    }
}

void handleRemoveRequest(int udpSocket, int clientSocket, const std::string& request,
                        const std::string& username) {
    // Print initial message
    std::cout << "The main server has received a remove request from member " 
              << username << " TCP over port " << TCP_PORT << "." << std::endl;

    // Forward request to Server R(from Beej's)
    struct sockaddr_in serverR_addr;
    serverR_addr.sin_family = AF_INET;
    serverR_addr.sin_port = htons(SERVER_R_PORT);
    serverR_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Extract filename from request
    std::istringstream iss(request);
    std::string command, filename;
    iss >> command >> filename;

    addToLog(username, "remove " + filename);
    // Create new message with username for serverR usage
    std::string serverR_message = "remove " + username + " " + filename;

    // Send request to Server R(from Beej's)
    sendto(udpSocket, serverR_message.c_str(), serverR_message.length(), 0,
           (struct sockaddr*)&serverR_addr, sizeof(serverR_addr));

    // Wait for Server R's response(from Beej's)
    char buffer[1024] = {0};
    struct sockaddr_in responseAddr;
    socklen_t responseLen = sizeof(responseAddr);
    
    ssize_t bytes = recvfrom(udpSocket, buffer, sizeof(buffer), 0,
                            (struct sockaddr*)&responseAddr, &responseLen);

    if (bytes > 0) {
        buffer[bytes] = '\0';
        std::string response(buffer);

        // Forward response to client
        if (response == "success") {
            std::cout << "The main server has received confirmation of the remove request done by the server R" 
                    << std::endl;
            std::string successMsg = "The remove request was successful.";
            send(clientSocket, successMsg.c_str(), successMsg.length(), 0);
        } else {
            // File not found or other error
            std::string errorMsg = filename + " does not exist in your repository.";
            send(clientSocket, errorMsg.c_str(), errorMsg.length(), 0);
        }
    }
}

void handleDeployRequest(int udpSocket, int clientSocket, const std::string& request,
                        const std::string& username) {
    // First contact Server R to get files(from Beej's)
    struct sockaddr_in serverR_addr;
    serverR_addr.sin_family = AF_INET;
    serverR_addr.sin_port = htons(SERVER_R_PORT);
    serverR_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    std::cout << "The main server has received a deploy request from member " << username 
              << " TCP over port " << TCP_PORT << "." << std::endl;

    // Create new message for serverR usage to lookup
    std::string serverR_message = "lookup " + username;

    // Forward request to Server R(from Beej's)
    std::cout << "The main server has sent the lookup request to server R." << std::endl;
    sendto(udpSocket, serverR_message.c_str(), serverR_message.length(), 0,
           (struct sockaddr*)&serverR_addr, sizeof(serverR_addr));

    // Get response(file list) from Server R(from Beej's)
    char buffer[BUFFER_SIZE] = {0};
    struct sockaddr_in responseAddr;
    socklen_t responseLen = sizeof(responseAddr);
    
    ssize_t bytes = recvfrom(udpSocket, buffer, BUFFER_SIZE, 0,
                            (struct sockaddr*)&responseAddr, &responseLen);

    std::cout << "The main server received the lookup response from server R." << std::endl;

    // Store the file list from Server R
    std::string fileList(buffer, bytes);  // Use bytes to create string

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

        // Create response message for client with list of deployed files
        std::string clientResponse = "The following files in his/her repository have been deployed.\n" + fileList;
        send(clientSocket, clientResponse.c_str(), clientResponse.length(), 0);
    }
    // No file to deploy, empty repository
    else {
        std::cout << "No file to deploy" << std::endl;
        std::string errorMsg = "Empty repository";
        send(clientSocket, errorMsg.c_str(), errorMsg.length(), 0);
    }
}

void handleLogRequest(int clientSocket, const std::string& username) {
    std::cout << "The main server has received a log request from member " 
              << username << " TCP over port " << TCP_PORT << "." << std::endl;

    std::string response;
    if (userLogs.find(username) != userLogs.end()) {
        int count = 1;
        for (const auto& entry : userLogs[username]) {
            response += std::to_string(count) + ". " + entry.operation + "\n";
            count++;
        }
    }
    // from Beej's
    send(clientSocket, response.c_str(), response.length(), 0);
    std::cout << "The main server has sent the log response to the client." << std::endl;
}

void addToLog(const std::string& username, const std::string& operation) {
    time_t now = time(0);
    std::string timestamp = ctime(&now);
    LogEntry entry = {
        username,
        operation,
        timestamp
    };
    userLogs[username].push_back(entry);
}
