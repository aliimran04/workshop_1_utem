
#pragma once
#include <string>
#include <cppconn/connection.h>

int readInt(const char* prompt);
void clearScreen();
bool isValidEmail(const std::string& email);
bool isValidRole(const std::string& role);
//bool searchIDsByCustomerName(sql::Connection* con);
bool searchIDsByCustomerName(sql::Connection* con, std::string nameInput);
// Function for masked password input
std::string getMaskedPassword(const char* prompt);