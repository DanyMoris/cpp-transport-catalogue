#pragma once
#include "transport_catalogue.h"
#include "graph.h"
#include "router.h"
#include <string>
#include <memory>
#include <vector>
#include <optional>

namespace transport_router {
    struct RouteItem {
        enum class Type { Wait, Bus } type;
        std::string stop_name;
        std::string bus_name;
        int span_count = 0;
        double time = 0.0;
    };

    struct RouteResult {
        double total_time = 0.0;
        std::vector<RouteItem> items;
    };

    class TransportRouter {
    public:
        TransportRouter(const transport_catalogue::TransportCatalogue& catalogue, int bus_wait_time,
            double bus_velocity_kmph);
        std::optional<RouteResult> BuildRoute(const std::string_view from, const std::string_view to) const;

    private:
        void BuildGraph();

        const transport_catalogue::TransportCatalogue& catalogue_;
        int bus_wait_time_;
        double bus_velocity_kmph_;
        graph::DirectedWeightedGraph<double> graph_;
        std::vector<RouteItem> edge_info_;
        std::unique_ptr<graph::Router<double>> router_;

        std::unordered_map<const domain::Stop*, size_t> stop_to_index_;
    };
}