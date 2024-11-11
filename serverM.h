// serverM.h example
#ifndef SERVER_M_H
#define SERVER_M_H

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <unistd.h>

// Function declarations
void setupUDPSocket();
void setupTCPSocket();
void handleClient();

#endif