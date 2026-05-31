#include "stat_reader.h"
#include <iomanip>

namespace stat_reader {
    namespace {
        std::string_view Trim(std::string_view str) {
            auto start = str.find_first_not_of(" \t\r\n");
            if (start == str.npos) {
                return "";
            }
            auto end = str.find_last_not_of(" \t\r\n");
            return str.substr(start, end - start + 1);
        }
    }

    void ParseAndPrintStat(const transport_catalogue::TransportCatalogue& transport_catalogue, std::string_view request, std::ostream& output) {
        request = Trim(request);
        size_t space_pos = request.find(' ');

        if (space_pos == std::string_view::npos) {
            return;
        }

        std::string_view command = request.substr(0, space_pos);
        std::string_view name = Trim(request.substr(space_pos + 1));

        if (command == "Bus") {
            const transport_catalogue::Bus* bus = transport_catalogue.getBus(name);

            if (!bus) {
                output << "Bus " << name << ": not found\n";
            }
            else {
                transport_catalogue::BusInfo info = transport_catalogue.getBusInfo(name);

                output << "Bus " << name << ": "
                    << info.total_stops << " stops on route, "
                    << info.unique_stops << " unique stops, "
                    << std::setprecision(6) << info.route_length << " route length\n";
            }
        }
        else if (command == "Stop") {

            const transport_catalogue::Stop* stop = transport_catalogue.getStop(name);

            if (!stop) {
                output << "Stop " << name << ": not found\n";
            }
            else {
                auto set = transport_catalogue.getStopInfo(name);
                if (set.empty()) {
                    output << "Stop " << name << ": no buses\n";
                }
                else {
                    output << "Stop " << name << ": buses";
                    for (auto bus_id : set) {
                        output << " " << bus_id;
                    }
                    output << "\n";
                }
            }
        }
    }
}