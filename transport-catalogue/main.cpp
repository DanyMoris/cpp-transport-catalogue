
#include "json_reader.h"
#include "request_handler.h"
#include "map_renderer.h"
#include <iostream>

int main() {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);

    transport_catalogue::TransportCatalogue catalogue;

    JsonReader reader(std::cin);

    reader.LoadBaseRequests(catalogue);
    RequestHandler handler(catalogue);
    reader.LoadTransportData(handler);

    renderer::MapRenderer renderer(reader.LoadRenderSettings());

    reader.ProcessStatRequests(handler, renderer, std::cout);

    return 0;
}