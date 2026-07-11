#include "request_handler.h"

const std::vector<const domain::Stop*> RequestHandler::GetActiveStopsSortedByName() const {
    const auto& all_stops = db_.getStops();
    std::vector<const domain::Stop*> result;
    result.reserve(all_stops.size());

    for (const auto& stop : all_stops) {
        auto buses_for_stop = db_.getBusesByStop(stop.name);
        if (buses_for_stop && !buses_for_stop->empty()) {
            result.push_back(&stop);
        }
    }

    std::sort(result.begin(), result.end(), [](const domain::Stop* lhs, const domain::Stop* rhs) {
        return lhs->name < rhs->name;
        });

    return result;
}

const std::vector<const domain::Bus*> RequestHandler::GetBusesSortedByName() const {
    const auto& all_buses = db_.getBuses();

    std::vector<const domain::Bus*> result;
    result.reserve(all_buses.size());

    for (const auto& bus : all_buses) {
        result.push_back(&bus);
    }

    std::sort(result.begin(), result.end(), [](const domain::Bus* lhs, const domain::Bus* rhs) {
        return lhs->id < rhs->id;
        });
    return result;
}