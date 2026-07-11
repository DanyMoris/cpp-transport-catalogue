#pragma once

#include "geo.h"
#include "svg.h"
#include "domain.h"

#include <vector>
#include <string>
#include <variant>
#include <optional>
#include <algorithm>

inline const double EPSILON = 1e-6;
inline bool IsZero(double value) {
    return std::abs(value) < EPSILON;
}

class SphereProjector {
public:
    template <typename PointInputIt>
    SphereProjector(PointInputIt points_begin, PointInputIt points_end,
        double max_width, double max_height, double padding);

    svg::Point operator()(geo::Coordinates coords) const;

private:
    double padding_;
    double min_lon_ = 0;
    double max_lat_ = 0;
    double zoom_coeff_ = 0;
};

namespace renderer {
    struct RenderSettings {
        double width = 0.0;
        double height = 0.0;
        double padding = 0.0;
        double line_width = 0.0;
        double stop_radius = 0.0;

        int bus_label_font_size = 0;
        svg::Point bus_label_offset;

        int stop_label_font_size = 0;
        svg::Point stop_label_offset;

        svg::Color underlayer_color;
        double underlayer_width = 0.0;

        std::vector<svg::Color> color_palette;
    };

    class MapRenderer {
    public:
        explicit MapRenderer(const RenderSettings& settings) : settings_(settings) {}

        void RenderRouteLines(svg::Document& doc, const std::vector<const domain::Bus*>& sorted_buses,
            const SphereProjector& projector) const;
        void RenderRouteLabels(svg::Document& doc, const std::vector<const domain::Bus*>& sorted_buses,
            const SphereProjector& projector) const;

        void RenderStopCircles(svg::Document& doc, const std::vector<const domain::Stop*>& active_stops,
            const SphereProjector& projector) const;

        void RenderStopLabels(svg::Document& doc, const std::vector<const domain::Stop*>& active_stops,
            const SphereProjector& projector) const;

        svg::Document RenderMap(const std::vector<const domain::Bus*>& buses,
            const std::vector<const domain::Stop*>& active_stops) const;


    private:
        RenderSettings settings_;
    };
}