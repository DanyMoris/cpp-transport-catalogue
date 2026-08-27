#include "json_reader.h"
#include "json_builder.h"
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
    const auto& root = doc_.GetRoot().AsDict();
    const auto& settings_map = root.at("render_settings").AsDict();

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
        const auto& request_map = request_node.AsDict();
        if (request_map.at("type").AsString() == "Stop") {
            catalogue.addStop(
                request_map.at("name").AsString(),
                { request_map.at("latitude").AsDouble(), request_map.at("longitude").AsDouble() }
            );
        }
    }
}

void JsonReader::LoadTransportData(const RequestHandler& handler) const {
    const auto& root = doc_.GetRoot().AsDict();
    if (!root.contains("routing_settings")) return;

    const auto& routing_settings_map = root.at("routing_settings").AsDict();
    handler.SetRouter(routing_settings_map.at("bus_wait_time").AsInt(), routing_settings_map.at("bus_velocity").AsDouble());
}

void JsonReader::ParseRoadDistances(const json::Array& base_requests, transport_catalogue::TransportCatalogue& catalogue) const {
    for (const auto& request_node : base_requests) {
        const auto& request_map = request_node.AsDict();
        if (request_map.at("type").AsString() == "Stop") {
            const std::string& stop_name = request_map.at("name").AsString();

            if (request_map.contains("road_distances")) {
                const auto& road_distances = request_map.at("road_distances").AsDict();
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
        const auto& request_map = request_node.AsDict();
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
    const auto& root_map = doc_.GetRoot().AsDict();
    if (!root_map.contains("base_requests")) return;

    const auto& base_requests = root_map.at("base_requests").AsArray();

    ParseStop(base_requests, catalogue);
    ParseRoadDistances(base_requests, catalogue);
    ParseBus(base_requests, catalogue);
}


json::Node JsonReader::MakeResponseOnBus(int request_id, const json::Dict& map, const RequestHandler& handler) const {
    std::string name = map.at("name").AsString();
    auto stat = handler.GetBusStat(name);

    if (stat.has_value()) {
        return json::Builder{}
            .StartDict()
            .Key("curvature").Value(stat->curvature)
            .Key("request_id").Value(request_id)
            .Key("route_length").Value(stat->route_length)
            .Key("stop_count").Value(static_cast<int>(stat->total_stops))
            .Key("unique_stop_count").Value(static_cast<int>(stat->unique_stops))
            .EndDict()
            .Build();
    }
    else {
        return json::Builder{}
            .StartDict()
            .Key("request_id").Value(request_id)
            .Key("error_message").Value(std::string("not found"))
            .EndDict()
            .Build();
    }
}

json::Node JsonReader::MakeResponseOnStop(int request_id, const json::Dict& map, const RequestHandler& handler) const {
    std::string name = map.at("name").AsString();
    if (!handler.IsStopExists(name)) {
        return json::Builder{}
            .StartDict()
            .Key("request_id").Value(request_id)
            .Key("error_message").Value(std::string("not found"))
            .EndDict()
            .Build();
    }
    else {
        json::Builder builder;
        auto array_ctx = builder.StartDict()
            .Key("request_id").Value(request_id)
            .Key("buses").StartArray();

        auto buses_ptr = handler.GetBusesByStop(name);
        if (buses_ptr) {
            std::vector<std::string> sorted_buses(buses_ptr->begin(), buses_ptr->end());
            std::sort(sorted_buses.begin(), sorted_buses.end());
            for (const auto& bus : sorted_buses) {
                array_ctx.Value(bus);
            }
        }

        return array_ctx.EndArray().EndDict().Build();
    }
}

json::Node JsonReader::MakeResponseOnMap(int request_id, const RequestHandler& handler,
    const renderer::MapRenderer& renderer) const {
    const auto& all_buses = handler.GetBusesSortedByName();
    const auto& active_stops = handler.GetActiveStopsSortedByName();

    svg::Document map_svg = renderer.RenderMap(all_buses, active_stops);
    std::ostringstream strm;
    map_svg.Render(strm);

    return json::Builder{}
        .StartDict()
        .Key("map").Value(strm.str())
        .Key("request_id").Value(request_id)
        .EndDict()
        .Build();
}

json::Node JsonReader::MakeResponseOnRouting(int request_id, const json::Dict& map, const RequestHandler& handler) const {
    const auto& from = map.at("from").AsString();
    const auto& to = map.at("to").AsString();

    if (!handler.IsStopExists(from) || !handler.IsStopExists(to)) {
        return json::Builder{}
            .StartDict()
            .Key("request_id").Value(request_id)
            .Key("error_message").Value(std::string("not found"))
            .EndDict()
            .Build();
    }

    auto opt_route = handler.FindRoute(from, to);
    if (!opt_route) {
        return json::Builder{}
            .StartDict()
            .Key("request_id").Value(request_id)
            .Key("error_message").Value(std::string("not found"))
            .EndDict()
            .Build();
    }

    const transport_router::RouteResult& route = *opt_route;

    json::Builder builder;
    auto array_ctx = builder.StartDict()
        .Key("request_id").Value(json::Node(request_id))
        .Key("total_time").Value(json::Node(route.total_time))
        .Key("items").StartArray();

    for (const auto& item : route.items) {
        if (item.type == transport_router::RouteItem::Type::Wait) {
            json::Builder item_builder;
            array_ctx.Value(item_builder.StartDict()
                .Key("type").Value("Wait")
                .Key("stop_name").Value(item.stop_name)
                .Key("time").Value(item.time)
                .EndDict().Build());
        }
        else {
            json::Builder item_builder;
            array_ctx.Value(item_builder.StartDict()
                .Key("type").Value("Bus")
                .Key("bus").Value(item.bus_name)
                .Key("span_count").Value(item.span_count)
                .Key("time").Value(item.time)
                .EndDict().Build());
        }
    }
    return array_ctx.EndArray().EndDict().Build();
}

void JsonReader::ProcessStatRequests(const RequestHandler& handler, const renderer::MapRenderer& renderer,
    std::ostream& output) const {
    const auto& root_map = doc_.GetRoot().AsDict();
    if (!root_map.contains("stat_requests")) return;

    const auto& stat_requests = root_map.at("stat_requests").AsArray();

    json::Builder builder;
    auto array_ctx = builder.StartArray();

    for (const auto& req : stat_requests) {
        const auto& map = req.AsDict();
        int request_id = map.at("id").AsInt();
        std::string type = map.at("type").AsString();

        if (type == "Bus") {
            array_ctx.Value(MakeResponseOnBus(request_id, map, handler));
        }
        else if (type == "Stop") {
            array_ctx.Value(MakeResponseOnStop(request_id, map, handler));
        }
        else if (type == "Map") {
            array_ctx.Value(MakeResponseOnMap(request_id, handler, renderer));
        }
        else if (type == "Route") {
            array_ctx.Value(MakeResponseOnRouting(request_id, map, handler));
        }
    }
    json::Print(json::Document{ array_ctx.EndArray().Build() }, output);
}
