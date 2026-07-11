#include "svg.h"

namespace svg {

    using namespace std::literals;

    std::ostream& operator<<(std::ostream& out, StrokeLineCap cap) {
        switch (cap) {
        case StrokeLineCap::BUTT:   out << "butt"sv; break;
        case StrokeLineCap::ROUND:  out << "round"sv; break;
        case StrokeLineCap::SQUARE: out << "square"sv; break;
        }
        return out;
    }

    std::ostream& operator<<(std::ostream& out, StrokeLineJoin join) {
        switch (join) {
        case StrokeLineJoin::ARCS:       out << "arcs"sv; break;
        case StrokeLineJoin::BEVEL:      out << "bevel"sv; break;
        case StrokeLineJoin::MITER:      out << "miter"sv; break;
        case StrokeLineJoin::MITER_CLIP: out << "miter-clip"sv; break;
        case StrokeLineJoin::ROUND:      out << "round"sv; break;
        }
        return out;
    }

    struct ColorPrinter {
        std::ostream& out;

        void operator()(std::monostate) const {
            out << "none"sv;
        }
        void operator()(const std::string& str) const {
            out << str;
        }
        void operator()(Rgb rgb) const {
            out << "rgb("sv << static_cast<int>(rgb.red)
                << ","sv << static_cast<int>(rgb.green)
                << ","sv << static_cast<int>(rgb.blue) << ")"sv;
        }
        void operator()(Rgba rgba) const {
            out << "rgba("sv << static_cast<int>(rgba.red)
                << ","sv << static_cast<int>(rgba.green)
                << ","sv << static_cast<int>(rgba.blue)
                << ","sv << rgba.opacity << ")"sv;
        }
    };

    std::ostream& operator<<(std::ostream& out, const Color& color) {
        std::visit(ColorPrinter{ out }, color);
        return out;
    }

    void Object::Render(const RenderContext& context) const {
        context.RenderIndent();
        RenderObject(context);
        context.out << std::endl;
    }

    // ---------- Circle ------------------
    Circle& Circle::SetCenter(Point center) {
        center_ = center;
        return *this;
    }

    Circle& Circle::SetRadius(double radius) {
        radius_ = radius;
        return *this;
    }

    void Circle::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
        out << "r=\""sv << radius_ << "\""sv;
        RenderAttrs(out);
        out << "/>"sv;
    }

    // ---------- Polyline ----------------
    Polyline& Polyline::AddPoint(Point point) {
        points_.push_back(point);
        return *this;
    }

    void Polyline::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<polyline points=\""sv;
        bool first = true;
        for (const auto& p : points_) {
            if (!first) out << " "sv;
            out << p.x << ","sv << p.y;
            first = false;
        }
        out << "\""sv;
        RenderAttrs(out);
        out << "/>"sv;
    }

    // ---------- Text --------------------
    Text& Text::SetPosition(Point pos) {
        pos_ = pos;
        return *this;
    }

    Text& Text::SetOffset(Point offset) {
        offset_ = offset;
        return *this;
    }

    Text& Text::SetFontSize(uint32_t size) {
        font_size_ = size;
        return *this;
    }

    Text& Text::SetFontFamily(std::string font_family) {
        font_family_ = std::move(font_family);
        return *this;
    }

    Text& Text::SetFontWeight(std::string font_weight) {
        font_weight_ = std::move(font_weight);
        return *this;
    }

    Text& Text::SetData(std::string data) {
        data_ = std::move(data);
        return *this;
    }

    void Text::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<text";
        RenderAttrs(out);
        out << " x=\""sv << pos_.x << "\" y=\""sv << pos_.y << "\" "sv
            << "dx=\""sv << offset_.x << "\" dy=\""sv << offset_.y << "\" "sv
            << "font-size=\""sv << font_size_ << "\""sv;

        if (!font_family_.empty()) out << " font-family=\""sv << font_family_ << "\""sv;
        if (!font_weight_.empty()) out << " font-weight=\""sv << font_weight_ << "\""sv;

        out << ">"sv;

        for (char c : data_) {
            switch (c) {
            case '"': out << "&quot;"sv; break;
            case '\'': out << "&apos;"sv; break;
            case '<': out << "&lt;"sv; break;
            case '>': out << "&gt;"sv; break;
            case '&': out << "&amp;"sv; break;
            default: out << c; break;
            }
        }
        out << "</text>"sv;
    }

    // ---------- Document ----------------
    void Document::AddPtr(std::unique_ptr<Object>&& obj) {
        objects_.push_back(std::move(obj));
    }

    void Document::Render(std::ostream& out) const {
        out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"sv;
        out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n"sv;

        RenderContext ctx(out, 2, 2);
        for (const auto& obj : objects_) {
            obj->Render(ctx);
        }

        out << "</svg>"sv;
    }

}  // namespace svg