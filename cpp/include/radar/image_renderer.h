#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "radar/contour_merger.h"

namespace radar {

struct ImageRenderOptions {
    std::size_t width = 1024;
    std::size_t height = 1024;
    double padding_ratio = 0.05;
};

class ImageRenderer {
public:
    explicit ImageRenderer(ImageRenderOptions options = {});

    void render(const std::vector<MergedContour>& contours, const std::string& output_path) const;

private:
    ImageRenderOptions options_;

    static bool point_in_polygon(double lat, double lon, const Polygon& polygon);
    static unsigned rgba_from_string(const std::string& key);
};

}  // namespace radar

