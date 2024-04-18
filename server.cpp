#include <iostream>
#include <winsock2.h>
#include <thread>
#include <vector>
#include <algorithm> // Include algorithm header for find
#include <string>
#include <mutex>
using namespace std;

#pragma comment(lib, "ws2_32.lib")

// Global vector to hold connected client sockets
vector<SOCKET> connectedClients;
mutex clientMutex; // Mutex to protect access to connectedClients

// Function to handle communication with a single client
void handleClient(SOCKET clientSocket) {
    char buffer[1024];
    int bytesReceived;

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived == SOCKET_ERROR || bytesReceived == 0) {
            cerr << "Client disconnected.\n";
            break;
        }

        cout << "Received from client: " << buffer << endl;

        // Relay message to all other clients
        clientMutex.lock();
        for (SOCKET& otherClientSocket : connectedClients) {
            if (otherClientSocket != clientSocket) {
                send(otherClientSocket, buffer, bytesReceived, 0);
            }
        }
        clientMutex.unlock();
    }

    // Remove the client socket from the vector after handling
    clientMutex.lock();
    auto it = find(connectedClients.begin(), connectedClients.end(), clientSocket);
    if (it != connectedClients.end()) {
        connectedClients.erase(it);
    }
    clientMutex.unlock();

    // Cleanup and close socket
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

    // Accept incoming connections and handle each client in a separate thread
    while (true) {
        // Accept a new connection
        SOCKET clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientAddrLen);
        if (clientSocket == INVALID_SOCKET) {
            cerr << "Accept failed.\n";
            closesocket(serverSocket);
            WSACleanup();
            return 1;
        }

        cout << "New client connected.\n";

        // Add client socket to vector of connected clients
        clientMutex.lock();
        connectedClients.push_back(clientSocket);
        clientMutex.unlock();

        // Create thread to handle communication with the client
        thread clientThread(handleClient, clientSocket);
        clientThread.detach(); // Detach thread to allow it to run independently
    }

    // Cleanup
    closesocket(serverSocket);
    WSACleanup();

    return 0;
}
