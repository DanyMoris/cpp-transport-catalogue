#pragma once
#include "json.h"
#include "transport_catalogue.h"
#include "request_handler.h"
#include "map_renderer.h"
#include <iostream>

class JsonReader {
public:
    explicit JsonReader(std::istream& input);

    void LoadBaseRequests(transport_catalogue::TransportCatalogue& catalogue) const;

    renderer::RenderSettings LoadRenderSettings() const;

    void ProcessStatRequests(const RequestHandler& handler, const renderer::MapRenderer& renderer, std::ostream& output) const;

private:
    json::Document doc_;

    svg::Color ParseColor(const json::Node& node) const;
    void FillBusesList(const std::unordered_set<std::string_view>* buses_ptr, json::Array& array) const;
};