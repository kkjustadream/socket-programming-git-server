Git450 Socket Programming Project

Personal Information
Full Name: Sheng-Hung Huang
Session 1
Student ID: 9651612207

Development Environment
VM: CS 104 (developer/developer)
OS: Ubuntu 16.04.3
C++ version: 11 or later
Compiler: g++

Features
Basic user authentication (member/guest)
Repository operations (lookup, push, remove, deploy)
Extra Credit: Implemented log command functionality

Code Files
Makefile: Compilation rules for all source files, includes 'all' and 'clean' targets
README.txt: Project documentation and implementation details
client.cpp: Client implementation for member and guest users
serverM.cpp: Main server for request routing and coordination
serverA.cpp: Authentication server for user verification
serverR.cpp: Repository server for file management
serverD.cpp: Deployment server for repository deployment

Idiosyncrasy
The system does not handle empty repository lookups (when a user has no files in their repository)
Test cases will not include empty repository scenarios

Code References from Beej's Guide
Basic socket creation (Chapter 5.2)
Bind and Listen Operations(Chapter 5.3, 5.5)
TCP connection handling (Chapter 5.7)
Select() implementation (Chapter 7.3)

Build Instructions:
Prerequisites:
members.txt: Contains username and encrypted password pairs for authentication
filenames.txt: Contains repository information (username and filename pairs)
(Optional) original.txt: Reference file containing username and original (non-encrypted) password pairs

Compilation and Execution:
make all
Start servers in order:
./serverM
./serverA
./serverR
./serverD
Start client with credentials:
For guest access: ./client guest guest
For member access: ./client <username> <password>
