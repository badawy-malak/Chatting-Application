#include <iostream>
#include <winsock2.h>
#include <string>
#include <fstream>
#include <sstream>
#include <mutex>
#include <thread>
#include <cstdlib> // For system("pause")
using namespace std;

#pragma comment(lib, "ws2_32.lib")

// Structure to represent connected clients
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

// Decrypt a string using Caesar cipher
string decrypt(const string& cipher_text, int key) {
    string plain_text = "";
    string alphabet = "abcdefghijklmnopqrstuvwxyz";
    for (char i : cipher_text) {
        char c = tolower(i);
        if (isalpha(c)) {
            int index = alphabet.find(c);
            char decrypted_char = alphabet[(index - key + 26) % 26];
            plain_text += (isupper(i)) ? toupper(decrypted_char) : decrypted_char;
        } else {
            plain_text += i; // Preserve non-alphabet characters
        }
    }
    return plain_text;
}

// Add a new client to the list of connected clients
void addClient(SOCKET clientSocket) {
    ClientNode* newNode = new ClientNode(clientSocket);
    lock_guard<mutex> lock(clientMutex);
    newNode->next = connectedClientsHead;
    connectedClientsHead = newNode;
}

// Remove a client from the list of connected clients
void removeClient(SOCKET clientSocket) {
    lock_guard<mutex> lock(clientMutex);
    ClientNode* current = connectedClientsHead;
    ClientNode* prev = nullptr;

    while (current != nullptr) {
        if (current->clientSocket == clientSocket) {
            if (prev == nullptr) {
                connectedClientsHead = current->next;
            } else {
                prev->next = current->next;
            }
            delete current;
            break;
        }
        prev = current;
        current = current->next;
    }
}

// Handle communication with a client
void handleClient(SOCKET clientSocket) {
    char buffer[1024];
    int bytesReceived;

    // Receive initial data from client
    memset(buffer, 0, sizeof(buffer));
    bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived > 0) {
        string credentials(buffer);
        stringstream ss(credentials);
        string command;
        ss >> command;

        if (command == "LOGIN") {
            string username, encryptedPassword;
            ss >> username >> encryptedPassword;

            // Decrypt the received encrypted password
            string decryptedPassword = decrypt(encryptedPassword, 3);

            // Check login credentials against stored data
            ifstream infile("users.txt");
            string line;
            bool loginSuccess = false;
            while (getline(infile, line)) {
                stringstream linestream(line);
                string storedUsername, storedEncryptedPassword;
                linestream >> storedUsername >> storedEncryptedPassword;

                // Decrypt the stored encrypted password for comparison
                string decryptedStoredPassword = decrypt(storedEncryptedPassword, 3);

                if (storedUsername == username && decryptedStoredPassword == decryptedPassword) {
                    loginSuccess = true;
                    break;
                }
            }
            infile.close();

            // Respond to client based on login success
            if (loginSuccess) {
                send(clientSocket, "Login Successful", 17, 0);
                cout << "User " << username << " logged in successfully." << endl;
            } else {
                send(clientSocket, "Login Failed", 13, 0);
                cout << "Failed login attempt for " << username << endl;
                closesocket(clientSocket);
                removeClient(clientSocket);
                return;
            }
        } else if (command == "CREATE") {
            string username, encryptedPassword;
            ss >> username >> encryptedPassword;

            // Check if the username already exists
            ifstream infile("users.txt");
            string line;
            bool usernameExists = false;
            while (getline(infile, line)) {
                stringstream linestream(line);
                string storedUsername;
                linestream >> storedUsername;

                if (storedUsername == username) {
                    usernameExists = true;
                    break;
                }
            }
            infile.close();

            // Respond to client based on username existence
            if (usernameExists) {
                send(clientSocket, "The username already exists.", 29, 0);
                closesocket(clientSocket);
                removeClient(clientSocket);
                return;
            } else {
                // Store the new username and encrypted password
                ofstream outfile("users.txt", ios::app);
                if (outfile.is_open()) {
                    outfile << username << " " << encryptedPassword << endl;
                    outfile.close();
                    send(clientSocket, "Account Created Successfully", 29, 0);
                    cout << "New account created for " << username << "." << endl;
                } else {
                    send(clientSocket, "Account creation failed", 24, 0);
                    cerr << "Error: Unable to open users.txt for writing." << endl;
                }
            }
        } else {
            cerr << "Unknown command received." << endl;
            closesocket(clientSocket);
            removeClient(clientSocket);
            return;
        }
    } else {
        cerr << "Failed to receive data." << endl;
        closesocket(clientSocket);
        removeClient(clientSocket);
        return;
    }

    // Chatting loop
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) {
            cerr << "Client disconnected or error occurred." << endl;
            break;
        }

        // Broadcast received message to other connected clients
        lock_guard<mutex> lock(clientMutex);
        for (ClientNode* current = connectedClientsHead; current != nullptr; current = current->next) {
            if (current->clientSocket != clientSocket) {
                send(current->clientSocket, buffer, bytesReceived, 0);
            }
        }
    }

    // Clean up: Close socket and remove client from list
    closesocket(clientSocket);
    removeClient(clientSocket);
}

int main() {
    WSADATA wsaData;
    SOCKET serverSocket;
    struct sockaddr_in serverAddr, clientAddr;
    int clientAddrLen = sizeof(clientAddr);

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed." << endl;
        return 1;
    }

    // Create socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        cerr << "Failed to create socket." << endl;
        WSACleanup();
        return 1;
    }

    // Bind socket to port
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8888);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "Bind failed." << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    // Listen for incoming connections
    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        cerr << "Listen failed." << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    cout << "Chat server is running on port 8888..." << endl;

    // Accept and handle incoming client connections
    while (true) {
        SOCKET clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if (clientSocket == INVALID_SOCKET) {
            cerr << "Accept failed." << endl;
            continue;
        }

        cout << "New client connected from " << inet_ntoa(clientAddr.sin_addr) << "." << endl;

        // Add client to the list and handle communication in a new thread
        addClient(clientSocket);
        thread(handleClient, clientSocket).detach();
    }

    // Clean up: Close server socket and cleanup Winsock
    closesocket(serverSocket);
    WSACleanup();

    return 0;
}
