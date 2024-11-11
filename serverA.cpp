// serverA.cpp
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <unistd.h>

int main() {
    // Create UDP socket
    int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket < 0) {
        std::cerr << "UDP socket creation failed" << std::endl;
        return 1;
    }

    // Setup address
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(21207);  // Your port based on USC ID
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    // Bind socket
    if (bind(udpSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Bind failed" << std::endl;
        close(udpSocket);
        return 1;
    }

    std::cout << "Server A is up and running using UDP on port 21207" << std::endl;

    // Test //
    // After printing "Server A is up and running..."
    struct sockaddr_in mainServer;
    mainServer.sin_family = AF_INET;
    mainServer.sin_port = htons(24207);
    //mainServer.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Send test message to main server
    const char* testMsg = "Test from Server A";
    sendto(udpSocket, testMsg, strlen(testMsg), 0, 
        (struct sockaddr*)&mainServer, sizeof(mainServer));


    // Buffer for receiving data
    char buffer[1024];
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    // Main server loop
    while (true) {
        // Wait for messages from Main Server
        ssize_t bytesReceived = recvfrom(udpSocket, buffer, sizeof(buffer), 0,
                                       (struct sockaddr*)&clientAddr, &clientLen);
        
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            // Process authentication request
            // Here you'll add the logic to:
            // 1. Read from members.txt
            // 2. Check credentials
            // 3. Send response back to Main Server
        }
    }

    // Cleanup (this won't be reached unless you break the loop)
    close(udpSocket);
    return 0;
}