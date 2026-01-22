#ifndef REPORT_GENERATION_H
#define REPORT_GENERATION_H

#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include "utils.h"

/**
 * Entry point for the Report Generation Module.
 * Provides a menu for various business analytics.
 */
void runReportGeneration(sql::Connection* con);

/**
 * Requirement: Generating Summary Lists.
 * Summarizes Monthly Sales, Inventory Value, and Profit Margin.
 */
void generateFinancialSummary(sql::Connection* con, int year, int month);

/**
 * Requirement: Generating Text-Based Charts.
 * Visualizes monthly sales volume using a bar chart format.
 */
void displaySalesTrendChart(sql::Connection* con, int year);  // Requirement 4

/**
 * Requirement: Generating Text-Based Graph Summaries.
 * Shows percentage changes in sales from month to month.
 */
void displaySalesGrowthGraph(sql::Connection* con, int year); // Requirement 5

/**
 * Requirement: Generating Reports in Table Format.
 * Displays detailed monthly transaction data including customer names.
 */
void displayMonthlySalesTable(sql::Connection* con, int year, int month);

#endif