#include "db.h"
#include <cppconn/driver.h>
#include <cppconn/prepared_statement.h>
#include <iostream>
#include "utils.h"


const std::string server = "tcp://127.0.0.1:3306";
const std::string username = "root";
const std::string password = "Bruh69420";

sql::Connection* connectDB() {
    sql::Driver* driver = get_driver_instance();
    sql::Connection* con = nullptr;

    try {
        con = driver->connect(server, username, password);
        con->setSchema("print");
        std::cout << "Database connection successful!\n";
    }
    catch (sql::SQLException& e) {
        std::cout << "Database connection error: " << e.what() << "\n";
        exit(1);
    }
    return con;
}

/*bool login(sql::Connection* con, std::string& role) {
    std::string username, password;
    std::cout << "=== Login ===\n";
    std::cout << "Username: ";
    std::getline(std::cin, username);
    std::cout << "Password: ";
    std::getline(std::cin, password);*/
bool login(sql::Connection* con, std::string& role) {
    std::string username, password;
    std::cout << "=== Login ===\n";

    std::cout << "Username: ";
    std::getline(std::cin, username);

    // *** MODIFIED LINE HERE ***
    password = getMaskedPassword("Password: ");
    // ************************

    try {
        std::unique_ptr<sql::PreparedStatement> stmt(
            con->prepareStatement("SELECT Role FROM user WHERE FullName = ? AND Password = ?")
        );
        stmt->setString(1, username);
        stmt->setString(2, password);

        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
        if (res->next()) {
            role = res->getString("Role");
            return true;
        }
        std::cout << "Invalid login.\n";
        return false;
    }
    catch (sql::SQLException& e) {
        std::cout << "Login error: " << e.what() << "\n";
        return false;
    }
}
