#include <iostream>
#include <winsock2.h>
#include <thread>
#include <string>
#include <fstream>
#include <sstream>
#include <mutex>
using namespace std;

#pragma comment(lib, "ws2_32.lib")

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

// Function to decrypt a string using Caesar cipher
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

void addClient(SOCKET clientSocket) {
    ClientNode* newNode = new ClientNode(clientSocket);
    clientMutex.lock();
    newNode->next = connectedClientsHead;
    connectedClientsHead = newNode;
    clientMutex.unlock();
}

void removeClient(SOCKET clientSocket) {
    clientMutex.lock();
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
    clientMutex.unlock();
}

void handleClient(SOCKET clientSocket) {
    char buffer[1024];
    int bytesReceived;

    memset(buffer, 0, sizeof(buffer));
    bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived > 0) {
        string credentials(buffer);
        stringstream ss(credentials);
        string command;
        ss >> command;

        if (command == "CREATE") {
            string username, encryptedPassword;
            ss >> username >> encryptedPassword;
            
            // Check if the username already exists
            ifstream infile("users.txt");
            string line;
            bool userExists = false;
            while (getline(infile, line)) {
                stringstream linestream(line);
                string storedUsername, storedEncryptedPassword;
                linestream >> storedUsername;
                if (storedUsername == username) {
                    userExists = true;
                    break;
                }
            }
            infile.close();
            
            if (userExists) {
                send(clientSocket, "User already exists", 18, 0);
                cout << "Account creation attempt for existing user: " << username << endl;
            } else {
                // Create the user
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
}
int main() {
    WSADATA wsaData;
    SOCKET serverSocket;
    struct sockaddr_in serverAddr, clientAddr;
    int clientAddrLen = sizeof(clientAddr);

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed.\n";
        return 1;
    }

    // Create socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        cerr << "Failed to create socket.\n";
        WSACleanup();
        return 1;
    }

    // Bind socket to port
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8888);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "Bind failed.\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    // Listen for incoming connections
    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        cerr << "Listen failed.\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    cout << "Chat server is running on port 8888...\n";

    while (true) {
        SOCKET clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if (clientSocket == INVALID_SOCKET) {
            cerr << "Accept failed.\n";
            continue;
        }

        cout << "New client connected from " << inet_ntoa(clientAddr.sin_addr) << ".\n";

        addClient(clientSocket);

        thread(handleClient, clientSocket).detach();  // Handle client in a new thread
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}