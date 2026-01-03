#pragma once

#include <mysql_connection.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>

// ==========================================
// FUNCTION DECLARATIONS
// ==========================================

// Main Menu Entry Point
void runInventoryModule(sql::Connection* con);

// Display Function
void readAllInventory(sql::Connection* con);

// Inventory Operations (Flowchart logic)
void addInventory(sql::Connection* con);
void recordInventoryTopUp(sql::Connection* con);
void recordInventoryConsumption(sql::Connection* con);
void searchInventory(sql::Connection* con);

// Helper Functions
bool checkInventoryIDExists(sql::Connection* con, int inventoryID);
