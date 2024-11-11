// serverM.cpp
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <unistd.h>

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

    char buffer[1024];
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    // Main server loop
    while (true) {
        // Accept TCP connections
        int clientSocket = accept(tcpSocket, (struct sockaddr*)&clientAddr, &clientLen);
        if (clientSocket < 0) {
            std::cerr << "Accept failed" << std::endl;
            continue;
        }
        std::cout << "Server M received connection from client" << std::endl;

        // Handle UDP messages from backend servers
        ssize_t udpBytes = recvfrom(udpSocket, buffer, sizeof(buffer), 0, 
                                  (struct sockaddr*)&clientAddr, &clientLen);
        if (udpBytes > 0) {
            buffer[udpBytes] = '\0';
            // Handle the message
        }

        // Handle TCP messages from client
        ssize_t tcpBytes = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (tcpBytes > 0) {
            buffer[tcpBytes] = '\0';
            // Handle the message
        }

        close(clientSocket);
    }

    // Cleanup (this part won't be reached unless you break the loop)
    close(udpSocket);
    close(tcpSocket);
    return 0;
}