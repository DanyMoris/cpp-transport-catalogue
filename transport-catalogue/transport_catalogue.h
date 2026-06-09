#pragma once
#include "geo.h"
#include <deque>
#include <vector>
#include <string>
#include <string_view>
#include <unordered_map>
#include <optional>
#include <unordered_set>


namespace transport_catalogue {
	struct Stop {
		std::string name;
		Coordinates coordinates;
	};

	struct Bus {
		std::string id;
		std::vector<const Stop*> route;
		bool is_circle;
	};

	struct BusInfo {
		size_t total_stops = 0;
		size_t unique_stops = 0;
		int route_length = 0;
		double curvature = 0.0;
	};

	struct StopDistancesHasher {
		size_t operator()(const std::pair<const Stop*, const Stop*>& points) const {
			size_t hash1 = std::hash<const void*>{}(points.first);
			size_t hash2 = std::hash<const void*>{}(points.second);
			return hash1 + hash2 * 37;
		}
	};

	class TransportCatalogue {
	private:
		std::deque<Stop> stations_;
		std::deque<Bus> routes_;
		std::unordered_map<std::string_view, const Stop*> map_stops_;
		std::unordered_map<std::string_view, const Bus*> map_bus_;
		std::unordered_map<std::string_view, std::unordered_set<std::string_view>> stop_to_buses_;

		std::unordered_map<std::pair<const Stop*, const Stop*>, int, StopDistancesHasher> distances_;

	public:
		void addStop(std::string_view name, Coordinates coordinates);
		void addBus(std::string_view name, const std::vector<std::string_view>& route, bool is_cirlce);
		const Stop* getStop(std::string_view id) const;
		const Bus* getBus(std::string_view name) const;
		BusInfo getBusInfo(std::string_view id) const;
		const std::unordered_set<std::string_view>& getStopInfo(std::string_view name) const;

		void SetDistance(const Stop* from, const Stop* to, int distance);
		int GetDistance(const Stop* from, const Stop* to) const;
	};
}