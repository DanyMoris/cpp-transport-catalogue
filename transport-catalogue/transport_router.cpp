#include "transport_router.h"
#include <stdexcept>
#include "graph.h"
#include "transport_catalogue.h"
using namespace transport_catalogue;
using namespace domain;
using namespace graph;

namespace transport_router {
    TransportRouter::TransportRouter(const TransportCatalogue& catalogue, int bus_wait_time, double bus_velocity_kmph)
        : catalogue_(catalogue), bus_wait_time_(bus_wait_time), bus_velocity_kmph_(bus_velocity_kmph) {
        const auto& stops = catalogue_.getStops();
        for (size_t i = 0; i < stops.size(); i++) {
            stop_to_index_[&stops[i]] = i;
        }
        const size_t vertex_count = stops.size() * 2;
        graph_ = DirectedWeightedGraph<double>(vertex_count);
        BuildGraph();

        router_ = std::make_unique<Router<double>>(graph_);
    }



    // BuildGraph: add wait edges and bus edges for all spans (i -> j, j>i)
    void TransportRouter::BuildGraph() {
        const auto& stops = catalogue_.getStops();
        const size_t n = stops.size();


        for (size_t i = 0; i < n; ++i) {
            VertexId wait_v = i * 2;
            VertexId board_v = i * 2 + 1;
            graph::Edge<double> e{ wait_v, board_v, static_cast<double>(bus_wait_time_) };
            graph_.AddEdge(e);
            RouteItem info;
            info.type = RouteItem::Type::Wait;
            info.stop_name = stops[i].name;
            info.time = static_cast<double>(bus_wait_time_);
            edge_info_.push_back(std::move(info));
        }


        for (const auto& bus : catalogue_.getBuses()) {

            std::vector<const domain::Stop*> seq = bus.route;
            if (!bus.is_circle) {
                for (size_t i = bus.route.size() - 1; i > 0; --i) {
                    seq.push_back(bus.route[i - 1]);
                }
            }

            // Add edges for every span (i -> j), accumulating distance/time, and set span_count = j - i
            for (size_t i = 0; i < seq.size(); ++i) {
                int dist_accum = 0;
                for (size_t j = i + 1; j < seq.size(); ++j) {
                    const domain::Stop* a = seq[j - 1];
                    const domain::Stop* b = seq[j];
                    dist_accum += catalogue_.GetDistance(a, b);

                    double minutes = (static_cast<double>(dist_accum) / 1000.0) / bus_velocity_kmph_ * 60.0;

                    VertexId from_v = stop_to_index_.at(seq[i]) * 2 + 1; // board at seq[i]
                    VertexId to_v = stop_to_index_.at(seq[j]) * 2;       // disembark (wait) at seq[j]

                    graph::Edge<double> e{ from_v, to_v, minutes };
                    graph_.AddEdge(e);

                    RouteItem info;
                    info.type = RouteItem::Type::Bus;
                    info.bus_name = bus.id;
                    info.span_count = static_cast<int>(j - i);
                    info.time = minutes;
                    edge_info_.push_back(std::move(info));
                }
            }
        }
    }

    std::optional<RouteResult> TransportRouter::BuildRoute(const std::string_view from,
        const std::string_view to) const {
        const domain::Stop* s_from = catalogue_.getStop(from);
        const domain::Stop* s_to = catalogue_.getStop(to);
        if (!s_from || !s_to) return std::nullopt;

        VertexId from_v = stop_to_index_.at(s_from) * 2;
        VertexId to_v = stop_to_index_.at(s_to) * 2;

        auto opt = router_->BuildRoute(from_v, to_v);
        if (!opt) return std::nullopt;

        RouteResult result;
        result.total_time = opt->weight;

        for (EdgeId id : opt->edges) {
            const RouteItem& info = edge_info_.at(id);
            if (info.type == RouteItem::Type::Wait) {
                result.items.push_back(info);
            }
            else {
                if (!result.items.empty()) {
                    RouteItem& last = result.items.back();
                    if (last.type == RouteItem::Type::Bus && last.bus_name == info.bus_name) {
                        last.span_count += info.span_count;
                        last.time += info.time;
                        continue;
                    }
                }
                result.items.push_back(info);
            }
        }
        return result;
    }
}