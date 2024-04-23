#include <iostream> //standard input and output operations
#include <winsock2.h> //enables the server to create and manage sockets, establish network connections, send and receive data over TCP/IP, and handle socket-related errors
#include <string> //string manipulation functionalities
#include <fstream> //file input and output operations
#include <sstream> //string stream functionalities. It is used for parsing and extracting tokens
#include <mutex> //support for mutexes (mutual exclusion)
#include <thread> //enables multi-threading capabilities
#include <cstdlib> // For system("pause")
using namespace std;

//instructs the linker to include the ws2_32.lib library during the linking phase of compilation.
#pragma comment(lib, "ws2_32.lib")

// Structure to represent connected clients in the linked list
struct ClientNode {
    SOCKET clientSocket;
    ClientNode* next;

    ClientNode(SOCKET socket) : clientSocket(socket), next(nullptr) {}
};

ClientNode* connectedClientsHead = nullptr;
mutex clientMutex;

// Function prototypes
void handleClient(SOCKET clientSocket);
string decrypt(const string& cipher_text, int key);

// Decrypt a string using Caesar cipher taked the cipher text and the key then returns the plain text
string decrypt(const string& cipher_text, int key) {
    string plain_text = "";
    string alphabet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUV1234567890!@#$%^&*()_+~`-=/.,<>:;|";
    for (char i : cipher_text) {
        char c = tolower(i);
        if (isalpha(c)) {
            int index = alphabet.find(c);
            char decrypted_char = alphabet[(index - key + 26) % 26];
            plain_text += (isupper(i)) ? toupper(decrypted_char) : decrypted_char;
        } else {
            plain_text += i; // Preserve non-alphabet characters
        }
    } return plain_text;
}

// Add a new client to the list of connected clients
void addClient(SOCKET clientSocket) {
    ClientNode* newNode = new ClientNode(clientSocket); //Creating a new client node using memory allocation
    lock_guard<mutex> lock(clientMutex); //Mutex locking ensuring that only one thread can modify the shared lsit
    newNode->next = connectedClientsHead; //Adding the client to the list
    connectedClientsHead = newNode;
}

// Remove a client from the list of connected clients throgh taking the client socket and does not return any values
void removeClient(SOCKET clientSocket) {
    // Lock the mutex to ensure exclusive access to the shared data
    lock_guard<mutex> lock(clientMutex);

    // Initialize pointers to traverse the linked list
    ClientNode* current = connectedClientsHead;  // Pointer to current node
    ClientNode* prev = nullptr;                  // Pointer to previous node

    // Traverse the linked list to find the node with the specified clientSocket
    while (current != nullptr) {
        // Check if the current node matches the clientSocket to be removed
        if (current->clientSocket == clientSocket) {
            // Remove the node from the linked list
            if (prev == nullptr) {
                // If the node to be removed is the head of the list
                connectedClientsHead = current->next; // Update the head pointer
            } else {
                // If the node to be removed is not the head of the list
                prev->next = current->next; // Update the previous node's next pointer
            }
            
            // Delete the current node (remove from memory)
            delete current;
            break; // Exit the loop after removing the node
        }
        
        // Move to the next node in the linked list
        prev = current;        // Update the previous pointer
        current = current->next; // Move to the next node
    }
}

