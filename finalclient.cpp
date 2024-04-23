#include <iostream>     // Input-output stream library
#include <winsock2.h>   // Windows Socket API
#include <string>       // String manipulation library
#include <thread>       // Multi-threading support
using namespace std;

#pragma comment(lib, "ws2_32.lib")  // Link the ws2_32 library

// Function to encrypt a string using Caesar cipher
string encrypt(const string& plain_text, int key) {
    string cipher_text = "";   // Initialize an empty string for the encrypted text
    string alphabet = "abcdefghijklmnopqrstuvwxyz";  // Define the alphabet
    for (char i : plain_text) { // Loop through each character in the plain text
        char c = tolower(i);    // Convert character to lowercase
        if (isalpha(c)) {       // Check if the character is alphabetic
            int index = alphabet.find(c);   // Find the index of the character in the alphabet
            char encrypted_char = alphabet[(index + key) % 26];  // Apply Caesar cipher encryption
            cipher_text += (isupper(i)) ? toupper(encrypted_char) : encrypted_char;  // Preserve case
        } else {
            cipher_text += i;   // Preserve non-alphabet characters
        }
    }
    return cipher_text;    // Return the encrypted text
}

// Function to decrypt a string using Caesar cipher
string decrypt(const string& cipher_text, int key) {
    string plain_text = "";     // Initialize an empty string for the decrypted text
    string alphabet = "abcdefghijklmnopqrstuvwxyz";  // Define the alphabet
    for (char i : cipher_text) { // Loop through each character in the cipher text
        char c = tolower(i);    // Convert character to lowercase
        if (isalpha(c)) {       // Check if the character is alphabetic
            int index = alphabet.find(c);   // Find the index of the character in the alphabet
            char decrypted_char = alphabet[(index - key + 26) % 26];  // Apply Caesar cipher decryption
            plain_text += (isupper(i)) ? toupper(decrypted_char) : decrypted_char;  // Preserve case
        } else {
            plain_text += i;    // Preserve non-alphabet characters
        }
    }
    return plain_text;   // Return the decrypted text
}

// Function to receive and display messages from the server
void receive_messages(SOCKET sock) {
    char buffer[1024];      // Buffer to store received data
    int bytesReceived;      // Variable to track the number of bytes received
    while (true) {          // Loop indefinitely
        memset(buffer, 0, sizeof(buffer));   // Clear the buffer
        bytesReceived = recv(sock, buffer, sizeof(buffer), 0);  // Receive data from server
        if (bytesReceived > 0) {   // Check if data was received successfully
            string decryptedMessage = decrypt(buffer, 3);  // Decrypt received message
            cout << "\n" << decryptedMessage << endl;   // Display decrypted message
        } else {
            cerr << "Server disconnected." << endl;    // Server disconnected or error occurred
            break;  // Exit the loop
        }
    }
}

// Main function
int main() {
    WSADATA wsaData;            // Structure containing information about Windows Socket implementation
    SOCKET clientSocket;        // Socket descriptor
    struct sockaddr_in serverAddr;  // Structure for server address

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed." << endl;   // Display error message if initialization fails
        return 1;   // Return with error code
    }

    // Create socket
    if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        cerr << "Failed to create socket." << endl;    // Display error message if socket creation fails
        WSACleanup();   // Cleanup Winsock
        return 1;       // Return with error code
    }

    // Server address and port
    serverAddr.sin_family = AF_INET;    // IPv4 address family
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");   // Server IP address (change as needed)
    serverAddr.sin_port = htons(8888);   // Server port (change as needed)

    // Connect to server
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "Connection failed." << endl;    // Display error message if connection fails
        closesocket(clientSocket);    // Close the client socket
        WSACleanup();   // Cleanup Winsock
        return 1;       // Return with error code
    }

    cout << "Welcome to Malak's chating app!" << endl;   // Display welcome message

    string name;    // Variable to store user's name
    int choice;     // Variable to store user's choice

    // Choose between account creation and login
    cout << "Enter '1' to create an account or '2' to login: ";
    cin >> choice;  // Read user input
    cin.ignore();   // Ignore newline character in buffer

    if (choice == 1) {
        // Account creation
        bool accountCreated = false;
        while (!accountCreated) {
            cout << "Enter your name: ";
            getline(cin, name); // Read user's name
            if (name.empty()) { // Validate name input
                cout << "Name cannot be empty. Please try again.\n";
                continue;   // Prompt user again if name is empty
            }
            cout << "Enter your password: ";
            string password;
            getline(cin, password); // Read user's password

            // Encrypt the password before sending to server
            string encryptedPassword = encrypt(password, 3);   // Encrypt password (using Caesar cipher)

            // Send the account creation request to the server
            char buffer[1024];
            string accountRequest = "CREATE " + name + " " + encryptedPassword;
            send(clientSocket, accountRequest.c_str(), accountRequest.size(), 0);

            // Wait for account creation response
            memset(buffer, 0, sizeof(buffer));
            int response = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (response > 0 && strcmp(buffer, "Account Created Successfully") == 0) {
                cout << "Account created successfully.\n";    // Display success message
                accountCreated = true;  // Set accountCreated flag to true
            } else {
                cout << "Account creation failed. Please try again.\n"; // Display failure message
                // Optionally, you can include more detailed error feedback here
            }
        }
    } else if (choice == 2) {
        // Login
        cout << "Enter your username: ";
        getline(cin, name); // Read user's username
        cout << "Enter your password: ";
        string password;
        getline(cin, password); // Read user's password

        // Encrypt the password before sending to server
        string encryptedPassword = encrypt(password, 3);   // Encrypt password (using Caesar cipher)

        // Send login request to server
        string loginRequest = "LOGIN " + name + " " + encryptedPassword;
        send(clientSocket, loginRequest.c_str(), loginRequest.size(), 0);

        // Wait for server response
        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        recv(clientSocket, buffer, sizeof(buffer), 0);
        if (strcmp(buffer, "Login Successful") == 0){
            cout << "Logged in successfully.\n";    // Display login success message
        } else {
            cout << "Login failed. Please check your username and password.\n";    // Display login failure message
            closesocket(clientSocket);  // Close the client socket
            WSACleanup();   // Cleanup Winsock
            return 3;       // Return with error code
        }
    } else {
        cerr << "Invalid choice. Exiting..." << endl;    // Display invalid choice message
        closesocket(clientSocket);  // Close the client socket
        WSACleanup();   // Cleanup Winsock
        return 1;       // Return with error code
    }

    // Create a thread to receive messages from the server
    thread recvThread(receive_messages, clientSocket);  // Create a thread to handle message reception
    recvThread.detach();    // Detach the thread to run independently

    string input;   // Variable to store user input
    while (true) {  // Loop indefinitely
        cout << "You (" << name << "): ";  // Display prompt
        getline(cin, input);    // Read user input

        if (input.empty()) {
            continue;   // Skip empty messages
        }

        // Format the message with client's name and send to server
        string fullMessage = name + ": " + input;  // Construct message
        string encryptedMessage = encrypt(fullMessage, 3);   // Encrypt message (using Caesar cipher)
        if (send(clientSocket, encryptedMessage.c_str(), encryptedMessage.size(), 0) == SOCKET_ERROR) {
            cerr << "Send failed." << endl;    // Display send failure message
            break;  // Exit the loop
        }
    }

    // Clean up: Close socket and cleanup Winsock
    closesocket(clientSocket);  // Close the client socket
    WSACleanup();   // Cleanup Winsock

    return 0;   // Return with success code
}
