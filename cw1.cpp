#include <iostream>
#include <string>
#include <fstream>     // Reading from & Writing to files
#include <cstdlib>     // Generating random numbers
#include <ctime>       // Unique password generation
#include <sstream>     // Converting data types to and from a string
#include <algorithm>   // Transform uppercases to lowercases for the encryption process
using namespace std;

// Structure to represent user information
struct User {
    string name;
    string username;
    string password;
    string fileName; // File name for the user's password list
    int numPasswords; // Number of passwords stored
};

// Define a structure to represent a Node in the linked list
struct Node {
    User data;    // Store user data in this node
    Node* next;   // Pointer to the next node in the linked list
};

// Functions declaration
void addUser(Node*& head, const User& newUser);
void readUsersFromFile(Node*& head);
void signUp(Node*& head);
bool login(Node* head);
void displayMenu();
string generateRandomPassword();
void addPassword(Node* userNode, const string& website, const string& password);
void deletePassword(Node* userNode, const string& website);
void displayPasswordList(Node* userNode);
string encrypt(const string& plain_text, int key); // Added encryption function prototype
string decrypt(const string& cipher_text, int key); // Added decryption function prototype
void updatePassword(Node* userNode, const string& website);


// Function to add a new user to the linked list
void addUser(Node*& head, const User& newUser) {
    Node* newNode = new Node; // Create a new node
    newNode->data = newUser;  // Assign user data to the new node
    newNode->next = nullptr;  // Set the next pointer to nullptr

    if (head == nullptr) {    // If the linked list is empty
        head = newNode;       // Set the new node as the head of the linked list
    }
    else {                    // If the linked list is not empty
        Node* current = head; // Start at the head of the linked list
        while (current->next != nullptr) { // Traverse to the end of the linked list
            current = current->next;       // Move to the next node
        }
        current->next = newNode; // Link the new node to the last node in the list
    }
}

// Function to retrieve user information from the file and store it in a linked list
void readUsersFromFile(Node*& head) {
    ifstream infile("users.txt"); // Open file for reading
    if (!infile) {
        cerr << "Error: Unable to open file!" << endl;
        return;
    }

    User user;
    while (infile >> user.name >> user.username >> user.password >> user.fileName) {
        addUser(head, user); // Add user to the linked list
    }

    infile.close();
}

void signUp(Node*& head) {
    User newUser;
    while (true) {
        cout << "Enter your name: ";
        cin.ignore(); // Clear the newline character from the input buffer
        getline(cin, newUser.name);
        if (!newUser.name.empty()) {
            break; // Exit loop if name is not empty
        }
        cout << "Error: Name cannot be empty!" << endl;
    }

    while (true) {
        cout << "Enter your username: ";
        cin >> newUser.username;
        if (!newUser.username.empty()) {
            break; // Exit loop if username is not empty
        }
        cout << "Error: Username cannot be empty!" << endl;
    }

    while (true) {
        cout << "Enter your password: ";
        cin >> newUser.password;
        if (!newUser.password.empty()) {
            break; // Exit loop if password is not empty
        }
        cout << "Error: Password cannot be empty!" << endl;
    }

    // Generate a unique file name for the user's password list
    newUser.fileName = newUser.name + "_passwords.txt";

    // Encrypt the password before storing it
    newUser.password = encrypt(newUser.password, 3);

    // Add the new user to the linked list
    addUser(head, newUser);

    // Create a file for the new user's password list
    ofstream userFile(newUser.fileName);
    if (!userFile) {
        cerr << "Error: Unable to create user file!" << endl;
        return;
    }
    userFile.close();

    // Write user information to the users.txt file
    ofstream usersFile("users.txt", ios::app); // Open file in append mode
    if (!usersFile) {
        cerr << "Error: Unable to open users file!" << endl;
        return;
    }
    usersFile << newUser.name << " " << newUser.username << " " << newUser.password << " " << newUser.fileName << endl;
    usersFile.close();

    // Notify user that signup was successful
    cout << "User signed up successfully!" << endl;

    // Ask the user if they want to login
    char choice;
    cout << "Do you want to login now? (y/n): ";
    cin >> choice;
    if (choice == 'y' || choice == 'Y') {
        login(head); // Call the login function
    }
}

