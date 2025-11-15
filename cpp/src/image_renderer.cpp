#include "radar/image_renderer.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <functional>
#include <limits>
#include <stdexcept>
#include <vector>

namespace radar {
namespace {

double clamp(double v, double lo, double hi) {
    return std::max(lo, std::min(v, hi));
}

void write_bitmap(const std::vector<unsigned char>& buffer, std::size_t width, std::size_t height,
                  const std::string& output_path) {
    std::ofstream out(output_path, std::ios::binary);
    if (!out) {
        throw std::runtime_error("Failed to open image output for writing");
    }

    const std::size_t row_stride = ((width * 3 + 3) / 4) * 4;
    const std::size_t image_size = row_stride * height;
    const std::uint32_t file_size = static_cast<std::uint32_t>(14 + 40 + image_size);

    unsigned char header[14] = {'B', 'M'};
    header[2] = static_cast<unsigned char>(file_size);
    header[3] = static_cast<unsigned char>(file_size >> 8);
    header[4] = static_cast<unsigned char>(file_size >> 16);
    header[5] = static_cast<unsigned char>(file_size >> 24);
    header[10] = 54;

    unsigned char dib[40] = {};
    dib[0] = 40;
    dib[4] = static_cast<unsigned char>(width);
    dib[5] = static_cast<unsigned char>(width >> 8);
    dib[6] = static_cast<unsigned char>(width >> 16);
    dib[7] = static_cast<unsigned char>(width >> 24);
    dib[8] = static_cast<unsigned char>(height);
    dib[9] = static_cast<unsigned char>(height >> 8);
    dib[10] = static_cast<unsigned char>(height >> 16);
    dib[11] = static_cast<unsigned char>(height >> 24);
    dib[12] = 1;
    dib[14] = 24;

    out.write(reinterpret_cast<const char*>(header), sizeof(header));
    out.write(reinterpret_cast<const char*>(dib), sizeof(dib));

    std::vector<unsigned char> padding(row_stride - width * 3, 0);
    for (std::size_t y = 0; y < height; ++y) {
        const auto* row = &buffer[3 * ((height - 1 - y) * width)];
        out.write(reinterpret_cast<const char*>(row), width * 3);
        if (!padding.empty()) {
            out.write(reinterpret_cast<const char*>(padding.data()), padding.size());
        }
    }
}

}  // namespace

ImageRenderer::ImageRenderer(ImageRenderOptions options) : options_(options) {
    if (options_.width == 0 || options_.height == 0) {
        throw std::invalid_argument("Image dimensions must be positive");
    }
}

void ImageRenderer::render(const std::vector<MergedContour>& contours, const std::string& output_path) const {
    const std::size_t width = options_.width;
    const std::size_t height = options_.height;

    std::vector<unsigned char> buffer(width * height * 3, 255);

    if (contours.empty()) {
        write_bitmap(buffer, width, height, output_path);
        return;
    }

    double min_lat = std::numeric_limits<double>::infinity();
    double max_lat = -std::numeric_limits<double>::infinity();
    double min_lon = std::numeric_limits<double>::infinity();
    double max_lon = -std::numeric_limits<double>::infinity();

    for (const auto& contour : contours) {
        for (const auto& vertex : contour.geometry.vertices) {
            min_lat = std::min(min_lat, vertex.latitude_deg);
            max_lat = std::max(max_lat, vertex.latitude_deg);
            min_lon = std::min(min_lon, vertex.longitude_deg);
            max_lon = std::max(max_lon, vertex.longitude_deg);
        }
    }

    if (!std::isfinite(min_lat) || !std::isfinite(max_lat) || !std::isfinite(min_lon) || !std::isfinite(max_lon)) {
        throw std::runtime_error("Unable to determine contour bounds");
    }

    double lat_span = max_lat - min_lat;
    double lon_span = max_lon - min_lon;
    const double lat_padding = std::max(lat_span * options_.padding_ratio, 1e-6);
    const double lon_padding = std::max(lon_span * options_.padding_ratio, 1e-6);

    min_lat -= lat_padding;
    max_lat += lat_padding;
    min_lon -= lon_padding;
    max_lon += lon_padding;

    lat_span = max_lat - min_lat;
    lon_span = max_lon - min_lon;

    auto x_to_lon = [&](std::size_t x) {
        if (lon_span <= 0.0) {
            return (min_lon + max_lon) * 0.5;
        }
        double ratio = (static_cast<double>(x) + 0.5) / static_cast<double>(width);
        return min_lon + ratio * lon_span;
    };

    auto y_to_lat = [&](std::size_t y) {
        if (lat_span <= 0.0) {
            return (min_lat + max_lat) * 0.5;
        }
        double ratio = (static_cast<double>(y) + 0.5) / static_cast<double>(height);
        return max_lat - ratio * lat_span;
    };

    auto lon_to_x_index = [&](double lon) {
        if (lon_span <= 0.0 || width == 1) {
            return 0.0;
        }
        double ratio = (lon - min_lon) / lon_span;
        return clamp(ratio, 0.0, 1.0) * static_cast<double>(width - 1);
    };

    auto lat_to_y_index = [&](double lat) {
        if (lat_span <= 0.0 || height == 1) {
            return 0.0;
        }
        double ratio = (max_lat - lat) / lat_span;
        return clamp(ratio, 0.0, 1.0) * static_cast<double>(height - 1);
    };

    for (const auto& contour : contours) {
        if (contour.geometry.vertices.empty()) {
            continue;
        }

        double contour_min_lat = std::numeric_limits<double>::infinity();
        double contour_max_lat = -std::numeric_limits<double>::infinity();
        double contour_min_lon = std::numeric_limits<double>::infinity();
        double contour_max_lon = -std::numeric_limits<double>::infinity();

        for (const auto& vertex : contour.geometry.vertices) {
            contour_min_lat = std::min(contour_min_lat, vertex.latitude_deg);
            contour_max_lat = std::max(contour_max_lat, vertex.latitude_deg);
            contour_min_lon = std::min(contour_min_lon, vertex.longitude_deg);
            contour_max_lon = std::max(contour_max_lon, vertex.longitude_deg);
        }

        int min_x = static_cast<int>(std::floor(lon_to_x_index(contour_min_lon)));
        int max_x = static_cast<int>(std::ceil(lon_to_x_index(contour_max_lon)));
        int min_y = static_cast<int>(std::floor(lat_to_y_index(contour_max_lat)));
        int max_y = static_cast<int>(std::ceil(lat_to_y_index(contour_min_lat)));

        min_x = std::clamp(min_x, 0, static_cast<int>(width) - 1);
        max_x = std::clamp(max_x, 0, static_cast<int>(width) - 1);
        min_y = std::clamp(min_y, 0, static_cast<int>(height) - 1);
        max_y = std::clamp(max_y, 0, static_cast<int>(height) - 1);

        const unsigned rgba = rgba_from_string(contour.phenomenon_type);
        const unsigned char r = static_cast<unsigned char>((rgba >> 24) & 0xFF);
        const unsigned char g = static_cast<unsigned char>((rgba >> 16) & 0xFF);
        const unsigned char b = static_cast<unsigned char>((rgba >> 8) & 0xFF);

        for (int y = min_y; y <= max_y; ++y) {
            const double lat = y_to_lat(static_cast<std::size_t>(y));
            for (int x = min_x; x <= max_x; ++x) {
                const double lon = x_to_lon(static_cast<std::size_t>(x));
                if (point_in_polygon(lat, lon, contour.geometry)) {
                    auto* pixel = &buffer[3 * (y * static_cast<int>(width) + x)];
                    pixel[0] = b;
                    pixel[1] = g;
                    pixel[2] = r;
                }
            }
        }
    }

    write_bitmap(buffer, width, height, output_path);
}

bool ImageRenderer::point_in_polygon(double lat, double lon, const Polygon& polygon) {
    bool inside = false;
    const std::size_t n = polygon.vertices.size();
    if (n < 3) {
        return false;
    }
    for (std::size_t i = 0, j = n - 1; i < n; j = i++) {
        const auto& vi = polygon.vertices[i];
        const auto& vj = polygon.vertices[j];
        const double xi = vi.longitude_deg;
        const double yi = vi.latitude_deg;
        const double xj = vj.longitude_deg;
        const double yj = vj.latitude_deg;

        const bool intersect = ((yi > lat) != (yj > lat)) &&
                               (lon < (xj - xi) * (lat - yi) / (yj - yi + 1e-12) + xi);
        if (intersect) {
            inside = !inside;
        }
    }
    return inside;
}

unsigned ImageRenderer::rgba_from_string(const std::string& key) {
    std::size_t hash = std::hash<std::string>{}(key);
    unsigned r = 80 + static_cast<unsigned>(hash & 0x7F);
    unsigned g = 80 + static_cast<unsigned>((hash >> 7) & 0x7F);
    unsigned b = 80 + static_cast<unsigned>((hash >> 14) & 0x7F);
    return (r << 24) | (g << 16) | (b << 8) | 0xFF;
}

}  // namespace radar

