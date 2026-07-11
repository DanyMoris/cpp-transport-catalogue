
#pragma once
#include <cmath>
#include <numbers>

namespace geo {
    struct Coordinates {
        double lat;
        double lng;
        bool operator==(const Coordinates& other) const {
            return lat == other.lat && lng == other.lng;
        }
        bool operator!=(const Coordinates& other) const {
            return !(*this == other);
        }
    };

    inline double ComputeDistance(geo::Coordinates from, geo::Coordinates to) {
        using namespace std;
        if (from == to) {
            return 0;
        }
        static const double dr = std::numbers::pi / 180.;
        return acos(sin(from.lat * dr) * sin(to.lat * dr)
            + cos(from.lat * dr) * cos(to.lat * dr) * cos(abs(from.lng - to.lng) * dr))
            * 6371000;
    }

}  // namespace geo 