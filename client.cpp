#include <iostream>
#include <winsock2.h>
#include <string>
#include <thread>
using namespace std;

#pragma comment(lib, "ws2_32.lib")

void receive_messages(SOCKET sock) {
    char buffer[1024];
    int bytesReceived;
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        bytesReceived = recv(sock, buffer, sizeof(buffer), 0);
        if (bytesReceived > 0) {
            cout << buffer << endl;
        } else {
            cerr << "Server disconnected." << endl;
            break;
        }
    }
}

int main() {
    WSADATA wsaData;
    SOCKET clientSocket;
    struct sockaddr_in serverAddr;
    char buffer[1024];

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed.\n";
        return 1;
    }

    if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        cerr << "Failed to create socket.\n";
        WSACleanup();
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_port = htons(8888);

    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "Connection failed.\n";
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    cout << "Welcome to the chat!\n";

    // Prompt for account creation or login
    string name;
    int choice;
    cout << "Enter '1' to create an account or '2' to login: ";
    cin >> choice;
    cin.ignore(); // Ignore newline character in buffer

    if (choice == 1) {
        // Account creation
        cout << "Enter your name: ";
        getline(cin, name);
        cout << "Enter your password: ";
        string password;
        getline(cin, password);

        // Send the account creation request to the server
        string accountRequest = name + " " + password;
        send(clientSocket, accountRequest.c_str(), accountRequest.size(), 0);

    } else if (choice == 2) {
        // Display message and exit
        cout << "Logging in is still a work in progress. Exiting...\n";
        closesocket(clientSocket);
        WSACleanup();
        return 0;
    } else {
        cerr << "Invalid choice. Exiting...\n";
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    cout << "Connected to chat server.\n";

    // Create a thread to receive messages from the server
    thread recvThread(receive_messages, clientSocket);
    recvThread.detach();

    string input;
    while (true) {
        cout << "You (" << name << "): ";
        getline(cin, input);

        if (input.empty()) {
            continue; // Skip empty messages
        }

        // Format the message with client's name and send to server
        string fullMessage = name + ": " + input;
        if (send(clientSocket, fullMessage.c_str(), fullMessage.size(), 0) == SOCKET_ERROR) {
            cerr << "Send failed.\n";
            break;
        }
    }

    closesocket(clientSocket);
    WSACleanup();

    return 0;
}