// Handle communication with a client
void handleClient(SOCKET clientSocket) {
    char buffer[1024]; // Buffer to store received data
    int bytesReceived;  // Variable to track the number of bytes received

    // Receive initial data from client
    memset(buffer, 0, sizeof(buffer)); // Initialize buffer to zero
    bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0); // Receive data from client

    if (bytesReceived > 0) { // Check if data was received successfully
        string credentials(buffer); // Convert received data to string
        stringstream ss(credentials); // Create a stringstream to parse the string
        string command;
        ss >> command; // Extract the command from the stringstream

        if (command == "LOGIN") { // Handle login command
            string username, encryptedPassword;
            ss >> username >> encryptedPassword; // Extract username and encrypted password

            // Decrypt the received encrypted password
            string decryptedPassword = decrypt(encryptedPassword, 3);

            // Check login credentials against stored data in users.txt
            ifstream infile("users.txt"); // Open users.txt file for reading
            string line;
            bool loginSuccess = false;
            while (getline(infile, line)) { // Read each line in the file
                stringstream linestream(line); // Create a stringstream to parse the line
                string storedUsername, storedEncryptedPassword;
                linestream >> storedUsername >> storedEncryptedPassword; // Extract username and encrypted password

                // Decrypt the stored encrypted password for comparison
                string decryptedStoredPassword = decrypt(storedEncryptedPassword, 3);

                if (storedUsername == username && decryptedStoredPassword == decryptedPassword) {
                    loginSuccess = true; // Login successful
                    break;
                }
            }
            infile.close(); // Close the file

            // Respond to client based on login success
            if (loginSuccess) {
                send(clientSocket, "Login Successful", 17, 0); // Send success message to client
                cout << "User " << username << " logged in successfully." << endl;
            } else {
                send(clientSocket, "Login Failed", 13, 0); // Send failure message to client
                cout << "Failed login attempt for " << username << endl;
                closesocket(clientSocket); // Close client socket
                removeClient(clientSocket); // Remove client from list of connected clients
                return; // Exit the function
            }
        } else if (command == "CREATE") { // Handle create account command
            string username, encryptedPassword;
            ss >> username >> encryptedPassword; // Extract username and encrypted password

            // Check if the username already exists in users.txt
            ifstream infile("users.txt"); // Open users.txt file for reading
            string line;
            bool usernameExists = false;
            while (getline(infile, line)) { // Read each line in the file
                stringstream linestream(line); // Create a stringstream to parse the line
                string storedUsername;
                linestream >> storedUsername; // Extract username

                if (storedUsername == username) {
                    usernameExists = true; // Username already exists
                    break;
                }
            }
            infile.close(); // Close the file

            // Respond to client based on username existence
            if (usernameExists) {
                send(clientSocket, "The username already exists.", 29, 0); // Send message to client
                closesocket(clientSocket); // Close client socket
                removeClient(clientSocket); // Remove client from list of connected clients
                return; // Exit the function
            } else {
                // Store the new username and encrypted password in users.txt
                ofstream outfile("users.txt", ios::app); // Open users.txt file for writing
                if (outfile.is_open()) {
                    outfile << username << " " << encryptedPassword << endl; // Write username and password to file
                    outfile.close(); // Close the file
                    send(clientSocket, "Account Created Successfully", 29, 0); // Send success message to client
                    cout << "New account created for " << username << "." << endl;
                } else {
                    send(clientSocket, "Account creation failed", 24, 0); // Send failure message to client
                    cerr << "Error: Unable to open users.txt for writing." << endl;
                }
            }
        } else {
            cerr << "Unknown command received." << endl; // Handle unknown command
            closesocket(clientSocket); // Close client socket
            removeClient(clientSocket); // Remove client from list of connected clients
            return; // Exit the function
        }
    } else {
        cerr << "Failed to receive data." << endl; // Handle data receive failure
        closesocket(clientSocket); // Close client socket
        removeClient(clientSocket); // Remove client from list of connected clients
        return; // Exit the function
    }

    // Chatting loop
    while (true) {
        memset(buffer, 0, sizeof(buffer)); // Clear buffer
        bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0); // Receive message from client
        if (bytesReceived <= 0) { // Check for client disconnection or error
            cerr << "Client disconnected or error occurred." << endl;
            break; // Exit the loop
        }

        // Broadcast received message to other connected clients
        lock_guard<mutex> lock(clientMutex); // Lock mutex to ensure thread safety
        for (ClientNode* current = connectedClientsHead; current != nullptr; current = current->next) {
            if (current->clientSocket != clientSocket) { // Exclude sender from broadcast
                cout << "Encrypted Message Received: " << buffer << "\n"; // Display received message
                send(current->clientSocket, buffer, bytesReceived, 0); // Send message to other clients
            }
        }
    }

    // Clean up: Close socket and remove client from list of connected clients
    closesocket(clientSocket); // Close client socket
    removeClient(clientSocket); // Remove client from list of connected clients
}


int main() {
    //Variable Declaration
    WSADATA wsaData;
    SOCKET serverSocket;
    struct sockaddr_in serverAddr, clientAddr;
    int clientAddrLen = sizeof(clientAddr);

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {  // Start Winsock (version 2.2)
        cerr << "WSAStartup failed." << endl;  // Display error message if startup fails
        return 1;
    }

    // Create socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);  // Create TCP socket
    if (serverSocket == INVALID_SOCKET) {  // Check if socket creation failed
        cerr << "Failed to create socket." << endl;  // Display error message
        WSACleanup();  // Clean up Winsock
        return 1;
    }

    // Bind socket to port
    serverAddr.sin_family = AF_INET;  // Specify IPv4 address family
    serverAddr.sin_addr.s_addr = INADDR_ANY;  // Bind to any available network interface
    serverAddr.sin_port = htons(8888);  // Use port 8888

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "Bind failed." << endl;  // Display error message if bind fails
        closesocket(serverSocket);  // Close the socket
        WSACleanup();  // Clean up Winsock
        return 1;
    }

    // Listen for incoming connections
    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        cerr << "Listen failed." << endl;  // Display error message if listen fails
        closesocket(serverSocket);  // Close the socket
        WSACleanup();  // Clean up Winsock
        return 1;
    }

    cout << "Chat server is running on port 8888..." << endl;

    // Accept and handle incoming client connections
    while (true) {
        SOCKET clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);  // Accept client connection
        if (clientSocket == INVALID_SOCKET) {
            cerr << "Accept failed." << endl;  // Display error message if accept fails
            continue;  // Continue to next iteration of loop
        }

        cout << "New client connected from " << inet_ntoa(clientAddr.sin_addr) << "." << endl;  // Display client connection info

        // Add client to the list and handle communication in a new thread
        addClient(clientSocket);  // Add client to connected clients list
        thread(handleClient, clientSocket).detach();  // Start new thread to handle client communication
    }

    // Clean up: Close server socket and cleanup Winsock
    closesocket(serverSocket);  // Close the server socket
    WSACleanup();  // Clean up Winsock

    return 0;  // Return success status
}
