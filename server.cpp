#include <iostream>
#include <winsock2.h>
#include <thread>
#include <vector>
#include <algorithm>
#include <string>
#include <mutex>
#include <fstream>
using namespace std;

#pragma comment(lib, "ws2_32.lib")

vector<pair<SOCKET, string>> connectedClients;
mutex clientMutex;

void handleClient(SOCKET clientSocket) {
    char buffer[1024];
    int bytesReceived;

    // Receive username from client
    bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived > 0) {
        string username(buffer);
        cout << "New client connected: " << username << endl;

        // Receive password from client
        bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived > 0) {
            string password(buffer);
            cout << "Password received." << endl;

            // Store username-password pair in file "users.txt"
            ofstream userFile("users.txt", ios::app); // Open file in append mode
            if (userFile.is_open()) {
                userFile << username << " " << password << endl;
                userFile.close();
            } else {
                cerr << "Failed to open users.txt for writing." << endl;
            }

            clientMutex.lock();
            connectedClients.push_back(make_pair(clientSocket, username)); // Store socket and username
            clientMutex.unlock();
        }
    }

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived == SOCKET_ERROR || bytesReceived == 0) {
            cerr << "Client disconnected.\n";
            break;
        }

        // Relay message to all other clients
        clientMutex.lock();
        for (auto& client : connectedClients) {
            if (client.first != clientSocket) {
                send(client.first, buffer, bytesReceived, 0);
            }
        }
        clientMutex.unlock();
    }

    // Remove client socket from vector after handling
    clientMutex.lock();
    auto it = find_if(connectedClients.begin(), connectedClients.end(),
                      [clientSocket](const pair<SOCKET, string>& client) {
                          return client.first == clientSocket;
                      });
    if (it != connectedClients.end()) {
        connectedClients.erase(it);
    }
    clientMutex.unlock();

    closesocket(clientSocket);
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
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
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

    cout << "Chat server is running...\n";

    while (true) {
        SOCKET clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientAddrLen);
        if (clientSocket == INVALID_SOCKET) {
            cerr << "Accept failed.\n";
            closesocket(serverSocket);
            WSACleanup();
            return 1;
        }

        cout << "New client connected.\n";

        thread clientThread(handleClient, clientSocket);
        clientThread.detach();
    }

    closesocket(serverSocket);
    WSACleanup();

    return 0;
}
