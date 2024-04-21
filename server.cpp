#include <iostream>
#include <winsock2.h>
#include <thread>
#include <vector>
#include <algorithm>
#include <string>
#include <fstream>
#include <mutex>
#include <sstream>
using namespace std;

#pragma comment(lib, "ws2_32.lib")

vector<SOCKET> connectedClients;
mutex clientMutex;

// Function prototypes
void handleClient(SOCKET clientSocket);

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

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
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
        SOCKET clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientAddrLen);
        if (clientSocket == INVALID_SOCKET) {
            cerr << "Accept failed.\n";
            continue;
        }

        cout << "New client connected from " << inet_ntoa(clientAddr.sin_addr) << ".\n";

        clientMutex.lock();
        connectedClients.push_back(clientSocket);
        clientMutex.unlock();

        thread(handleClient, clientSocket).detach();  // Correctly using handleClient in a new thread
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}

void handleClient(SOCKET clientSocket) {
    char buffer[1024];
    int bytesReceived;

    memset(buffer, 0, sizeof(buffer));
    bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived > 0) {
        string credentials(buffer);
        vector<string> tokens;
        stringstream ss(credentials);
        string intermediate;

        // Tokenizing the string by spaces
        while (getline(ss, intermediate, ' ')) {
            tokens.push_back(intermediate);
        }

        if (tokens[0] == "LOGIN") {
            string username = tokens[1];
            string password = tokens[2];
            ifstream infile("users.txt");
            string line;
            bool loginSuccess = false;
            while (getline(infile, line)) {
                if (line == username + " " + password) {
                    loginSuccess = true;
                    break;
                }
            }
            infile.close();

            if (loginSuccess) {
                send(clientSocket, "Login Successful", 17, 0);
                cout << "User " << username << " logged in successfully." << endl;
            } else {
                send(clientSocket, "Login Failed", 13, 0);
                cout << "Failed login attempt for " << username << endl;
                closesocket(clientSocket);
                return;
            }
        } else if (tokens[0] == "CREATE") {
            string username = tokens[1];
            string password = tokens[2];
            ofstream outfile("users.txt", ios::app);
            if (outfile.is_open()) {
                outfile << username << " " << password << endl;
                outfile.close();
                send(clientSocket, "Account Created Successfully", 29, 0);
                cout << "New account created for " << username << "." << endl;
            } else {
                send(clientSocket, "Account creation failed", 24, 0);
                cerr << "Error: Unable to open users.txt for writing." << endl;
            }
        }
    } else {
        cerr << "Failed to receive data." << endl;
        closesocket(clientSocket);
        return;
    }

    // Message relay loop
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) {
            cerr << "Client disconnected or error occurred.\n";
            break;
        }

        clientMutex.lock();
        for (SOCKET& otherClientSocket : connectedClients) {
            if (otherClientSocket != clientSocket) {
                send(otherClientSocket, buffer, bytesReceived, 0);
            }
        }
        clientMutex.unlock();
    }

    clientMutex.lock();
    connectedClients.erase(remove(connectedClients.begin(), connectedClients.end(), clientSocket), connectedClients.end());
    clientMutex.unlock();
    closesocket(clientSocket);
}