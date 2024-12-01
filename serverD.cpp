#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <fstream>
#include <sstream>
#include <vector>

#define SERVER_D_PORT 23207

class DeploymentServer {
private:
    int udpSocket;
    const int PORT = SERVER_D_PORT;
    std::string deployedFile = "deployed.txt";

    void saveDeployment(const std::string& username, const std::vector<std::string>& files) {
        std::ofstream file(deployedFile, std::ios::app);
        for (const auto& filename : files) {
            file << username << " " << filename << "\n";
        }
        file.close();
    }

public:
    DeploymentServer() {
        setupSocket();
    }

    // from Beej's
    void setupSocket() {
        udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
        if (udpSocket < 0) {
            std::cerr << "Socket creation failed" << std::endl;
            exit(1);
        }

        struct sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(PORT);
        serverAddr.sin_addr.s_addr = INADDR_ANY;

        if (bind(udpSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            std::cerr << "Bind failed" << std::endl;
            close(udpSocket);
            exit(1);
        }

        std::cout << "Server D is up and running using UDP on port " << PORT << std::endl;
    }

    void run() {
        char buffer[1024];
        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);

        while (true) {
            memset(buffer, 0, sizeof(buffer));
            ssize_t bytesReceived = recvfrom(udpSocket, buffer, sizeof(buffer), 0,
                                           (struct sockaddr*)&clientAddr, &clientLen);
            
            if (bytesReceived > 0) {
                std::cout << "Server D has received a deploy request from the main server." << std::endl;
                
                std::string request(buffer);
                std::istringstream iss(request);
                std::string username;
                std::vector<std::string> files;
                
                iss >> username;
                std::string filename;
                while (iss >> filename) {
                    files.push_back(filename);
                }

                saveDeployment(username, files);
                std::cout << "Server D has deployed the user " << username << "'s repository." << std::endl;

                std::string response = "success";
                sendto(udpSocket, response.c_str(), response.length(), 0,
                      (struct sockaddr*)&clientAddr, clientLen);
            }
        }
    }
};

int main() {
    DeploymentServer server;
    server.run();
    return 0;
}