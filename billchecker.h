/**
 * \file
 *
 * Declaration for the Electricity BillChecker Class.
 *
 *************************************************************************
 *
 *      Joonas Hämäläinen
 *
 *************************************************************************
 */

#ifndef ELECTRICITY_BILL_CHECKER_H
#define ELECTRICITY_BILL_CHECKER_H

#include <iostream>
#include <string>
#include <map>
#include <tuple>
#include <vector>

namespace electricity
{

using OneDayDataMap = std::map<std::string, float>;
using OneMonthDataMap = std::map<std::string, OneDayDataMap>;
// CSVFields usage
// This tuple is used to parse values from csv files.
// All values represent header names for different datas. You need to specify them
// yourself to fit your needs (depends on your Electicity company's csv format).
//
// Values that are mandatory to have:
// * DateTime, Price for calculating spotprices (You can use empty string to fill the tuple).
// * Hour, Consumption, Avg. Temperature for calculating consumption.
using CSVFields = std::tuple<std::string, std::string, std::string>;

// You can set these to match your contract. (All values are in euros)
#define ELECTRICITY_COMPANY_MARGINAL 0.006f
#define TRANSFER_BASE_COST 0.0409f
#define FUSE_BASE_COST 13.0f
#define MONTHLY_FEE 3.53f

// Don't touch these two they are fixed values for transfer.
#define ENERGY_TAX 0.02778f
#define SECURITY_SUPPLY_COST 0.00016f

// Saves calculated data in this struct.
struct Totals
{
    Totals() = default;
    Totals(float cost, float consumption, std::vector<float> temps);

    float totalConsumption;
    float totalAmount;
    float totalAmountMarginal;
    float totalAmountWithMarginal;
    float totalFinalAmount;
    float avgCostPerkWh;
    float temperatureSum;
    float transferCost;
    float energyTax;
    float securitySupplyCost;
    float avgSpotPrice;
};

struct OperatingContext
{
    std::string spotPrices;
    std::string consumption;
    char spotFileDelimiter;
    char consumptionFileDelimiter;
};

class BillChecker final
{
public:
    BillChecker(const OperatingContext& opCtx);
    ~BillChecker() = default;

    BillChecker(const BillChecker&) = delete;
    BillChecker& operator=(const BillChecker&) = delete;
    BillChecker(BillChecker&&) = delete;
    BillChecker& operator=(BillChecker&&) = delete;

    // Print calculated totals to stdout.
    void printTotals() const;
    // Get Consumption data in json format.
    std::string getJsonConsumption() const;
    // Get SpotPrices data in json format.
    std::string getJsonSpotPrices() const;
    // Get Totals data in json format.
    std::string getJsonTotals() const;

private:
    BillChecker() = default;

    void calculatePower();
    void consumptionDataToMap(
            const std::string& data,
            const char delimiter,
            CSVFields fields);
    void spotPricesToMap(
            const std::string& data,
            const char delimiter,
            CSVFields fields);

private:
    OneMonthDataMap _spotPriceDataMap;
    OneMonthDataMap _consumptionDataMap;
    Totals _totals;
    unsigned _days;
    std::vector<float> _temperatures;
};

} // namespace electricity

#endif // ELECTRICITY_BILL_CHECKER_H