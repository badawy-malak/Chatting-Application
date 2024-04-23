#include <iostream>
#include <winsock2.h>
#include <thread>
#include <vector>
#include <mutex>
#include <algorithm>
using namespace std;

#pragma comment(lib, "ws2_32.lib")

vector<SOCKET> connectedClients;
mutex clientMutex;

void handleClient(SOCKET clientSocket) {
    char buffer[1024];
    int bytesReceived;

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) {
            cerr << "Client disconnected or error occurred.\n";
            break;
        }

        // Display the received encrypted message in the server terminal
        cout << "Encrypted Message from Client: " << buffer << endl;

        // Server does not decrypt messages, only handles them as encrypted
        clientMutex.lock();
        for (auto it = connectedClients.begin(); it != connectedClients.end(); ++it) {
            if (*it != clientSocket) {
                send(*it, buffer, bytesReceived, 0);
            }
        }
        clientMutex.unlock();
    }

    // Remove client from the list of connected clients
    clientMutex.lock();
    auto it = find(connectedClients.begin(), connectedClients.end(), clientSocket);
    if (it != connectedClients.end()) {
        connectedClients.erase(it);
    }
    clientMutex.unlock();

    closesocket(clientSocket);
}

int main() {
    WSADATA wsaData;
    SOCKET serverSocket;
    struct sockaddr_in serverAddr;

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
    serverAddr.sin_addr.s_addr = INADDR_ANY; // Bind to all available interfaces
    serverAddr.sin_port = htons(8888); // Use port 8888

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "Bind failed. Port may be in use or insufficient privileges.\n";
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

    // Accept incoming connections and handle clients
    while (true) {
        SOCKET clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            cerr << "Accept failed.\n";
            continue;
        }

        cout << "New client connected.\n";

        // Add client to the list of connected clients
        clientMutex.lock();
        connectedClients.push_back(clientSocket);
        clientMutex.unlock();

        // Create a new thread to handle the client
        thread(handleClient, clientSocket).detach();
    }

    // Cleanup and exit
    closesocket(serverSocket);
    WSACleanup();

    return 0;
}
