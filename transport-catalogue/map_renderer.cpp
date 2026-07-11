#include "map_renderer.h"

template <typename PointInputIt>
SphereProjector::SphereProjector(PointInputIt points_begin, PointInputIt points_end,
    double max_width, double max_height, double padding) : padding_(padding) {
    if (points_begin == points_end) return;

    const auto [left_it, right_it] = std::minmax_element(
        points_begin, points_end, [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
    min_lon_ = left_it->lng;
    const double max_lon = right_it->lng;

    const auto [bottom_it, top_it] = std::minmax_element(
        points_begin, points_end,
        [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
    const double min_lat = bottom_it->lat;
    max_lat_ = top_it->lat;

    std::optional<double> width_zoom;
    if (!IsZero(max_lon - min_lon_)) {
        width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
    }

    std::optional<double> height_zoom;
    if (!IsZero(max_lat_ - min_lat)) {
        height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
    }

    if (width_zoom && height_zoom) {
        zoom_coeff_ = std::min(*width_zoom, *height_zoom);
    }
    else if (width_zoom) {
        zoom_coeff_ = *width_zoom;
    }
    else if (height_zoom) {
        zoom_coeff_ = *height_zoom;
    }
}

svg::Point SphereProjector::operator()(geo::Coordinates coords) const {
    return {
            (coords.lng - min_lon_) * zoom_coeff_ + padding_,
            (max_lat_ - coords.lat) * zoom_coeff_ + padding_
    };
}

namespace renderer {
    void MapRenderer::RenderRouteLines(svg::Document& doc, const std::vector<const domain::Bus*>& sorted_buses,
        const SphereProjector& projector) const {

        size_t color_index = 0;
        const size_t palette_size = settings_.color_palette.size();

        for (const auto* bus : sorted_buses) {
            if (bus->route.empty()) {
                continue;
            }

            svg::Polyline line;

            line.SetStrokeColor(settings_.color_palette[color_index % palette_size])
                .SetFillColor(svg::NoneColor)
                .SetStrokeWidth(settings_.line_width)
                .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

            ++color_index;

            if (bus->is_circle) {
                for (const auto* stop : bus->route) {
                    line.AddPoint(projector(stop->coordinates));
                }
            }
            else {
                for (const auto* stop : bus->route) {
                    line.AddPoint(projector(stop->coordinates));
                }
                for (auto it = std::next(bus->route.rbegin()); it != bus->route.rend(); ++it) {
                    line.AddPoint(projector((*it)->coordinates));
                }
            }

            doc.Add(std::move(line));
        }
    }

    void MapRenderer::RenderRouteLabels(svg::Document& doc, const std::vector<const domain::Bus*>& buses,
        const SphereProjector& projector) const {
        int color_index = 0;
        for (const auto* bus : buses) {
            if (bus->route.empty()) {
                continue;
            }

            const svg::Color& route_color = settings_.color_palette[color_index % settings_.color_palette.size()];

            auto add_label = [&](const domain::Stop* stop) {
                svg::Point pos = projector(stop->coordinates);

                svg::Text base_text;
                base_text.SetPosition(pos)
                    .SetOffset({ settings_.bus_label_offset.x, settings_.bus_label_offset.y })
                    .SetFontSize(settings_.bus_label_font_size)
                    .SetFontFamily("Verdana")
                    .SetFontWeight("bold")
                    .SetData(bus->id);

                svg::Text underlayer = base_text;
                underlayer.SetFillColor(settings_.underlayer_color)
                    .SetStrokeColor(settings_.underlayer_color)
                    .SetStrokeWidth(settings_.underlayer_width)
                    .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                    .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
                doc.Add(underlayer);

                svg::Text text = base_text;
                text.SetFillColor(route_color);
                doc.Add(text);
                };

            add_label(bus->route.front());

            if (!bus->is_circle && bus->route.front() != bus->route.back()) {
                add_label(bus->route.back());
            }

            ++color_index;
        }
    }

    void MapRenderer::RenderStopCircles(svg::Document& doc, const std::vector<const domain::Stop*>& active_stops,
        const SphereProjector& projector) const {
        for (const auto* stop : active_stops) {
            svg::Circle circle;
            circle.SetCenter(projector(stop->coordinates))
                .SetRadius(settings_.stop_radius)
                .SetFillColor("white");
            doc.Add(circle);
        }
    }

    void MapRenderer::RenderStopLabels(svg::Document& doc, const std::vector<const domain::Stop*>& active_stops,
        const SphereProjector& projector) const {
        for (const auto* stop : active_stops) {
            svg::Point pos = projector(stop->coordinates);

            svg::Text base_text;
            base_text.SetPosition(pos)
                .SetOffset({ settings_.stop_label_offset.x, settings_.stop_label_offset.y })
                .SetFontSize(settings_.stop_label_font_size)
                .SetFontFamily("Verdana")
                .SetData(stop->name);

            svg::Text underlayer = base_text;
            underlayer.SetFillColor(settings_.underlayer_color)
                .SetStrokeColor(settings_.underlayer_color)
                .SetStrokeWidth(settings_.underlayer_width)
                .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
            doc.Add(underlayer);

            svg::Text text = base_text;
            text.SetFillColor("black");
            doc.Add(text);
        }
    }

    svg::Document MapRenderer::RenderMap(const std::vector<const domain::Bus*>& sorted_buses,
        const std::vector<const domain::Stop*>& sorted_stops) const {
        svg::Document doc;

        std::vector<geo::Coordinates> active_coords;
        for (const auto* stop : sorted_stops) {
            active_coords.push_back(stop->coordinates);
        }

        if (active_coords.empty()) {
            return doc;
        }

        SphereProjector projector(
            active_coords.begin(), active_coords.end(),
            settings_.width, settings_.height, settings_.padding
        );

        RenderRouteLines(doc, sorted_buses, projector);
        RenderRouteLabels(doc, sorted_buses, projector);
        RenderStopCircles(doc, sorted_stops, projector);
        RenderStopLabels(doc, sorted_stops, projector);

        return doc;
    }
}



