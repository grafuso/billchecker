#include "billchecker.h"
#include <boost/program_options.hpp>
#include <string>

int main(int argc, char *argv[])
{
    std::string choice, spotFile, consFile;
    char sfDelimiter, cfDelimiter;

    namespace po = boost::program_options;
    po::options_description description("Usage:");
    description.add_options()
        ( "help,h",
                    "Display this help message")
        ( "json,j", po::value<std::string>(&choice)->
                    default_value(""),
                    "You can choose what data to print out as a json."
                    " Options: "
                    "'consumption', 'spot', 'totals'")
        ("spotfile", po::value<std::string>(&spotFile)->
                    default_value(""),
                    "File for spot prices data in csv format.")
        ("sf-delimiter", po::value<char>(&sfDelimiter)->
                    default_value(','),
                    "SpotPrice file delimiter for parsing csv data.")
        ("consfile", po::value<std::string>(&consFile)->
                    default_value(""),
                    "File for consumption data in csv format.")
        ("cf-delimiter", po::value<char>(&cfDelimiter)->
                    default_value(';'),
                    "Consumption file delimiter for parsing csv data.");

    po::variables_map vm;
    try
    {
        po::parsed_options parsed =
            po::command_line_parser(argc, argv).options(description).run();

        po::store(parsed, vm);
        po::notify(vm);
    }
    catch (std::exception& e)
    {
        std::cerr << "Command line error: " << e.what() << std::endl;
        return false;
    }

    if (vm.count("help"))
    {
        std::cout << description << std::endl;
        return 0;
    }

    if (spotFile.empty() || consFile.empty())
    {
        std::cerr << "Mandatory spotfile or consumption file missing."
                << std::endl;
        return false;
    }

    electricity::OperatingContext ctx;
    ctx.spotPrices = spotFile;
    ctx.consumption = consFile;
    ctx.spotFileDelimiter = sfDelimiter;
    ctx.consumptionFileDelimiter = cfDelimiter;

    // Start program
    electricity::BillChecker bill(ctx);

    if (choice == "consumption")
    {
        std::cout << bill.getJsonConsumption();
    }
    else if (choice == "spot")
    {
        std::cout << bill.getJsonSpotPrices();
    }
    else if(choice == "totals")
    {
        std::cout << bill.getJsonTotals();
    }
    else
    {
        bill.printTotals();
    }

    return 0;
}