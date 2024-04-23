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


void receive_messages(SOCKET sock) {
    char buffer[1024];
    int bytesReceived;
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        bytesReceived = recv(sock, buffer, sizeof(buffer), 0);
        if (bytesReceived > 0) {
            string decryptedMessage = decrypt(buffer,3);
            cout << "\n"<<decryptedMessage << endl;
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

        // Encrypt the password before sending to server
        string encryptedPassword = encrypt(password, 3); // Example key: 3

        // Send the account creation request to the server
        string accountRequest = "CREATE " + name + " " + encryptedPassword;
        send(clientSocket, accountRequest.c_str(), accountRequest.size(), 0);

        // Wait for account creation success message
        memset(buffer, 0, sizeof(buffer));
        recv(clientSocket, buffer, sizeof(buffer), 0);
        if (strcmp(buffer, "Account Created Successfully") == 0) {
            cout << "Account created successfully.\n";
        } else if (strcmp(buffer, "The user name already exist.") == 0) {
            cout << "A user with the same name has an account. Try creating an account with another name or login to the existing acouont.\nProgram Exiting..";
            closesocket(clientSocket);
            WSACleanup();
            return 0;
        }else {
            cout << "Account creation failed.\n";
            closesocket(clientSocket);
            WSACleanup();
            return 0;
        }
    } else if (choice == 2) {
        // Login
        cout << "Enter your username: ";
        getline(cin, name);
        cout << "Enter your password: ";
        string password;
        getline(cin, password);

        // Encrypt the password before sending to server
        string encryptedPassword = encrypt(password, 3); // Example key: 3

        // Send the login request to the server
        string loginRequest = "LOGIN " + name + " " + encryptedPassword;
        send(clientSocket, loginRequest.c_str(), loginRequest.size(), 0);

        // Wait for login success message
        memset(buffer, 0, sizeof(buffer));
        recv(clientSocket, buffer, sizeof(buffer), 0);
        if (strcmp(buffer, "Login Successful") == 0) {
            cout << "Logged in successfully.\n";
        } else {
            cout << "Login failed. Please check your username and password.\n";
            closesocket(clientSocket);
            WSACleanup();
            return 0;
        }
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
        string encryptedMessage = encrypt(fullMessage,3);
        if (send(clientSocket, encryptedMessage.c_str(), encryptedMessage.size(), 0) == SOCKET_ERROR) {
            cerr << "Send failed.\n";
            break;
        }
    }

    closesocket(clientSocket);
    WSACleanup();

    return 0;
}