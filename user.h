#pragma once
#include <string>
#include <mysql_connection.h>

void createUser(sql::Connection* con, const std::string&, const std::string&, const std::string&, const std::string&);
void readUsers(sql::Connection* con);
void updateUser(sql::Connection* con, int userID, const std::string&, const std::string&, const std::string&, const std::string&);
void deleteUser(sql::Connection* con, int userID);
void searchUser(sql::Connection* con, const std::string&);

