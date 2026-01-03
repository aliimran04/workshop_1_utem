#include "utils.h"
#include <iostream>
#include <regex>
#include <limits>
#include <conio.h> // Windows specific for _getch()

void clearScreen() {
    system("cls");
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
