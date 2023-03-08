/**
 * \file
 *
 * Implementation for the Electricity BillChecker Class.
 *
 *************************************************************************
 *
 *      Joonas Hämäläinen
 *
 *************************************************************************
 */
#include "billchecker.h"
#include "csv.hpp"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <sstream>
#include <iomanip>
#include <numeric>

namespace electricity
{

template <typename T>
std::string to_string_with_precision(const T a_value, const int n = 3)
{
    std::ostringstream out;
    out.precision(n);
    out << std::fixed << a_value;
    return out.str();
}

static void replaceComma(std::string& str)
{
    auto it = str.find(",");
    if (it != std::string::npos)
        str.replace(it, 1, ".");
}

static std::string addLeadingZero(const std::string& str)
{
    std::string ret(str);
    if (str.size() < 2)
        ret = std::string("0").append(str);
    return ret;
}

// Convert datestring from DD.MM.YYYY to YYYY-MM-DD
// 3.10.2022 -> 2022-11-03
static std::string convertDateStr(const std::string& str)
{
    std::istringstream ss(str);
    std::vector<std::string> tokens;
    std::string token;

    while (std::getline(ss, token, '.')) { tokens.push_back(token); }

    // YYYY-
    token = tokens.back() + "-";
    tokens.pop_back();
    // YYYY-MM-
    token += addLeadingZero(tokens.back()) + "-";
    tokens.pop_back();
    // YYYY-MM-DD
    token += addLeadingZero(tokens.back());
    return token;
}

// Convert hour from (0:00 or 00:00:00) to 00:00
static std::string convertHourStr(const std::string& str)
{
    std::istringstream ss(str);
    std::vector<std::string> tokens;
    std::string token;

    while (std::getline(ss, token, ':')) { tokens.push_back(token); }

    // 00:00
    token = addLeadingZero(tokens.front()) + ":00";
    return token;
}

static std::string getJson(const OneMonthDataMap& map)
{
    using namespace rapidjson;
    StringBuffer s;
    Writer<StringBuffer> writer(s);
    writer.StartObject();
    for (const auto& [ date, map] : map)
    {
        writer.Key(date.c_str());
        writer.StartObject();
        for (const auto& [ hour, data ] : map)
        {
            writer.Key(hour.c_str());
            writer.String(to_string_with_precision(data).c_str());
        }
        writer.EndObject();
    }
    writer.EndObject();

    return s.GetString();
}

static std::string getTotalsJson(const Totals& totals, unsigned days)
{
    using namespace rapidjson;
    StringBuffer s;
    Writer<StringBuffer> writer(s);

    writer.StartObject();
    writer.Key("consumption");
    writer.String(to_string_with_precision(totals.totalConsumption).c_str());
    writer.Key("marginal");
    writer.String(to_string_with_precision(totals.totalAmountMarginal).c_str());
    writer.Key("cost_wo_marginal");
    writer.String(to_string_with_precision(totals.totalAmount).c_str());
    writer.Key("cost_with_marginal");
    writer.String(to_string_with_precision(totals.totalAmountWithMarginal).c_str());
    writer.Key("total_cost");
    writer.String(to_string_with_precision(totals.totalFinalAmount).c_str());
    writer.Key("avg_kwh_cost");
    writer.String(to_string_with_precision(totals.avgCostPerkWh).c_str());
    writer.Key("avg_spotprice_per_kwh");
    writer.String(to_string_with_precision(totals.avgSpotPrice).c_str());
    writer.Key("days");
    writer.String(std::to_string(days).c_str());
    writer.Key("avg_temperature");
    writer.String(to_string_with_precision(totals.temperatureSum / days).c_str());
    writer.Key("transfer_cost");
    writer.String(to_string_with_precision(totals.transferCost).c_str());
    writer.Key("energy_tax");
    writer.String(to_string_with_precision(totals.energyTax).c_str());
    writer.Key("security_supply_cost");
    writer.String(to_string_with_precision(totals.securitySupplyCost).c_str());
    writer.Key("total_transfer_cost");
    writer.String(to_string_with_precision(
            totals.transferCost + totals.energyTax +
                totals.securitySupplyCost).c_str());
    writer.Key("total_cost_with_transfer");
    writer.String(to_string_with_precision(
            totals.totalFinalAmount + totals.transferCost + totals.energyTax +
                totals.securitySupplyCost).c_str());
    writer.EndObject();

    return s.GetString();
}

Totals::Totals(float cost, float consumption, std::vector<float> temps) :
    totalConsumption(consumption),
    totalAmount(cost / 100),
    totalAmountMarginal(consumption * ELECTRICITY_COMPANY_MARGINAL),
    totalAmountWithMarginal(totalAmount + totalAmountMarginal),
    totalFinalAmount(totalAmount + totalAmountMarginal + MONTHLY_FEE),
    avgCostPerkWh((cost + totalAmountMarginal*100) / totalConsumption),
    temperatureSum(std::accumulate(temps.begin(), temps.end(), 0.0)),
    transferCost(consumption * TRANSFER_BASE_COST),
    energyTax(consumption * ENERGY_TAX),
    securitySupplyCost(consumption * SECURITY_SUPPLY_COST),
    avgSpotPrice(0)
{}

BillChecker::BillChecker(const OperatingContext& opCtx) :
    _days(0),
    _temperatures(0)
{
    spotPricesToMap(
            opCtx.spotPrices,
            opCtx.spotFileDelimiter,
            CSVFields{"DateTime", "Hinta", ""});
    consumptionDataToMap(
            opCtx.consumption,
            opCtx.consumptionFileDelimiter,
            CSVFields{"Alkaa", "Kulutus (kWh)", "Keskilämpötila"});
    calculatePower();
}

void BillChecker::spotPricesToMap(
        const std::string& data,
        const char delimiter,
        CSVFields fields)
{
    OneDayDataMap dayMap;
    csv::CSVFormat format;
    format.delimiter(delimiter);
    csv::CSVReader spotPricesReader(data, format);
    std::string dayTracker;

    for (const auto& row : spotPricesReader)
    {
        std::string readHour, readDate;
        std::istringstream ss(row[std::get<0>(fields)].get());
        ss >> readDate >> readHour;
        readHour = convertHourStr(readHour);

        if (dayTracker != readDate && !dayTracker.empty())
        {
            _spotPriceDataMap[dayTracker] = dayMap;
            dayMap.clear();
        }
        dayTracker = readDate;
        try
        {
            dayMap[readHour] = row[std::get<1>(fields)].get<float>();
        }
        catch (const std::exception& e)
        {
            std::cout << e.what() << std::endl;
            return;
        };
    }
    _spotPriceDataMap[dayTracker] = dayMap;
}

void BillChecker::consumptionDataToMap(
        const std::string& data,
        const char delimiter,
        CSVFields fields)
{
    bool zeroConsData(false);
    OneDayDataMap dayMap;
    std::vector<float> dayTemperatures;
    csv::CSVFormat format;
    format.delimiter(delimiter);
    csv::CSVReader spotPricesReader(data, format);
    std::string dayTracker;
    std::string prevHour;

    for (const auto& row : spotPricesReader)
    {
        std::string readHour, readDate;
        std::istringstream ss(row[std::get<0>(fields)].get());
        ss >> readDate >> readHour;
        // Need to convert to YYYY-MM-DD
        // 3.10.2022 -> 2022-11-03
        readDate = convertDateStr(readDate);
        readHour = convertHourStr(readHour);

        if (dayTracker != readDate && !dayTracker.empty())
        {
            _days++;
            _consumptionDataMap[dayTracker] = dayMap;
            _temperatures.push_back(
                    std::accumulate(
                        dayTemperatures.begin(),
                        dayTemperatures.end(),
                        0.0)
                    / 24);
            dayMap.clear();
            dayTemperatures.clear();
        }
        dayTracker = readDate;
        try
        {
            std::string consumptionStr = row[std::get<1>(fields)].get();
            replaceComma(consumptionStr);
            std::string temperatureStr = row[std::get<2>(fields)].get();
            replaceComma(temperatureStr);
            if (consumptionStr == "0.00")
            {
                // We don't have consumption data anymore we can just return.
                zeroConsData = true;
                return;
            }
            // Winter/Summer time hack
            if (readHour != prevHour)
            {
                dayMap[readHour] = std::stof(consumptionStr);
            }
            else
            {
                dayMap[readHour] += std::stof(consumptionStr);
            }
            dayTemperatures.push_back(std::stof(temperatureStr));
            prevHour = readHour;
        }
        catch (const std::exception& e)
        {
            std::cout << e.what() << std::endl;
            return;
        };
    }

    if (!zeroConsData)
        _days++;
    _consumptionDataMap[dayTracker] = dayMap;
}

void BillChecker::calculatePower()
{
    float totalCost(0);
    float totalConsumption(0);
    float spotPriceSum(0);

    for (const auto& [ conDate, map ] : _consumptionDataMap)
    {
        const auto& spotMap = _spotPriceDataMap[conDate];
        for (const auto& [ hour, price ] : spotMap)
        {
            try
            {
                totalCost += price * map.at(hour);
                totalConsumption += map.at(hour);
            }
            catch(const std::exception& e)
            {
#ifdef DEBUG_PRINTING
                std::cerr << "No consumption data for - price: " << std::to_string(price) << " hour: "
                    << hour << e.what() << '\n';
#endif
            }
        }
        spotPriceSum += std::accumulate(spotMap.begin(), spotMap.end(), 0,
            [](float value, const OneDayDataMap::value_type &entry){ return value + entry.second;}) / 24.0f;
    }
    _totals = Totals(totalCost, totalConsumption, _temperatures);
    _totals.avgSpotPrice = spotPriceSum / (float)_days;
}

void BillChecker::printTotals() const
{
    std::cout << "Total consumption: " <<
        to_string_with_precision(_totals.totalConsumption) << " kWh"<< std::endl;
    std::cout << "Total marginal amount: " <<
        to_string_with_precision(_totals.totalAmountMarginal) << " €" << std::endl;
    std::cout << "Total cost of bill w/o marginal: " <<
        to_string_with_precision(_totals.totalAmount) << " €" << std::endl;
    std::cout << "Total cost of bill with marginal: " <<
        to_string_with_precision(_totals.totalAmountWithMarginal) << " €" << std::endl;
    std::cout << "Total cost of bill: " <<
        to_string_with_precision(_totals.totalFinalAmount) << " €" << std::endl;
    std::cout << "Average cost per kWh: " <<
        to_string_with_precision(_totals.avgCostPerkWh) << " cnt" << std::endl;
    std::cout << "Average SpotPrice for " << _days << " days: " <<
        to_string_with_precision(_totals.avgSpotPrice) << " cnt" << std::endl;
    std::cout << "Average temperature: " <<
        to_string_with_precision(_totals.temperatureSum / _days) << " C" << std::endl;
    std::cout << "Transfer costs: " <<
        to_string_with_precision(_totals.transferCost + _totals.energyTax +
                _totals.securitySupplyCost) << " cnt" << std::endl;
    std::cout << "Transfer and energy total: " <<
        to_string_with_precision(
            _totals.totalFinalAmount + _totals.transferCost +
            _totals.energyTax + _totals.securitySupplyCost) << " €" << std::endl;
}

std::string BillChecker::getJsonConsumption() const
{
    return getJson(_consumptionDataMap);
}

std::string BillChecker::getJsonSpotPrices() const
{
    return getJson(_spotPriceDataMap);
}

std::string BillChecker::getJsonTotals() const
{
    return getTotalsJson(_totals, _days);
}

}