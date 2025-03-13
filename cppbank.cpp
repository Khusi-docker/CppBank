#include <iostream>
#include <cstdlib>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <chrono>
#include <thread>
#include <vector>

using namespace std;
using json = nlohmann::json;

const string FIREBASE_URL = "https://bank-9c0da-default-rtdb.firebaseio.com/";
const string JSON_EXT = ".json";
const double MAX_TRANSACTION_AMOUNT = 100000.0; // Maximum deposit/withdrawal limit
const double MIN_BALANCE = 100.0; // Minimum balance required
const int MAX_LOGIN_ATTEMPTS = 3; // Maximum password attempts

size_t callback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

void loadingEffect(const string& message) {
    cout << "\033[1;33m" << message;
    for (int i = 0; i < 3; ++i) {
        cout << ".";
        cout.flush();
        this_thread::sleep_for(chrono::milliseconds(500));
    }
    cout << "\033[0m" << endl;
}

string getFromFirebase(const string& path) {
    loadingEffect("Fetching data");
    CURL* curl = curl_easy_init();
    string response;
    if (curl) {
        string url = FIREBASE_URL + path + JSON_EXT;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            cerr << "\033[1;31mError: " << curl_easy_strerror(res) << "\033[0m" << endl;
        }
        curl_easy_cleanup(curl);
    }
    return response;
}

