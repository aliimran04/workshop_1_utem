
#pragma once
#include <string>
#include <mysql_connection.h>

sql::Connection* connectDB();
bool login(sql::Connection* con, std::string& role);
