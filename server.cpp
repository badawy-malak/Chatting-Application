#include <iostream>
#include <winsock2.h>
#include <thread>
#include <vector>
#include <algorithm>
#include <string>
#include <fstream>
#include <mutex>
using namespace std;

#pragma comment(lib, "ws2_32.lib")

vector<SOCKET> connectedClients;
mutex clientMutex;

void handleClient(SOCKET clientSocket) {
    char buffer[1024];
    int bytesReceived;

    // Receive username and password from client
    memset(buffer, 0, sizeof(buffer));
    bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived > 0) {
        string username(buffer);
        cout << "New client connected: " << username << endl;

        // Store username and password in a text file (users.txt)
        ofstream outfile("users.txt", ios::app);
        if (outfile.is_open()) {
            outfile << username << endl;
        }
        outfile.close();
    }

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived == SOCKET_ERROR || bytesReceived == 0) {
            cerr << "Client disconnected.\n";
            break;
        }

        cout << buffer << endl;

        // Relay message to all other clients
        clientMutex.lock();
        for (SOCKET& otherClientSocket : connectedClients) {
            if (otherClientSocket != clientSocket) {
                if (send(otherClientSocket, buffer, bytesReceived, 0) == SOCKET_ERROR) {
                    cerr << "Send failed.\n";
                    // Handle send error
                }
            }
        }
        clientMutex.unlock();
    }

    // Remove client socket from vector after handling
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

        clientMutex.lock();
        connectedClients.push_back(clientSocket);
        clientMutex.unlock();

        thread clientThread(handleClient, clientSocket);
        clientThread.detach();
    }

    closesocket(serverSocket);
    WSACleanup();

    return 0;
}
