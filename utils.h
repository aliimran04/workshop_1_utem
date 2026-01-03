#pragma once
#include <string>

int readInt(const char* prompt);
void clearScreen();
bool isValidEmail(const std::string& email);
bool isValidRole(const std::string& role);
// Function for masked password input
std::string getMaskedPassword(const char* prompt);