void sendToFirebase(const string& path, const string& data, const string& method) {
    loadingEffect("Updating data");
    CURL* curl = curl_easy_init();
    if (curl) {
        string url = FIREBASE_URL + path + JSON_EXT;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
}

void displayIntroAnimation() {
    cout << "\033[1;36m";
    cout << "============================================\n";
    cout << "|                                          |\n";
    cout << "|          WELCOME TO CPP BANK             |\n";
    cout << "|                                          |\n";
    cout << "============================================\n";
    cout << "\033[0m";
    this_thread::sleep_for(chrono::milliseconds(1000));
    cout << "\033[1;32mLoading your banking experience";
    for (int i = 0; i < 3; ++i) {
        cout << ".";
        cout.flush();
        this_thread::sleep_for(chrono::milliseconds(500));
    }
    cout << "\033[0m" << endl;
}

void createAccount() {
    string accNo, name, password;
    double balance;
    cout << "\033[1;34mEnter Account Number: \033[0m";
    cin >> accNo;
    
    // Check if account already exists
    string existingData = getFromFirebase("accounts/" + accNo);
    if (!existingData.empty() && existingData != "null") {
        cout << "\033[1;31mAccount already exists!\033[0m\n";
        return;
    }
    
    cout << "\033[1;34mEnter Name: \033[0m";
    cin >> name;
    cout << "\033[1;34mEnter Password: \033[0m";
    cin >> password;
    do {
        cout << "\033[1;34mEnter Initial Balance (Min: " << MIN_BALANCE << "): \033[0m";
        cin >> balance;
    } while (balance < MIN_BALANCE);
    
    string data = "{\"name\": \"" + name + "\", \"password\": \"" + password + "\", \"balance\": " + to_string(balance) + "}";
    sendToFirebase("accounts/" + accNo, data, "PUT");
    cout << "\033[1;32mAccount Created Successfully!\033[0m\n";
}

bool authenticate(const string& accNo) {
    string storedData = getFromFirebase("accounts/" + accNo);
    if (storedData.empty() || storedData == "null") {
        cout << "\033[1;31mAccount not found!\033[0m\n";
        return false;
    }
    
    try {
        json accountData = json::parse(storedData);
        string storedPassword = accountData["password"];
        string enteredPassword;
        
        for (int attempts = 0; attempts < MAX_LOGIN_ATTEMPTS; ++attempts) {
            cout << "\033[1;34mEnter Password: \033[0m";
            cin >> enteredPassword;
            if (enteredPassword == storedPassword) {
                return true;
            }
            cout << "\033[1;31mIncorrect Password! Attempts left: " << (MAX_LOGIN_ATTEMPTS - attempts - 1) << "\033[0m\n";
        }
        cout << "\033[1;31mToo many incorrect attempts. Access denied!\033[0m\n";
        return false;
    } catch (json::parse_error& e) {
        cout << "\033[1;31mJSON Parsing Error: \033[0m" << e.what() << endl;
        return false;
    }
}

void changePassword(const string& accNo) {
    string newPassword;
    cout << "\033[1;34mEnter New Password: \033[0m";
    cin >> newPassword;

    string storedData = getFromFirebase("accounts/" + accNo);
    json accountData = json::parse(storedData);
    accountData["password"] = newPassword;

    sendToFirebase("accounts/" + accNo, accountData.dump(), "PUT");
    cout << "\033[1;32mPassword Changed Successfully!\033[0m\n";
}

void viewAccountDetails(const string& accNo) {
    string storedData = getFromFirebase("accounts/" + accNo);
    json accountData = json::parse(storedData);
    cout << "\033[1;34mAccount Number: \033[0m" << accNo << endl;
    cout << "\033[1;34mName: \033[0m" << accountData["name"] << endl;
    cout << "\033[1;34mBalance: \033[0m" << accountData["balance"] << endl;
}

void depositMoney(const string& accNo) {
    double amount;
    cout << "\033[1;34mEnter Amount to Deposit: \033[0m";
    cin >> amount;
    
    if (amount <= 0 || amount > MAX_TRANSACTION_AMOUNT) {
        cout << "\033[1;31mInvalid Amount!\033[0m\n";
        return;
    }
    
    string storedData = getFromFirebase("accounts/" + accNo);
    json accountData = json::parse(storedData);
    double currentBalance = accountData["balance"];
    double newBalance = currentBalance + amount;
    accountData["balance"] = newBalance;
    
    sendToFirebase("accounts/" + accNo, accountData.dump(), "PUT");
    cout << "\033[1;32mAmount Deposited Successfully!\033[0m\n";
}

void withdrawMoney(const string& accNo) {
    double amount;
    cout << "\033[1;34mEnter Amount to Withdraw: \033[0m";
    cin >> amount;
    
    if (amount <= 0 || amount > MAX_TRANSACTION_AMOUNT) {
        cout << "\033[1;31mInvalid Amount!\033[0m\n";
        return;
    }
    
    string storedData = getFromFirebase("accounts/" + accNo);
    json accountData = json::parse(storedData);
    double currentBalance = accountData["balance"];
    
    if (currentBalance - amount < MIN_BALANCE) {
        cout << "\033[1;31mInsufficient Balance!\033[0m\n";
        return;
    }
    
    double newBalance = currentBalance - amount;
    accountData["balance"] = newBalance;
    
    sendToFirebase("accounts/" + accNo, accountData.dump(), "PUT");
    cout << "\033[1;32mAmount Withdrawn Successfully!\033[0m\n";
}

void checkBalance(const string& accNo) {
    string storedData = getFromFirebase("accounts/" + accNo);
    json accountData = json::parse(storedData);
    double currentBalance = accountData["balance"];
    cout << "\033[1;34mCurrent Balance: \033[0m" << currentBalance << "\n";
}

void deleteAccount(const string& accNo) {
    sendToFirebase("accounts/" + accNo, "", "DELETE");
    cout << "\033[1;32mAccount Deleted Successfully!\033[0m\n";
}

void userMenu(const string& accNo) {
    int choice;
    do {
        cout << "\n\033[1;36m===== User Menu =====\033[0m";
        cout << "\n1. View Account Details";
        cout << "\n2. Deposit Money";
        cout << "\n3. Withdraw Money";
        cout << "\n4. Check Balance";
        cout << "\n5. Change Password";
        cout << "\n6. Delete Account";
        cout << "\n7. Logout";
        cout << "\n\033[1;34mEnter your choice: \033[0m";
        cin >> choice;
        switch (choice) {
            case 1: viewAccountDetails(accNo); break;
            case 2: depositMoney(accNo); break;
            case 3: withdrawMoney(accNo); break;
            case 4: checkBalance(accNo); break;
            case 5: changePassword(accNo); break;
            case 6: deleteAccount(accNo); return; // Logout after deleting account
            case 7: cout << "\033[1;32mLogging out...\033[0m\n"; break;
            default: cout << "\033[1;31mInvalid Choice!\033[0m\n";
        }
    } while (choice != 7);
}

int main() {
    displayIntroAnimation();
    int choice;
    do {
        cout << "\n\033[1;36m===== Cpp Bank Main Menu =====\033[0m";
        cout << "\n1. Create Account";
        cout << "\n2. Login";
        cout << "\n3. Exit";
        cout << "\n\033[1;34mEnter your choice: \033[0m";
        cin >> choice;
        switch (choice) {
            case 1: createAccount(); break;
            case 2: {
                string accNo;
                cout << "\033[1;34mEnter Account Number: \033[0m";
                cin >> accNo;
                if (authenticate(accNo)) {
                    userMenu(accNo);
                }
                break;
            }
            case 3: cout << "\033[1;32mExiting...\033[0m\n"; break;
            default: cout << "\033[1;31mInvalid Choice!\033[0m\n";
        }
    } while (choice != 3);
    return 0;
}