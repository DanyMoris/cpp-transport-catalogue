#pragma once
#include "transport_catalogue.h"
#include "transport_router.h"
#include "map_renderer.h"
#include <memory>
#include <optional>
#include <unordered_set>
#include <algorithm>

class RequestHandler {
public:
    explicit RequestHandler(const transport_catalogue::TransportCatalogue& db) : db_(db) {}

    void SetRouter(int bus_wait_time,
        double bus_velocity_kmph) const {
        transport_router_ = std::make_unique<transport_router::TransportRouter>(db_, bus_wait_time, bus_velocity_kmph);
    }

    std::optional<transport_router::RouteResult> FindRoute(std::string_view from, std::string_view to) const {
        if (!transport_router_) {
            return std::nullopt;
        }
        return transport_router_->BuildRoute(from, to);
    }

    std::optional<domain::BusInfo> GetBusStat(const std::string_view& bus_name) const {
        return db_.getBusInfo(bus_name);
    }

    const std::unordered_set<std::string_view>* GetBusesByStop(std::string_view stop_name) const {
        return db_.getBusesByStop(stop_name);
    }

    const std::vector<const domain::Bus*> GetBusesSortedByName() const;
    const std::vector<const domain::Stop*> GetActiveStopsSortedByName() const;

    bool IsStopExists(std::string_view name) const {
        return db_.getStop(name) != nullptr ? true : false;
    }

private:
    const transport_catalogue::TransportCatalogue& db_;
    mutable std::unique_ptr<transport_router::TransportRouter> transport_router_;
};