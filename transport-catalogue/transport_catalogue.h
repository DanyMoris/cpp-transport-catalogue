#pragma once
#include "domain.h"
#include <deque>
#include <vector>
#include <string>
#include <string_view>
#include <unordered_map>
#include <optional>
#include <unordered_set>


namespace transport_catalogue {

	struct StopDistancesHasher {
		size_t operator()(const std::pair<const domain::Stop*, const domain::Stop*>& points) const {
			size_t hash1 = std::hash<const void*>{}(points.first);
			size_t hash2 = std::hash<const void*>{}(points.second);
			return hash1 + hash2 * 37;
		}
	};

	class TransportCatalogue {
	private:
		std::deque<domain::Stop> stations_;
		std::deque<domain::Bus> routes_;
		std::unordered_map<std::string_view, const domain::Stop*> map_stops_;
		std::unordered_map<std::string_view, const domain::Bus*> map_bus_;
		std::unordered_map<std::string_view, std::unordered_set<std::string_view>> stop_to_buses_;

		std::unordered_map<std::pair<const domain::Stop*, const domain::Stop*>, int, StopDistancesHasher> distances_;

	public:
		void addStop(std::string_view name, geo::Coordinates coordinates);
		void addBus(std::string_view name, const std::vector<std::string_view>& route, bool is_cirlce);
		const domain::Stop* getStop(std::string_view id) const;
		const domain::Bus* getBus(std::string_view name) const;
		std::optional<domain::BusInfo> getBusInfo(std::string_view id) const;
		const std::unordered_set<std::string_view>* getBusesByStop(std::string_view name) const;

		const std::deque<domain::Bus>& getBuses() const;
		const std::deque<domain::Stop>& getStops() const;

		void SetDistance(const domain::Stop* from, const domain::Stop* to, int distance);
		int GetDistance(const domain::Stop* from, const domain::Stop* to) const;
	};
}