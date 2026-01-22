#include "utils.h"
#include <iostream>
#include <regex>
#include <limits>
#include <iomanip>
#include <cppconn/connection.h>
#include <cppconn/prepared_statement.h> // Define sql::PreparedStatement
#include <cppconn/resultset.h>          // Define sql::ResultSet
#include <conio.h> // Windows specific for _getch()

void clearScreen() {
    system("cls");
}
// In utils.cpp
/*bool searchIDsByCustomerName(sql::Connection* con) {
    if (std::cin.peek() == '\n') std::cin.ignore();

    std::string nameInput;
    std::cout << "\nEnter Customer Name to find IDs: ";
    std::getline(std::cin, nameInput);
    std::string searchPattern = "%" + nameInput + "%";

    try {
        // SQL modified to explicitly include UserID
        std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
            "SELECT u.FullName, u.UserID, p.JobID, pay.TransactionID "
            "FROM user u LEFT JOIN printjob p ON u.UserID = p.UserID " // Changed JOIN to LEFT JOIN
            "LEFT JOIN payment pay ON p.JobID = pay.JobID "
            "WHERE u.FullName LIKE ? ORDER BY p.JobID DESC"
        ));
        pstmt->setString(1, searchPattern);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        bool found = false;
        if (res->next()) {
            found = true;
            std::cout << "\n--- Search Results for: " << nameInput << " ---\n";
            std::cout << "------------------------------------------------------------\n";
            // Header updated for Customer, UID, JID, and TID
            std::cout << std::left << std::setw(20) << "Customer" << " | "
                      << std::setw(8) << "UID" << " | "
                      << std::setw(8) << "JID" << " | "
                      << std::setw(8) << "TID" << std::endl;
            std::cout << "------------------------------------------------------------\n";

            do {
                std::cout << std::left << std::setw(20) << res->getString("FullName").substr(0, 18) << " | "
                          << std::setw(8) << res->getInt("UserID") << " | "
                          << std::setw(8) << res->getInt("JobID") << " | "
                          << std::setw(8) << (res->isNull("TransactionID") ? "---" : std::to_string(res->getInt("TransactionID"))) 
                          << std::endl;
            } while (res->next());
            std::cout << "------------------------------------------------------------\n";
        }

        if (!found) {
            std::cout << "[Error] No user found with name '" << nameInput << "'. Please try again.\n";
        }
        return found; 
    }
    catch (sql::SQLException& e) {
        std::cerr << "Search Error: " << e.what() << std::endl;
        return false;
    }
}*/
// Update this line to accept two parameters
bool searchIDsByCustomerName(sql::Connection* con, std::string nameInput) {
    // You no longer need to ask for input inside the function because 
    // it's now passed from the 'case 3' menu

    std::string searchPattern = "%" + nameInput + "%";

    try {
        // SQL optimized to prevent duplicate rows from your 100,000+ transactions
        std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
            "SELECT u.FullName, u.UserID, u.Email "
            "FROM user u "
            "WHERE u.FullName LIKE ? "
            "GROUP BY u.UserID" // Hides JID/TID noise
        ));
        pstmt->setString(1, searchPattern);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        if (res->next()) {
            std::cout << "\n--- Unique Users Found ---\n";
            std::cout << std::left << std::setw(25) << "Full Name" << " | " << "UserID" << std::endl;
            do {
                std::cout << std::left << std::setw(25) << res->getString("FullName") << " | "
                    << res->getInt("UserID") << std::endl;
            } while (res->next());
            return true;
        }

        // Displays error if the Malaysian name doesn't exist
        std::cout << "[Error] No user found with name '" << nameInput << "'. Please try again.\n";
        return false;
    }
    catch (sql::SQLException& e) {
        return false;
    }
}

int readInt(const char* prompt) {
    int value;
    while (true) {
        std::cout << prompt;
        if (std::cin >> value) return value;
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Invalid number.\n";
    }
}

bool isValidEmail(const std::string& email) {
    if (email.empty()) return true;
    const std::regex re(R"(^[^@\s]+@[^@\s]+\.[^@\s]+$)");
    return std::regex_match(email, re);
}

bool isValidRole(const std::string& role) {
    return role.empty() || role == "Admin" || role == "Staff" || role == "Customer";
}

std::string getMaskedPassword(const char* prompt) {
    std::string password;
    char ch;

    std::cout << prompt;

    while (true) {
        // _getch() reads a single character without echoing it to the console.
        ch = _getch();

        // 1. Check for ENTER key (ASCII 13) to finalize input
        if (ch == 13) {
            std::cout << '\n'; // Move to a new line after ENTER
            break;
        }

        // 2. Check for BACKSPACE key (ASCII 8)
        else if (ch == 8) {
            if (password.length() > 0) {
                // Erase the last character from the string
                password.pop_back();
                // Erase the character and the asterisk from the console:
                // Move cursor back one, print space, move cursor back one again.
                std::cout << "\b \b";
            }
        }

        // 3. Handle Regular Characters
        else if (ch != 0) {
            password += ch;
            std::cout << '*';
        }
    }
    return password;
}

