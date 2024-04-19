#include <iostream>
#include <fstream>
#include <string>

using namespace std;

// Structure to represent user information
struct User {
    string name;
    string username;
    string password;
};

// Define a structure to represent a Node in the linked list
struct Node {
    User data;    // Store user data in this node
    Node* next;   // Pointer to the next node in the linked list
};

// Function prototypes
void addUserToFile(const User& newUser);
void readUsersFromFile(Node*& head);
void signUp(Node*& head);
bool login(Node* head);
void displayMenu();

// Function to add a new user to the file
void addUserToFile(const User& newUser) {
    ofstream outfile("users.txt", ios::app); // Open file in append mode
    if (!outfile) {
        cerr << "Error: Unable to open file for writing!" << endl;
        return;
    }
    outfile << newUser.name << " " << newUser.username << " " << newUser.password << endl;
    outfile.close();
}

// Function to retrieve user information from the file and store it in a linked list
void readUsersFromFile(Node*& head) {
    ifstream infile("users.txt"); // Open file for reading
    if (!infile) {
        cerr << "Error: Unable to open file!" << endl;
        return;
    }

    User user;
    while (infile >> user.name >> user.username >> user.password) {
        Node* newNode = new Node; // Create a new node
        newNode->data = user;     // Assign user data to the new node
        newNode->next = head;     // Insert at the beginning of the linked list
        head = newNode;           // Update the head to the new node
    }

    infile.close();
}

// Function to sign up a new user
void signUp(Node*& head) {
    User newUser;
    cout << "Enter your name: ";
    cin.ignore(); // Clear the newline character from the input buffer
    getline(cin, newUser.name);
    cout << "Enter your username: ";
    cin >> newUser.username;
    cout << "Enter your password: ";
    cin >> newUser.password;

    addUserToFile(newUser); // Add the new user to the file

    // Notify user that signup was successful
    cout << "User signed up successfully!" << endl;

    // Ask the user if they want to login
    char choice;
    cout << "Do you want to login now (y/n)? ";
    cin >> choice;
    if (choice == 'y' || choice == 'Y') {
        // Create a new node for the signed-up user
        Node* newNode = new Node;
        newNode->data = newUser;
        newNode->next = head;
        head = newNode;
        
        login(head); // Call the login function
    } else {
        cout << "Exiting the program. Goodbye!" << endl;
    }
}

// Function to log in an existing user
bool login(Node* head) {
    string username, password;
    cout << "Enter your username: ";
    cin >> username;
    cin.ignore(); // Ignore the newline character left in the input buffer
    cout << "Enter your password: ";
    getline(cin, password); // Read the entire line for the password

    Node* current = head;
    while (current != nullptr) {
        if (current->data.username == username && current->data.password == password) {
            cout << "Login successful!" << endl;
            // Present options to the user
            cout << "Welcome, " << current->data.name << "!" << endl;
            displayMenu();
            return true;
        }
        current = current->next;
    }

    cout << "Invalid username or password!" << endl;

    // Prompt the user to retry login or exit
    char retryChoice;
    cout << "Do you want to retry login (l) or exit (e)? ";
    cin >> retryChoice;
    if (retryChoice == 'l' || retryChoice == 'L') {
        login(head); // Retry login
    } else {
        cout << "Exiting the program. Goodbye!" << endl;
    }
    return false;
}

// Function to display menu options after successful login
void displayMenu() {
    cout << "Options:\n"
         << "1. Generate Password\n"
         << "2. Retrieve Password\n"
         << "3. Add Password\n"
         << "4. Delete Password\n"
         << "5. Display Password List\n";
}

// Main function
int main() {
    Node* userList = nullptr; // Initialize linked list to store user information
    readUsersFromFile(userList); // Read user information from file and store in linked list

    int choice;
    cout << "Welcome to User Authentication System!" << endl;
    cout << "1. Sign Up\n2. Login\nEnter your choice: ";
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
