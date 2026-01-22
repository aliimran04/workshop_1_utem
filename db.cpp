#include "db.h"
#include <cppconn/driver.h>
#include <cppconn/prepared_statement.h>
#include <iostream>
#include "utils.h"
#include <fstream>
#include <map>
// Helper to read the config file
std::map<std::string, std::string> loadConfig(const std::string& filename) {
    std::map<std::string, std::string> config;
    std::ifstream file(filename);
    std::string line;
    while (std::getline(file, line)) {
        size_t delimiter = line.find('=');
        if (delimiter != std::string::npos) {
            std::string key = line.substr(0, delimiter);
            std::string value = line.substr(delimiter + 1);
            config[key] = value;
        }
    }
    return config;
}


/*const std::string server = "tcp://139.162.57.13:3306";
const std::string username = "print";
const std::string password = "Bruh0403";

sql::Connection* connectDB() {
    sql::Driver* driver = get_driver_instance();
    sql::Connection* con = nullptr;

    try {
        con = driver->connect(server, username, password);
        con->setSchema("printing_shop_test");
        std::cout << "Database connection successful!\n";
    }
    catch (sql::SQLException& e) {
        std::cout << "Database connection error: " << e.what() << "\n";
        exit(1);
    }
    return con;
}*/
sql::Connection* connectDB() {
    auto config = loadConfig("config.ini"); // Load external file
    sql::Driver* driver = get_driver_instance();
    sql::Connection* con = nullptr;

    try {
        // Use map values instead of hardcoded strings
        con = driver->connect(config["DB_HOST"], config["DB_USER"], config["DB_PASS"]);
        con->setSchema(config["DB_NAME"]);
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
