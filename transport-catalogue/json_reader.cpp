#include "json_reader.h"
#include <sstream>

svg::Color JsonReader::ParseColor(const json::Node& node) const {
    if (node.IsString()) {
        return node.AsString();
    }

    if (node.IsArray()) {
        const auto& arr = node.AsArray();

        if (arr.size() == 3) {
            return svg::Rgb{
                static_cast<uint8_t>(arr[0].AsInt()),
                static_cast<uint8_t>(arr[1].AsInt()),
                static_cast<uint8_t>(arr[2].AsInt())
            };
        }

        if (arr.size() == 4) {
            return svg::Rgba{
                static_cast<uint8_t>(arr[0].AsInt()),
                static_cast<uint8_t>(arr[1].AsInt()),
                static_cast<uint8_t>(arr[2].AsInt()),
                arr[3].AsDouble()
            };
        }
    }

    return svg::NoneColor;
}

JsonReader::JsonReader(std::istream& input) : doc_(json::Load(input)) {}

renderer::RenderSettings JsonReader::LoadRenderSettings() const {
    const auto& root = doc_.GetRoot().AsMap();
    const auto& settings_map = root.at("render_settings").AsMap();

    renderer::RenderSettings settings;

    settings.width = settings_map.at("width").AsDouble();
    settings.height = settings_map.at("height").AsDouble();
    settings.padding = settings_map.at("padding").AsDouble();
    settings.stop_radius = settings_map.at("stop_radius").AsDouble();
    settings.line_width = settings_map.at("line_width").AsDouble();


    settings.bus_label_font_size = settings_map.at("bus_label_font_size").AsInt();

    const auto& bus_offset = settings_map.at("bus_label_offset").AsArray();
    settings.bus_label_offset = { bus_offset[0].AsDouble(), bus_offset[1].AsDouble() };

    settings.stop_label_font_size = settings_map.at("stop_label_font_size").AsInt();

    const auto& stop_offset = settings_map.at("stop_label_offset").AsArray();
    settings.stop_label_offset = { stop_offset[0].AsDouble(), stop_offset[1].AsDouble() };

    settings.underlayer_color = ParseColor(settings_map.at("underlayer_color"));
    settings.underlayer_width = settings_map.at("underlayer_width").AsDouble();

    const auto& palette = settings_map.at("color_palette").AsArray();
    for (const auto& color_node : palette) {
        settings.color_palette.push_back(ParseColor(color_node));
    }

    return settings;
}

void JsonReader::ParseStop(const json::Array& base_requests, transport_catalogue::TransportCatalogue& catalogue) const {
    for (const auto& request_node : base_requests) {
        const auto& request_map = request_node.AsMap();
        if (request_map.at("type").AsString() == "Stop") {
            catalogue.addStop(
                request_map.at("name").AsString(),
                { request_map.at("latitude").AsDouble(), request_map.at("longitude").AsDouble() }
            );
        }
    }
}

void JsonReader::ParseRoadDistances(const json::Array& base_requests, transport_catalogue::TransportCatalogue& catalogue) const {
    for (const auto& request_node : base_requests) {
        const auto& request_map = request_node.AsMap();
        if (request_map.at("type").AsString() == "Stop") {
            const std::string& stop_name = request_map.at("name").AsString();

            if (request_map.contains("road_distances")) {
                const auto& road_distances = request_map.at("road_distances").AsMap();
                const auto* stop_from = catalogue.getStop(stop_name);

                for (const auto& [target_name, distance_node] : road_distances) {
                    const auto* stop_to = catalogue.getStop(target_name);
                    if (stop_from && stop_to) {
                        catalogue.SetDistance(stop_from, stop_to, distance_node.AsInt());
                    }
                }
            }
        }
    }
}

