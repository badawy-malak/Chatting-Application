#include <iostream>
#include <winsock2.h>
#include <string>
#include <thread>
using namespace std;

#pragma comment(lib, "ws2_32.lib")

// Function to encrypt a string using Caesar cipher
string encryptMessage(const string& message, int key) {
    string encryptedMessage = "";
    string alphabet = "abcdefghijklmnopqrstuvwxyz";
    for (char i : message) {
        char c = tolower(i);
        if (isalpha(c)) {
            int index = alphabet.find(c);
            char encrypted_char = alphabet[(index + key) % 26];
            encryptedMessage += (isupper(i)) ? toupper(encrypted_char) : encrypted_char;
        } else {
            encryptedMessage += i; // Preserve non-alphabet characters
        }
    }
    return encryptedMessage;
}

// Function to decrypt a string using Caesar cipher
string decryptMessage(const string& encryptedMessage, int key) {
    string decryptedMessage = "";
    string alphabet = "abcdefghijklmnopqrstuvwxyz";
    for (char i : encryptedMessage) {
        char c = tolower(i);
        if (isalpha(c)) {
            int index = alphabet.find(c);
            char decrypted_char = alphabet[(index - key + 26) % 26];
            decryptedMessage += (isupper(i)) ? toupper(decrypted_char) : decrypted_char;
        } else {
            decryptedMessage += i; // Preserve non-alphabet characters
        }
    }
    return decryptedMessage;
}

void receiveMessages(SOCKET sock) {
    char buffer[1024];
    int bytesReceived;
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        bytesReceived = recv(sock, buffer, sizeof(buffer), 0);
        if (bytesReceived > 0) {
            string receivedMessage = string(buffer);
            string decryptedMessage = decryptMessage(receivedMessage, 3); // Example key: 3
            cout << "Received: " << decryptedMessage << endl;
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

    cout << "Connected to chat server.\n";

    // Create a thread to receive messages from the server
    thread recvThread(receiveMessages, clientSocket);
    recvThread.detach();

    string name;
    cout << "Enter your name: ";
    getline(cin, name);

    string input;
    while (true) {
        cout << "You (" << name << "): ";
        getline(cin, input);

        if (input.empty()) {
            continue; // Skip empty messages
        }

        // Encrypt the message before sending to the server
        string encryptedMessage = encryptMessage(input, 3); // Example key: 3

        // Send the encrypted message to the server
        send(clientSocket, encryptedMessage.c_str(), encryptedMessage.size(), 0);
    }

    closesocket(clientSocket);
    WSACleanup();

    return 0;
}
