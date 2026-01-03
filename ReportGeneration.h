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
void runReportGenerationModule(sql::Connection* con);

// Report Functions (matching the flowchart)
void generateMonthlyRevenue(sql::Connection* con);
void generateTopCustomers(sql::Connection* con);

// Helper function
bool isValidMonth(int month);