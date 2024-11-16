#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>

class RepositoryServer {
private:
    int udpSocket;
    std::map<std::string, std::vector<std::string>> userFiles;
    const int PORT = 22207;  // Your USC ID

    void loadFilenames() {
        std::ifstream file("filenames.txt");
        std::string line, username, filename;
        
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            if (iss >> username >> filename) {
                userFiles[username].push_back(filename);
            }
        }
        file.close();
    }

    void saveFilenames() {
        std::ofstream file("filenames.txt");
        if (!file.is_open()) {
            std::cerr << "Error: Could not open filenames.txt for writing" << std::endl;
            return;
        }
        // Write each username and filename pair
        for (const auto& user : userFiles) {
            const std::string& username = user.first;  // Get username
            for (const auto& filename : user.second) {
                file << username << " " << filename << "\n";  // Write "username filename" format
            }
        }
        file.close();
    }

public:
    RepositoryServer() {
        // Create UDP socket
        udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
        if (udpSocket < 0) {
            std::cerr << "Socket creation failed" << std::endl;
            exit(1);
        }

        // Setup address
        struct sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(PORT);
        serverAddr.sin_addr.s_addr = INADDR_ANY;

        // Bind socket
        if (bind(udpSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            std::cerr << "Bind failed" << std::endl;
            close(udpSocket);
            exit(1);
        }

        std::cout << "Server R is up and running using UDP on port " << PORT << std::endl;
        
        loadFilenames();
    }

    std::string handleLookup(const std::string& username) {
        std::cout << "Server R has received a lookup request from the main server." << std::endl;
        
        std::string response;
        if (userFiles.find(username) == userFiles.end()) {
            response = username + " does not exist";
        } else if (userFiles[username].empty()) {
            response = "Empty repository";
        } else {
            for (const auto& file : userFiles[username]) {
                response += file + "\n";
            }
        }
        
        std::cout << "Server R has finished sending the response to the main server." << std::endl;
        return response;
    }

    std::string handlePush(const std::string& username, const std::string& filename) {
        std::cout << "Server R has received a push request from the main server." << std::endl;
        
        // Check if file exists
        auto it = userFiles.find(username);
        if (it != userFiles.end()) {
            for (const auto& file : it->second) {
                if (file == filename) {
                    std::cout << filename << " exists in " << username 
                             << "'s repository; requesting overwrite confirmation." << std::endl;
                    return "exists";
                }
            }
        }

        // Debug print
        std::cout << "Debug - Pushing file: username='" << username << "' filename='" << filename << "'" << std::endl;

        // Add new file to user's repository
        userFiles[username].push_back(filename);
        // Save to filenames.txt
        saveFilenames();
        std::cout << filename << " uploaded successfully." << std::endl;
        return "success";
    }

    std::string handleOverwrite(const std::string& username, const std::string& filename, 
                        const std::string& confirm) {
        if (confirm == "Y" || confirm == "y") {
            std::cout << "User requested overwrite; overwrite successful." << std::endl;        
            // File already exists in userFiles, no need to add again
            return "success";
        } else {
            std::cout << "Overwrite denied" << std::endl;
            return "Overwrite denied";
        }
    }

    std::string handleRemove(const std::string& username, const std::string& filename) {
        std::cout << "Server R has received a remove request from the main server." << std::endl;
        
        auto it = userFiles.find(username);
        if (it != userFiles.end()) {
            auto& files = it->second;
            for (auto fileIt = files.begin(); fileIt != files.end(); ++fileIt) {
                if (*fileIt == filename) {
                    files.erase(fileIt);
                    saveFilenames();
                    return "success";
                }
            }
        }
        return "file not found";
    }

    std::string handleDeploy(const std::string& username) {
        std::cout << "Server R has received a deploy request from the main server." << std::endl;
        
        std::string response;
        auto it = userFiles.find(username);
        if (it != userFiles.end() && !it->second.empty()) {
            for (const auto& file : it->second) {
                response += file + "\n";
            }
        } else {
            response = "empty";
        }
        
        std::cout << "Server R has finished sending the response to the main server." << std::endl;
        return response;
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
                std::string request(buffer);


                // Debug print
                std::cout << "Debug - Received request: '" << request << "'" << std::endl;


                std::string response;
                std::istringstream iss(request);
                std::string command, username, filename;
                
                iss >> command;
                if (command == "lookup") {
                    iss >> username;
                    response = handleLookup(username);
                } 
                else if (command == "push") {
                    iss >> username >> filename;
                    response = handlePush(username, filename);
                }
                else if (command == "overwrite") {
                    iss >> username >> filename >> command;
                    std::cout << "username: " << username << std::endl;
                    std::cout << "filename: " << filename << std::endl;
                    std::cout << "command: " << command << std::endl;
                    response = handleOverwrite(username, filename, command);
                }
                else if (command == "remove") {
                    iss >> username >> filename;
                    response = handleRemove(username, filename);
                }
                else if (command == "deploy") {
                    iss >> username;
                    response = handleDeploy(username);
                }

                sendto(udpSocket, response.c_str(), response.length(), 0,
                      (struct sockaddr*)&clientAddr, clientLen);
            }
        }
    }
};

int main() {
    RepositoryServer server;
    server.run();
    return 0;
}