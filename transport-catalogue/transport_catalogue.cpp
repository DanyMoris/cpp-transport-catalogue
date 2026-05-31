#include "transport_catalogue.h"
#include <stdexcept>
#include <unordered_set>

namespace transport_catalogue {
    void TransportCatalogue::addStop(std::string_view name, Coordinates coordinates) {
        stations_.push_back({ std::string(name), coordinates });

        std::string_view saved_name = stations_.back().name;
        map_stops[saved_name] = &stations_.back();
    }

    void TransportCatalogue::addBus(std::string_view id, const std::vector<std::string_view>& route, bool is_circle) {
        Bus bus;
        bus.id = std::string(id);
        bus.is_circle = is_circle;

        for (std::string_view station_name : route) {

            auto it = map_stops.find(station_name);
            if (it != map_stops.end()) {
                bus.route.push_back(it->second);
            }
        }

        routes_.push_back(std::move(bus));

        std::string_view saved_name = routes_.back().id;
        map_bus[saved_name] = &routes_.back();

        for (const Stop* stop_ptr : routes_.back().route) {
            stop_to_buses_[stop_ptr->name].insert(saved_name);
        }
    }

    const Stop* TransportCatalogue::getStop(const std::string_view name) const {
        auto it = map_stops.find(name);
        if (it != map_stops.end()) {
            return it->second;
        }
        return nullptr;
    }

    const Bus* TransportCatalogue::getBus(const std::string_view id) const {
        auto it = map_bus.find(id);
        if (it != map_bus.end()) {
            return it->second;
        }
        return nullptr;
    }

    BusInfo TransportCatalogue::getBusInfo(const std::string_view id) const {
        const Bus* bus = getBus(id);

        if (!bus) {
            throw std::invalid_argument("Bus not found");
        }

        BusInfo info;

        info.total_stops = bus->route.size();

        std::unordered_set<const Stop*> unique_stops(bus->route.begin(), bus->route.end());
        info.unique_stops = unique_stops.size();

        for (size_t i = 0; i + 1 < bus->route.size(); ++i) {
            info.route_length += ComputeDistance(bus->route[i]->coordinates, bus->route[i + 1]->coordinates);
        }

        return info;
    }

    std::set<std::string_view> TransportCatalogue::getStopInfo(std::string_view name) const {
        const Stop* stop = getStop(name);
        if (!stop) {
            throw std::invalid_argument("Stop not found");
        }

        auto it = stop_to_buses_.find(stop->name);
        if (it != stop_to_buses_.end()) {
            return it->second;
        }

        return {};
    }
}