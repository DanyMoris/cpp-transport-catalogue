#include "transport_catalogue.h"
#include <stdexcept>
#include <unordered_set>

namespace transport_catalogue {
    const std::deque<domain::Bus>& TransportCatalogue::getBuses() const {
        return routes_;
    }

    const std::deque<domain::Stop>& TransportCatalogue::getStops() const {
        return stations_;
    }

    void TransportCatalogue::addStop(std::string_view name, geo::Coordinates coordinates) {

        stations_.push_back({ std::string(name), coordinates });

        std::string_view saved_name = stations_.back().name;
        map_stops_[saved_name] = &stations_.back();
    }

    void TransportCatalogue::addBus(std::string_view id, const std::vector<std::string_view>& route, bool is_circle) {
        domain::Bus bus;
        bus.id = std::string(id);
        bus.is_circle = is_circle;

        for (std::string_view station_name : route) {

            auto it = map_stops_.find(station_name);
            if (it != map_stops_.end()) {
                bus.route.push_back(it->second);
            }
        }

        routes_.push_back(std::move(bus));

        std::string_view saved_name = routes_.back().id;
        map_bus_[saved_name] = &routes_.back();

        for (const domain::Stop* stop_ptr : routes_.back().route) {
            stop_to_buses_[stop_ptr->name].insert(saved_name);
        }
    }

    const domain::Stop* TransportCatalogue::getStop(const std::string_view name) const {
        auto it = map_stops_.find(name);
        if (it != map_stops_.end()) {
            return it->second;
        }
        return nullptr;
    }

    const domain::Bus* TransportCatalogue::getBus(const std::string_view id) const {
        auto it = map_bus_.find(id);
        if (it != map_bus_.end()) {
            return it->second;
        }
        return nullptr;
    }

    std::optional<domain::BusInfo> TransportCatalogue::getBusInfo(const std::string_view id) const {
        const domain::Bus* bus = getBus(id);

        if (!bus) {
            return std::nullopt;
        }

        domain::BusInfo info;

        info.total_stops = bus->is_circle ? bus->route.size() : bus->route.size() * 2 - 1;

        std::unordered_set<const domain::Stop*> unique_stops(bus->route.begin(), bus->route.end());
        info.unique_stops = unique_stops.size();

        double geo_length = 0.0;
        int road_length = 0;


        for (size_t i = 0; i + 1 < bus->route.size(); ++i) {
            const domain::Stop* from = bus->route[i];
            const domain::Stop* to = bus->route[i + 1];

            geo_length += geo::ComputeDistance(from->coordinates, to->coordinates);
            road_length += GetDistance(from, to);
        }


        if (!bus->is_circle) {

            geo_length *= 2;

            for (size_t i = bus->route.size() - 1; i > 0; --i) {
                const domain::Stop* from = bus->route[i];
                const domain::Stop* to = bus->route[i - 1];

                road_length += GetDistance(from, to);
            }
        }

        info.route_length = road_length;
        info.curvature = road_length / geo_length;

        return info;
    }

    const std::unordered_set<std::string_view>* TransportCatalogue::getBusesByStop(std::string_view stop_name) const {
        auto it = stop_to_buses_.find(stop_name);
        if (it == stop_to_buses_.end()) {
            return nullptr;
        }
        return &it->second;
    }

    void TransportCatalogue::SetDistance(const domain::Stop* from, const domain::Stop* to, int distance) {
        if (from && to) {
            distances_[{from, to}] = distance;
        }
    }

    int TransportCatalogue::GetDistance(const domain::Stop* from, const domain::Stop* to) const {

        if (auto it = distances_.find({ from, to }); it != distances_.end()) {
            return it->second;
        }
        if (auto it = distances_.find({ to, from }); it != distances_.end()) {
            return it->second;
        }
        return 0;
    }

}