// Function to log in an existing user
bool login(Node* head) {
    string username, password;
    cout << "Enter your username: ";
    cin >> username;
    cout << "Enter your password: ";
    cin >> password;

    // Search for the user in the linked list
    Node* current = head;
    while (current != nullptr) {
        if (current->data.username == username) {
            // Decrypt the stored password and compare with the input password
            string decryptedPassword = decrypt(current->data.password, 3);
            if (password == decryptedPassword) {
                cout << "Login successful!" << endl;
                // Present options to the user
                cout << "Welcome back, " << current->data.name << "!\nLets get started :)" << endl;

                // Handle user options
                int option;
                cout<<"\n";
                displayMenu();
                while (true) {
                    cout << "\nEnter your choice: ";
                    cin >> option;
                    switch (option) {
                        case 1:
                            {
                                string website;
                                string password;
                                cout << "Enter website name: ";
                                cin >> website;
                                cout << "Enter password: ";
                                cin >> password;
                                addPassword(current, website, password);
                            }
                            break;
                        case 2:
                         {
                                string website;
                                cout << "Enter website name to update password: ";
                                cin >> website;
                                updatePassword(current, website);
                        }
                        break;
                        case 3:
                            {
                                string website;
                                cout << "Enter website name to delete password: ";
                                cin >> website;
                                deletePassword(current, website);
                            }
                            break;
                        case 5:
                            displayPasswordList(current);
                            break;                        case 4:
                        {
                            string website;
                            string generatedPassword;
                            cout << "Enter the website name for which you want to generate a password: ";
                            cin >> website;
                            generatedPassword = generateRandomPassword();
                            addPassword(current,website,generatedPassword);
                        }
                        break;
                        case 6:
                        return false;
                        break;
                        default:
                            cout << "Invalid option!" << endl;
                            break;
                    }
                    cout << "Do you want to continue? (y/n): ";
                    char cont;
                    cin >> cont;
                    if (cont != 'y' && cont != 'Y') {
                        break;
                    }
                }
                return true; // Login successful, exit the function
            } else {
                cout << "Invalid password!" << endl;
                break; // Exit loop and prompt for login or sign up
            }
        }
        current = current->next;
    }

    cout << "User not found!" << endl;

    // Ask user if they want to login again or sign up
    cout << "Do you want to login again (l), or do you want to signup (s), or do you want to exit (e)? ";
    char choice;
    cin >> choice;
    if (choice == 'l' || choice == 'L') {
        login(head);
    } else if (choice == 's' || choice == 'S') {
        signUp(head);
    } else if(choice =='e' || choice =='E') {
        return false;
    }else{
        cout << "Invalid choice!" << endl;

    }

    return false; // User not found or invalid login, exit the function
}


// Function to display menu options after successful login
void displayMenu() {
    cout << "List of Option to manage your passwords:\n"
         << "1. Add Password to your list\n"
         << "2. Update an old password from your list\n"
         << "3. Delete an existing Password from your list\n"
         << "4. Generate a random Password to a website of your choice\n"
         << "5. Display your Password List\n"
         << "6. Exit the program\n";
}

// Function to add a password for a website to the user's information
void addPassword(Node* userNode, const string& website, const string& password) {
    // Open the user's password file
    ofstream userFile(userNode->data.fileName, ios::app);
    if (!userFile) {
        cerr << "Error: Unable to open user file!" << endl;
        return;
    }

    // Encrypt the password before storing it
    string encryptedPassword = encrypt(password, 3);

    // Write the website name and encrypted password to the file
    userFile << website << " " << encryptedPassword << endl;

    // Close the file
    userFile.close();

    cout << "Password added successfully!" << endl;
}

// Function to delete a password for a website from the user's information
void deletePassword(Node* userNode, const string& website) {
    // Open the user's password file
    ifstream userFile(userNode->data.fileName);
    if (!userFile) {
        cerr << "Error: Unable to open user file!" << endl;
        return;
    }

    // Create a temporary file to store the updated password list
    string tempFileName = userNode->data.fileName + ".temp";
    ofstream tempFile(tempFileName);
    if (!tempFile) {
        cerr << "Error: Unable to create temporary file!" << endl;
        userFile.close();
        return;
    }

    // Read passwords from the original file, skipping the one to be deleted
    string storedWebsite, storedPassword;
    bool deleted = false;
    while (userFile >> storedWebsite >> storedPassword) {
        if (storedWebsite == website) {
            deleted = true;
            continue; // Skip writing the deleted entry to the temporary file
        }
        tempFile << storedWebsite << " " << storedPassword << endl;
    }

    // Close the files
    userFile.close();
    tempFile.close();

    // Replace the original file with the temporary file
    remove(userNode->data.fileName.c_str());            // Delete the original file
    rename(tempFileName.c_str(), userNode->data.fileName.c_str()); // Rename the temporary file

    if (deleted) {
        cout << "Password for " << website << " deleted successfully!" << endl;
    } else {
        cout << "Password for " << website << " not found!" << endl;
    }
}

