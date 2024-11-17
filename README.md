# Develop in VM

CS 104 user, code: developer

## Problem

need multi-clients, fix serverM

using namespace std;

invalid input

## Test user authenticate

```bash
make
# Terminal 1
./serverM

# Terminal 2
./serverA

# Terminal 3
./client guest guest

# Terminal 3
./client HannahWilliams598 pQQdZC2e2pjQ
```

## Test repo operations

```bash
# Terminal 1
./serverM

# Terminal 2
./serverA

# Terminal 3
./serverR

# Terminal 4
./serverD
```

```bash
# Terminal 5(member)
./client HannahWilliams598 pQQdZC2e2pjQ

# Test lookup(if member can lookup without username to lookup own repo)
lookup alice
lookup bob

# Test push
push newfile.cpp

# Test remove
remove test1.cpp

# Test deploy
deploy
```

For lookup:
```bash
# ServerR Terminal
Server R has received a lookup request from the main server.
Server R has finished sending the response to the main server.

# Client Terminal
The client received the response from the main server using TCP over port <port>
test1.cpp
test2.cpp
----Start a new request----
```

For push:
```bash
# ServerR Terminal
Server R has received a push request from the main server.
newfile.cpp uploaded successfully.

# Client Terminal
newfile.cpp pushed successfully
```

For deploy:
```bash
# ServerR Terminal
Server R has received a deploy request from the main server.
Server R has finished sending the response to the main server.

# ServerD Terminal
Server D has received a deploy request from the main server.
Server D has deployed the user alices repository.

# Client Terminal
The following files have been deployed:
test1.cpp
test2.cpp
newfile.cpp
----Start a new request----
```

## flow

Week 1 (Days 1-5): Foundation and Basic Setup
    Days 1-2: Project Setup and Planning
        Set up development environment in Ubuntu
        Study the project requirements thoroughly
        Create basic file structure for all components (client.c, serverM.c, serverA.c, serverR.c, serverD.c)
    Days 3-5: Basic Network Implementation
        Implement basic socket creation and connection setup
        Establish UDP connections between servers
        Establish TCP connection between client and main server
        Test basic connectivity between components
Week 2 (Days 6-10): Core Authentication
    Days 6-7: Server A Implementation
        Implement member authentication logic
        Create password encryption scheme
        Handle members.txt file reading
        Test authentication flows
    Days 8-10: Main Server and Client Authentication
        Implement client authentication interface
        Complete ServerM authentication routing
        Test guest and member authentication
        Debug authentication flows
Week 3 (Days 11-15): Repository Operations
    Days 11-12: Lookup and Push
        Implement lookup functionality
        Create push operation logic
        Handle file existence checks
        Test basic file operations
    Days 13-15: Deploy and Remove
        Implement deploy functionality
        Create remove operation logic
        Handle filenames.txt management
        Test all repository operations
Week 4 (Days 16-20): Integration and Testing
    Days 16-17: Integration
        Connect all components
        Implement error handling
        Add required message outputs
        Test all operations end-to-end
    Days 18-19: Testing and Debugging
        Comprehensive testing of all features
        Fix bugs and edge cases
        Test with multiple clients
        Verify all output messages
    Day 20: Final Review
        Code cleanup and documentation
        README file preparation
        Final end-to-end testing
        Prepare submission package
Optional: Extra Credit (if time permits)
    Implement log functionality
    Test and debug log feature
    Add log-related message outputs
Remember to:
    Test frequently throughout development
    Follow the exact naming conventions specified
    Match output messages precisely
    Kill zombie processes during testing
    Document any port number changes in README
