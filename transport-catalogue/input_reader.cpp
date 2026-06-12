#include "input_reader.h"
#include <string>
#include <string_view>
#include <vector>
#include <iterator>


namespace input_reader {
    namespace {
        std::string_view Trim(std::string_view str) {
            auto start = str.find_first_not_of(" \t\r\n");
            if (start == str.npos) {
                return "";
            }
            auto end = str.find_last_not_of(" \t\r\n");
            return str.substr(start, end - start + 1);
        }

        void ParseAndSetDistances(transport_catalogue::TransportCatalogue& catalogue, std::string_view stop_name, std::string_view description) {
            size_t first_comma = description.find(',');
            if (first_comma == std::string_view::npos) return;

            size_t second_comma = description.find(',', first_comma + 1);
            if (second_comma == std::string_view::npos) return;

            std::string_view distances_str = description.substr(second_comma + 1);

            while (!distances_str.empty()) {
                size_t comma_pos = distances_str.find(',');
                std::string_view chunk = distances_str.substr(0, comma_pos);

                size_t m_pos = chunk.find("m to ");
                if (m_pos != std::string_view::npos) {
                    std::string_view dist_str = Trim(chunk.substr(0, m_pos));
                    std::string_view target_name = Trim(chunk.substr(m_pos + 5));

                    int distance = std::stoi(std::string(dist_str));
                    const auto* from = catalogue.getStop(stop_name);
                    const auto* to = catalogue.getStop(target_name);

                    if (from && to) {
                        catalogue.SetDistance(from, to, distance);
                    }
                }
                if (comma_pos == std::string_view::npos) {
                    break;
                }
                distances_str.remove_prefix(comma_pos + 1);
            }
        }

        std::pair<double, double> ParseCoordinates(const CommandDescription& cmd) {
            size_t first_comma = cmd.description.find(',');
            if (first_comma != std::string::npos) {
                double lat = std::stod(cmd.description.substr(0, first_comma));

                size_t second_comma = cmd.description.find(',', first_comma + 1);
                std::string lon_str = (second_comma != std::string::npos)
                    ? cmd.description.substr(first_comma + 1, second_comma - first_comma - 1)
                    : cmd.description.substr(first_comma + 1);

                double lng = std::stod(lon_str);

                return { lat, lng };
            }
            return {};
        }
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

            auto pair = ParseCoordinates(cmd);
            double lat = pair.first;
            double lng = pair.second;

            catalogue.addStop(cmd.id, { lat, lng });
        }


        for (const auto& cmd : commands_) {
            if (!cmd || cmd.command != "Stop") continue;

            ParseAndSetDistances(catalogue, cmd.id, cmd.description);
        }


        for (const auto& cmd : commands_) {
            if (!cmd || cmd.command != "Bus") continue;

            bool is_circle = (cmd.description.find('>') != std::string::npos);

            std::vector<std::string_view> route_stops = ParseRoute(cmd.description);

            catalogue.addBus(cmd.id, route_stops, is_circle);
        }
    }
}