void JsonReader::ParseBus(const json::Array& base_requests, transport_catalogue::TransportCatalogue& catalogue) const {
    for (const auto& request_node : base_requests) {
        const auto& request_map = request_node.AsMap();
        if (request_map.at("type").AsString() == "Bus") {
            const std::string& bus_name = request_map.at("name").AsString();
            bool is_roundtrip = request_map.at("is_roundtrip").AsBool();

            std::vector<std::string_view> route;
            for (const auto& stop_node : request_map.at("stops").AsArray()) {
                route.push_back(stop_node.AsString());
            }
            catalogue.addBus(bus_name, route, is_roundtrip);
        }
    }
}


void JsonReader::LoadBaseRequests(transport_catalogue::TransportCatalogue& catalogue) const {
    const auto& root_map = doc_.GetRoot().AsMap();
    if (!root_map.contains("base_requests")) return;

    const auto& base_requests = root_map.at("base_requests").AsArray();

    ParseStop(base_requests, catalogue);
    ParseRoadDistances(base_requests, catalogue);
    ParseBus(base_requests, catalogue);
}

void JsonReader::FillResponseOnBus(json::Dict& response, const json::Dict& map, const RequestHandler& handler) const {
    std::string name = map.at("name").AsString();
    auto stat = handler.GetBusStat(name);
    if (stat.has_value()) {
        response["curvature"] = stat->curvature;
        response["route_length"] = stat->route_length;
        response["stop_count"] = static_cast<int>(stat->total_stops);
        response["unique_stop_count"] = static_cast<int>(stat->unique_stops);
    }
    else {
        response["error_message"] = std::string("not found");
    }
}

void JsonReader::FillResponseOnStop(json::Dict& response, const json::Dict& map, const RequestHandler& handler) const {
    std::string name = map.at("name").AsString();
    if (!handler.IsStopExists(name)) {
        response["error_message"] = std::string("not found");
    }
    else {
        auto buses_ptr = handler.GetBusesByStop(name);
        json::Array buses_list;
        if (buses_ptr) {
            FillBusesList(buses_ptr, buses_list);
        }
        response["buses"] = buses_list;
    }
}

void JsonReader::FillResponseOnMap(json::Dict& response, const RequestHandler& handler, const renderer::MapRenderer& renderer) const {
    const auto& all_buses = handler.GetBusesSortedByName();
    const auto& active_stops = handler.GetActiveStopsSortedByName();

    svg::Document map_svg = renderer.RenderMap(all_buses, active_stops);

    std::ostringstream strm;
    map_svg.Render(strm);

    response["map"] = strm.str();
}

void JsonReader::ProcessStatRequests(const RequestHandler& handler, const renderer::MapRenderer& renderer,
    std::ostream& output) const {
    const auto& root_map = doc_.GetRoot().AsMap();
    if (!root_map.contains("stat_requests")) return;

    const auto& stat_requests = root_map.at("stat_requests").AsArray();
    json::Array json_responses;

    for (const auto& req : stat_requests) {
        const auto& map = req.AsMap();
        int request_id = map.at("id").AsInt();
        std::string type = map.at("type").AsString();

        json::Dict response;
        response["request_id"] = request_id;

        if (type == "Bus") {
            FillResponseOnBus(response, map, handler);
        }
        else if (type == "Stop") {
            FillResponseOnStop(response, map, handler);
        }
        else if (type == "Map") {
            FillResponseOnMap(response, handler, renderer);
        }
        json_responses.push_back(std::move(response));
    }
    json::Print(json::Document{ std::move(json_responses) }, output);
}

void JsonReader::FillBusesList(const std::unordered_set<std::string_view>* buses_ptr, json::Array& array) const {
    if (!buses_ptr) return;
    const auto& buses_set = *buses_ptr;
    std::vector<std::string> sorted_buses(buses_set.begin(), buses_set.end());
    std::sort(sorted_buses.begin(), sorted_buses.end());
    for (const auto& bus : sorted_buses) {
        array.push_back(json::Node(bus));
    }
}