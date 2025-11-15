#pragma once

#include <optional>
#include <string>
#include <vector>

#include "radar/cluster_analyzer.h"

namespace radar {

struct Polygon {
    std::vector<GeoCoordinate> vertices;
};

struct MergedContour {
    std::string phenomenon_type;
    Polygon geometry;
    double max_reflectivity = 0.0;
    std::optional<double> max_echo_top_km;
};

class ContourMerger {
public:
    std::vector<MergedContour> merge(const std::vector<Cluster>& clusters) const;
    void write_geojson(const std::vector<MergedContour>& contours, const std::string& path) const;

private:
    Polygon convex_hull(const std::vector<GeoCoordinate>& points) const;
    static bool bounding_boxes_intersect(const Polygon& a, const Polygon& b);
    static Polygon merge_polygons(const Polygon& a, const Polygon& b);
};

}  // namespace radar
