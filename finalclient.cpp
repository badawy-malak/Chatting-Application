#include <iostream>
#include <winsock2.h>
#include <string>
#include <thread>
using namespace std;

#pragma comment(lib, "ws2_32.lib")

// Function to encrypt a string using Caesar cipher
string encrypt(const string& plain_text, int key) {
    string cipher_text = "";
    string alphabet = "abcdefghijklmnopqrstuvwxyz";
    for (char i : plain_text) {
        char c = tolower(i);
        if (isalpha(c)) {
            int index = alphabet.find(c);
            char encrypted_char = alphabet[(index + key) % 26];
            cipher_text += (isupper(i)) ? toupper(encrypted_char) : encrypted_char;
        } else {
            cipher_text += i; // Preserve non-alphabet characters
        }
    }
    return cipher_text;
}

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

// Function to receive and display messages from the server
void receive_messages(SOCKET sock) {
    char buffer[1024];
    int bytesReceived;
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        bytesReceived = recv(sock, buffer, sizeof(buffer), 0);
        if (bytesReceived > 0) {
            string decryptedMessage = decrypt(buffer, 3); // Decrypt received message
            cout << decryptedMessage << endl;
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

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed." << endl;
        return 1;
    }

    // Create socket
    if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        cerr << "Failed to create socket." << endl;
        WSACleanup();
        return 1;
    }

    // Server address and port
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Change to server IP address
    serverAddr.sin_port = htons(8888); // Change to server port

    // Connect to server
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "Connection failed." << endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    cout << "Welcome to Malak's chating app!" << endl;
    string name;
    int choice;

    // Choose between account creation and login
    cout << "Enter '1' to create an account or '2' to login: ";
    cin >> choice;
    cin.ignore(); // Ignore newline character in buffer

    if (choice == 1) {
        // Account creation
    bool accountCreated = false;
        cout << "Enter your name: ";
        getline(cin, name);
        cout << "Enter your password: ";
        string password;
        getline(cin, password);

        // Encrypt the password before sending to server
        string encryptedPassword = encrypt(password, 3);

        // Send account creation request to server
        string accountRequest = "CREATE " + name + " " + encryptedPassword;
        send(clientSocket, accountRequest.c_str(), accountRequest.size(), 0);

        // Wait for server response
        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        recv(clientSocket, buffer, sizeof(buffer), 0);
        cout << buffer << endl; // Display server response
        int response = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (response > 0 && strcmp(buffer, "Account Created Successfully") == 0) {
            cout << "Account created successfully.\n";
            accountCreated = true;
        } else {
            cout << "Account creation failed. Please try again.\n";
        closesocket(clientSocket);
        WSACleanup();
        return 1;
        }
    } else if (choice == 2) {
        // Login
        cout << "Enter your username: ";
        getline(cin, name);
        cout << "Enter your password: ";
        string password;
        getline(cin, password);

        // Encrypt the password before sending to server
        string encryptedPassword = encrypt(password, 3);

        // Send login request to server
        string loginRequest = "LOGIN " + name + " " + encryptedPassword;
        send(clientSocket, loginRequest.c_str(), loginRequest.size(), 0);

        // Wait for server response
        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        recv(clientSocket, buffer, sizeof(buffer), 0);
        if (strcmp(buffer, "Login Successful") == 0){
            cout << "Logged in successfully.\n";
        } else {
            cout << "Login failed. Please check your username and password.\n";
            closesocket(clientSocket);
            WSACleanup();
            return 3;
        }
    } else {
        cerr << "Invalid choice. Exiting..." << endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

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
        string encryptedMessage = encrypt(fullMessage, 3);
        if (send(clientSocket, encryptedMessage.c_str(), encryptedMessage.size(), 0) == SOCKET_ERROR) {
            cerr << "Send failed." << endl;
            break;
        }
    }

    // Clean up: Close socket and cleanup Winsock
    closesocket(clientSocket);
    WSACleanup();

    return 0;
}
