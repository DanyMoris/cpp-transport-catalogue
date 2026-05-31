#pragma once
#include "geo.h"
#include <deque>
#include <vector>
#include <string>
#include <string_view>
#include <unordered_map>
#include <set>


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
		double route_length = 0.0;
	};

	class TransportCatalogue {
	private:
		std::deque<Stop> stations_;
		std::deque<Bus> routes_;
		std::unordered_map<std::string_view, const Stop*> map_stops;
		std::unordered_map<std::string_view, const Bus*> map_bus;
		std::unordered_map<std::string_view, std::set<std::string_view>> stop_to_buses_;

	public:
		TransportCatalogue() = default;

		void addStop(std::string_view name, Coordinates coordinates);
		void addBus(std::string_view name, const std::vector<std::string_view>& route, bool is_cirlce);
		const Stop* getStop(std::string_view id) const;
		const Bus* getBus(std::string_view name) const;
		BusInfo getBusInfo(std::string_view id) const;
		std::set<std::string_view> getStopInfo(std::string_view name) const;
	};
}