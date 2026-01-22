#pragma once
#include <string>
#include <mysql_connection.h>

void createUser(sql::Connection* con, const std::string&, const std::string&, const std::string&, const std::string&);
void readUsers(sql::Connection* con);
//void updateUser(sql::Connection* con, int userID, const std::string&, const std::string&, const std::string&, const std::string&);
void deleteUser(sql::Connection* con, int userID);
void searchUser(sql::Connection* con, const std::string&);
void updateUser(sql::Connection* con, int userID, const std::string& newName, const std::string& newRole, const std::string& newEmail, const std::string& newPwd);
void viewUserDetails(sql::Connection* con, int userID);
