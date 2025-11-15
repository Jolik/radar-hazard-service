#include "radar/contour_merger.h"

#include <algorithm>
#include <array>
#include <fstream>
#include <limits>
#include <numeric>
#include <stdexcept>

namespace radar {

namespace {

double cross(const GeoCoordinate& o, const GeoCoordinate& a, const GeoCoordinate& b) {
    double ax = a.longitude_deg - o.longitude_deg;
    double ay = a.latitude_deg - o.latitude_deg;
    double bx = b.longitude_deg - o.longitude_deg;
    double by = b.latitude_deg - o.latitude_deg;
    return ax * by - ay * bx;
}

}

Polygon ContourMerger::convex_hull(const std::vector<GeoCoordinate>& points) const {
    if (points.size() <= 3) {
        Polygon polygon;
        polygon.vertices = points;
        if (!polygon.vertices.empty()) {
            polygon.vertices.push_back(polygon.vertices.front());
        }
        return polygon;
    }
    std::vector<GeoCoordinate> sorted = points;
    std::sort(sorted.begin(), sorted.end(), [](const GeoCoordinate& a, const GeoCoordinate& b) {
        if (a.longitude_deg == b.longitude_deg) {
            return a.latitude_deg < b.latitude_deg;
        }
        return a.longitude_deg < b.longitude_deg;
    });

    std::vector<GeoCoordinate> hull;
    hull.reserve(sorted.size() * 2);

    for (const auto& point : sorted) {
        while (hull.size() >= 2 && cross(hull[hull.size() - 2], hull.back(), point) <= 0) {
            hull.pop_back();
        }
        hull.push_back(point);
    }

    std::size_t lower_size = hull.size();
    for (auto it = sorted.rbegin(); it != sorted.rend(); ++it) {
        while (hull.size() > lower_size && cross(hull[hull.size() - 2], hull.back(), *it) <= 0) {
            hull.pop_back();
        }
        hull.push_back(*it);
    }

    if (!hull.empty()) {
        hull.pop_back();
        hull.push_back(hull.front());
    }

    return Polygon{hull};
}

bool ContourMerger::bounding_boxes_intersect(const Polygon& a, const Polygon& b) {
    auto compute_bounds = [](const Polygon& poly) {
        double min_lat = std::numeric_limits<double>::max();
        double max_lat = std::numeric_limits<double>::lowest();
        double min_lon = std::numeric_limits<double>::max();
        double max_lon = std::numeric_limits<double>::lowest();
        for (const auto& vertex : poly.vertices) {
            min_lat = std::min(min_lat, vertex.latitude_deg);
            max_lat = std::max(max_lat, vertex.latitude_deg);
            min_lon = std::min(min_lon, vertex.longitude_deg);
            max_lon = std::max(max_lon, vertex.longitude_deg);
        }
        return std::array<double, 4>{min_lat, max_lat, min_lon, max_lon};
    };

    auto bounds_a = compute_bounds(a);
    auto bounds_b = compute_bounds(b);
    return !(bounds_a[1] < bounds_b[0] || bounds_b[1] < bounds_a[0] ||
             bounds_a[3] < bounds_b[2] || bounds_b[3] < bounds_a[2]);
}

Polygon ContourMerger::merge_polygons(const Polygon& a, const Polygon& b) {
    std::vector<GeoCoordinate> points;
    if (!a.vertices.empty()) {
        points.insert(points.end(), a.vertices.begin(), a.vertices.end() - 1);
    }
    if (!b.vertices.empty()) {
        points.insert(points.end(), b.vertices.begin(), b.vertices.end() - 1);
    }
    ContourMerger merger;
    return merger.convex_hull(points);
}

std::vector<MergedContour> ContourMerger::merge(const std::vector<Cluster>& clusters) const {
    std::vector<MergedContour> contours;
    for (const auto& cluster : clusters) {
        std::vector<GeoCoordinate> points;
        for (const auto& cell : cluster.cells) {
            points.insert(points.end(), cell.geometry.vertices.begin(), cell.geometry.vertices.end());
        }
        if (points.empty()) {
            continue;
        }
        auto polygon = convex_hull(points);
        if (polygon.vertices.empty()) {
            continue;
        }
        MergedContour contour;
        contour.geometry = polygon;
        contour.max_reflectivity = cluster.max_reflectivity;
        contour.max_echo_top_km = cluster.max_echo_top_km;
        if (!cluster.cells.empty()) {
            contour.phenomenon_type = cluster.cells.front().phenomenon_type;
        }
        contours.push_back(std::move(contour));
    }

    bool changed = true;
    while (changed) {
        changed = false;
        for (std::size_t i = 0; i < contours.size(); ++i) {
            for (std::size_t j = i + 1; j < contours.size(); ++j) {
                if (contours[i].phenomenon_type != contours[j].phenomenon_type) {
                    continue;
                }
                if (bounding_boxes_intersect(contours[i].geometry, contours[j].geometry)) {
                    contours[i].geometry = merge_polygons(contours[i].geometry, contours[j].geometry);
                    contours[i].max_reflectivity = std::max(contours[i].max_reflectivity, contours[j].max_reflectivity);
                    if (contours[i].max_echo_top_km.has_value() || contours[j].max_echo_top_km.has_value()) {
                        contours[i].max_echo_top_km = std::max(contours[i].max_echo_top_km.value_or(0.0),
                                                              contours[j].max_echo_top_km.value_or(0.0));
                    }
                    contours.erase(contours.begin() + static_cast<long>(j));
                    changed = true;
                    break;
                }
            }
            if (changed) {
                break;
            }
        }
    }

    return contours;
}

void ContourMerger::write_geojson(const std::vector<MergedContour>& contours, const std::string& path) const {
    std::ofstream stream(path);
    if (!stream.is_open()) {
        throw std::runtime_error("Cannot open GeoJSON output: " + path);
    }
    stream << "{\n  \"type\": \"FeatureCollection\",\n  \"features\": [\n";
    for (std::size_t i = 0; i < contours.size(); ++i) {
        const auto& contour = contours[i];
        stream << "    {\n      \"type\": \"Feature\",\n      \"properties\": {\n";
        stream << "        \"phenomenon\": \"" << contour.phenomenon_type << "\",\n";
        stream << "        \"max_reflectivity\": " << contour.max_reflectivity << ",\n";
        if (contour.max_echo_top_km.has_value()) {
            stream << "        \"max_echo_top_km\": " << contour.max_echo_top_km.value() << "\n";
        } else {
            stream << "        \"max_echo_top_km\": null\n";
        }
        stream << "      },\n      \"geometry\": {\n        \"type\": \"Polygon\",\n        \"coordinates\": [\n          [";
        for (std::size_t p = 0; p < contour.geometry.vertices.size(); ++p) {
            const auto& vertex = contour.geometry.vertices[p];
            stream << "[" << vertex.longitude_deg << ", " << vertex.latitude_deg << "]";
            if (p + 1 != contour.geometry.vertices.size()) {
                stream << ", ";
            }
        }
        stream << "]\n        ]\n      }\n    }";
        if (i + 1 != contours.size()) {
            stream << ",";
        }
        stream << "\n";
    }
    stream << "  ]\n}\n";
}

}  // namespace radar
