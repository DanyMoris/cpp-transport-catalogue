#pragma once
#include "geo.h"
#include <string>
#include <vector>

namespace domain {
	struct Stop {
		std::string name;
		geo::Coordinates coordinates;
	};

	struct Bus {
		std::string id;
		std::vector<const Stop*> route;
		bool is_circle = false;
	};

	struct BusInfo {
		size_t total_stops = 0;
		size_t unique_stops = 0;
		int route_length = 0;
		double curvature = 0.0;
	};

}