void updatePassword(Node* userNode, const string& website) {
    // Open the user's password file
    ifstream userFile(userNode->data.fileName);
    if (!userFile) {
        cerr << "Error: Unable to open user file!" << endl;
        return;
    }

    // Create a temporary file to store the updated password list
    string tempFileName = userNode->data.fileName + ".temp";
    ofstream tempFile(tempFileName);
    if (!tempFile) {
        cerr << "Error: Unable to create temporary file!" << endl;
        userFile.close();
        return;
    }

    // Read passwords from the original file, updating the one for the given website
    string storedWebsite, storedPassword;
    bool updated = false;
    cout << "Enter the old password for " << website << ": ";
    string oldPassword;
    cin >> oldPassword;
    string oldEncryptedPassword = encrypt(oldPassword, 3);
    while (userFile >> storedWebsite >> storedPassword) {
        if (storedWebsite == website) {
            if (storedPassword == oldEncryptedPassword) {
                string newPassword;
                cout << "Enter the new password for " << website << ": ";
                cin >> newPassword;
                string newEncryptedPassword = encrypt(newPassword, 3);
                tempFile << website << " " << newEncryptedPassword << endl;
                updated = true;
            } else {
                cout << "Invalid old password for " << website << "!" << endl;
                tempFile << storedWebsite << " " << storedPassword << endl;
            }
        } else {
            tempFile << storedWebsite << " " << storedPassword << endl;
        }
    }

    // Close the files
    userFile.close();
    tempFile.close();

    // Replace the original file with the temporary file
    remove(userNode->data.fileName.c_str());            // Delete the original file
    rename(tempFileName.c_str(), userNode->data.fileName.c_str()); // Rename the temporary file

    if (updated) {
        cout << "Password for " << website << " updated successfully!" << endl;
    } else {
        cout << "Password for " << website << " not found!" << endl;
    }
}


// Function to display all passwords for a user
void displayPasswordList(Node* userNode) {
    // Open the user's password file
    ifstream userFile(userNode->data.fileName);
    if (!userFile) {
        cerr << "Error: Unable to open user file!" << endl;
        return;
    }

    // Read passwords from the file and decrypt them
    string website, encryptedPassword;
    while (userFile >> website >> encryptedPassword) {
        string decryptedPassword = decrypt(encryptedPassword, 3);
        cout << "Website: " << website << ", Password: " << decryptedPassword << endl;
    }

    // Close the file
    userFile.close();
}

// Function to generate a random password of given length
string generateRandomPassword() {
    const string validChars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()_+-=[]{}|;:,.<>?";

    srand(static_cast<unsigned int>(time(nullptr)));

    string password;
    for (int i = 0; i < 10; ++i) {
        int randomIndex = rand() % validChars.length();
        password += validChars[randomIndex];
    }

    return password;
}


string toLowerCase(const std::string& str) {
    string lowerCaseStr = str;
    transform(lowerCaseStr.begin(), lowerCaseStr.end(), lowerCaseStr.begin(), ::tolower);
    return lowerCaseStr;
}
// Function to encrypt a string using Caesar cipher
string encrypt(const string& plain_text, int key) {
    
    string cipher_text = "";
    string alphabet = "abcdefghijklmnopqrstuvwxyz";
    string lowerCaseInput = toLowerCase(plain_text);
    for (char i : lowerCaseInput) {
        for (int j = 0; j < alphabet.length(); j++) {
            if (i == alphabet[j]) {
                cipher_text += alphabet[(j + key) % 26];
                break;
            }
        }
    }
    return cipher_text;
}

// Function to decrypt a string using Caesar cipher
string decrypt(const string& cipher_text, int key) {
    string plain_text = "";
    string alphabet = "abcdefghijklmnopqrstuvwxyz";
    for (char i : cipher_text) {
        for (int j = 0; j < alphabet.length(); j++) {
            if (i == alphabet[j]) {
                plain_text += alphabet[(j - key + 26) % 26];
                break;
            }
        }
    }
    return plain_text;
}



// Main function
int main() {
    Node* userList = nullptr; // Initialize linked list to store user information
    readUsersFromFile(userList); // Read user information from file and store in linked list

    int choice;
    cout << "Welcome to our Password Manager!\nLets get started:)"<< endl;
    cout << "1. Are you a New User: Create an account!\n2. Are you an existing User: Login your account\nEnter your choice: ";
    cin >> choice;

    switch (choice) {
        case 1:
            signUp(userList);
            break;
        case 2:
            login(userList);
            break;
        default:
            cout << "Invalid choice!" << endl;
            break;
    }

    return 0;
}