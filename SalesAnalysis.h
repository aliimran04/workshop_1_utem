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
void runSalesAnalysisModule(sql::Connection* con);

// Analysis Functions (matching the flowchart)
void calculateOperationCost(sql::Connection* con);
void calculateProfit(sql::Connection* con);
void calculateTotalRevenue(sql::Connection* con);

// Helper function for cost (used by multiple options)
double fetchOperationCost(sql::Connection* con);