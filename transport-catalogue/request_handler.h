#pragma once
#include "transport_catalogue.h"
#include "map_renderer.h"
#include <optional>
#include <unordered_set>
#include <algorithm>

class RequestHandler {
public:
    explicit RequestHandler(const transport_catalogue::TransportCatalogue& db) : db_(db) {}

    std::optional<domain::BusInfo> GetBusStat(const std::string_view& bus_name) const {
        return db_.getBusInfo(bus_name);
    }

    const std::unordered_set<std::string_view>* GetBusesByStop(const std::string_view& stop_name) const {
        return db_.getStopInfo(stop_name);

    }

    const std::vector<const domain::Bus*> GetBusesSortedByName() const {
        auto buses = db_.getBuses();
        std::sort(buses.begin(), buses.end(), [](const domain::Bus* lhs, const domain::Bus* rhs) {
            return lhs->id < rhs->id;
            });

        return buses;
    }

    const std::vector<const domain::Stop*> GetActiveStopsSortedByName() const {
        auto stops = db_.getStops();
        std::sort(stops.begin(), stops.end(), [](const domain::Stop* lhs, const domain::Stop* rhs) {
            return lhs->name < rhs->name;
            });

        return stops;
    }

    bool IsStopExists(std::string_view name) const {
        return db_.getStop(name) != nullptr ? true : false;
    }

private:
    const transport_catalogue::TransportCatalogue& db_;
};