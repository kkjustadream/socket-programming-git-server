// serverM.cpp
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

#define TCP_PORT 25207
#define UDP_PORT 24207

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
            char buffer[1024] = {0};
            struct sockaddr_in senderAddr;
            socklen_t senderLen = sizeof(senderAddr);
            
            ssize_t udpBytes = recvfrom(udpSocket, buffer, sizeof(buffer), 0, 
                                    (struct sockaddr*)&senderAddr, &senderLen);
            
            if (udpBytes > 0) {
                buffer[udpBytes] = '\0';
                // Check if message is from Server A
                if (ntohs(senderAddr.sin_port) == 21207) {
                    std::cout << "Server M received message from Server A" << std::endl;
                    std::cout << "Message: " << buffer << std::endl;
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

            // Handle client message
            char buffer[1024] = {0};
            ssize_t tcpBytes = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (tcpBytes > 0) {
                buffer[tcpBytes] = '\0';
                // Handle the message
                // Parse username and password
                std::string credentials(buffer);
                size_t space = credentials.find(' ');
                std::string username = credentials.substr(0, space);
                std::string password = credentials.substr(space + 1);
                // Print received credentials (hide password)
                std::string hidden_password(password.length(), '*');
                std::cout << "Server M has received username " << username 
                        << " and password " << password << "." << std::endl;

                // Handle guest authentication
                if (username == "guest" && password == "guest") {
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
                char response[1024] = {0};
                
                ssize_t bytes = recvfrom(udpSocket, response, sizeof(response), 0,
                                        (struct sockaddr*)&responseAddr, &responseLen);
                
                std::cout << "The main server has received the response from server A using UDP over "
                        << UDP_PORT << std::endl;

                // Forward response to client
                send(clientSocket, response, bytes, 0);
                
                std::cout << "The main server has sent the response from server A to client using TCP over port "
                        << TCP_PORT << "." << std::endl;


            }
            close(clientSocket);
        }

    }

    // Cleanup (this part won't be reached unless you break the loop)
    close(udpSocket);
    close(tcpSocket);
    return 0;
}