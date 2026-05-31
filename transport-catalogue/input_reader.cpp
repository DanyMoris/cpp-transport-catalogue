#include "input_reader.h"
#include <string>
#include <string_view>
#include <vector>
#include <iterator>


namespace input_reader {
    std::string_view Trim(std::string_view str) {
        auto start = str.find_first_not_of(" \t\r\n");
        if (start == str.npos) {
            return "";
        }
        auto end = str.find_last_not_of(" \t\r\n");
        return str.substr(start, end - start + 1);
    }


    std::vector<std::string_view> Split(std::string_view str, char delimiter) {
        std::vector<std::string_view> result;
        size_t pos = 0;
        while ((pos = str.find(delimiter)) != str.npos) {

            result.push_back(Trim(str.substr(0, pos)));
            str.remove_prefix(pos + 1);
        }

        result.push_back(Trim(str));
        return result;
    }

    std::vector<std::string_view> ParseRoute(std::string_view route) {
        if (route.find('>') != route.npos) {
            return Split(route, '>');
        }

        auto stops = Split(route, '-');
        std::vector<std::string_view> results(stops.begin(), stops.end());

        results.insert(results.end(), std::next(stops.rbegin()), stops.rend());

        return results;
    }


    void InputReader::ParseLine(std::string_view line) {
        CommandDescription cmd;

        size_t space_pos = line.find(' ');
        if (space_pos == line.npos) {
            return;
        }

        cmd.command = std::string(Trim(line.substr(0, space_pos)));

        size_t colon_pos = line.find(':');
        if (colon_pos == line.npos) {
            return;
        }

        size_t id_start = space_pos + 1;
        size_t id_length = colon_pos - id_start;


        cmd.id = std::string(Trim(line.substr(id_start, id_length)));


        cmd.description = std::string(Trim(line.substr(colon_pos + 1)));

        commands_.push_back(std::move(cmd));
    }

    void InputReader::ApplyCommands(transport_catalogue::TransportCatalogue& catalogue) const {

        for (const auto& cmd : commands_) {
            if (!cmd || cmd.command != "Stop") {
                continue;
            }

            size_t comma_pos = cmd.description.find(',');
            if (comma_pos != std::string::npos) {

                double lat = std::stod(cmd.description.substr(0, comma_pos));
                double lng = std::stod(cmd.description.substr(comma_pos + 1));


                catalogue.addStop(cmd.id, { lat, lng });
            }
        }


        for (const auto& cmd : commands_) {
            if (!cmd || cmd.command != "Bus") {
                continue;
            }

            bool is_circle = (cmd.description.find('>') != std::string::npos);

            std::vector<std::string_view> route_stops = ParseRoute(cmd.description);

            catalogue.addBus(cmd.id, route_stops, is_circle);
        }
    }
}