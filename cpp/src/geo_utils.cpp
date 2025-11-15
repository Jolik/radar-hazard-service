#include "radar/geo_utils.h"

#include <cmath>

namespace radar {
namespace {
constexpr double kEarthRadiusKm = 6371.0;
constexpr double kPi = 3.14159265358979323846;
constexpr double kDegToRad = kPi / 180.0;
constexpr double kRadToDeg = 180.0 / kPi;

GeoCoordinate to_geo(double lat_rad, double lon_rad) {
    return GeoCoordinate{lat_rad * kRadToDeg, lon_rad * kRadToDeg};
}

}  // namespace

GeoCalculator::GeoCalculator(double radar_lat_deg, double radar_lon_deg, double radar_alt_m)
    : radar_lat_rad_(radar_lat_deg * kDegToRad),
      radar_lon_rad_(radar_lon_deg * kDegToRad),
      radar_alt_m_(radar_alt_m) {}

CellGeometry GeoCalculator::compute_geometry(const RadarObservation& obs, double gate_length_km) const {
    double azimuth_rad = obs.azimuth_deg * kDegToRad;
    double elevation_rad = obs.elevation_deg * kDegToRad;

    double slant_range_km = obs.range_km;
    double ground_range_km = slant_range_km * std::cos(elevation_rad);

    GeoCoordinate center = move(to_geo(radar_lat_rad_, radar_lon_rad_), ground_range_km, obs.azimuth_deg);

    double half_gate = gate_length_km / 2.0;
    double half_angle = std::asin(half_gate / (2 * ground_range_km + 1e-6)) * kRadToDeg;

    GeoCoordinate v1 = move(center, half_gate, obs.azimuth_deg - half_angle);
    GeoCoordinate v2 = move(center, half_gate, obs.azimuth_deg + half_angle);
    GeoCoordinate v3 = move(center, half_gate, obs.azimuth_deg + 180.0 + half_angle);
    GeoCoordinate v4 = move(center, half_gate, obs.azimuth_deg + 180.0 - half_angle);

    return CellGeometry{center, {v1, v2, v3, v4}};
}

GeoCoordinate GeoCalculator::move(const GeoCoordinate& start, double distance_km, double azimuth_deg) const {
    double azimuth_rad = azimuth_deg * kDegToRad;
    double lat_rad = start.latitude_deg * kDegToRad;
    double lon_rad = start.longitude_deg * kDegToRad;

    double angular_distance = distance_km / kEarthRadiusKm;

    double new_lat = std::asin(std::sin(lat_rad) * std::cos(angular_distance) +
                               std::cos(lat_rad) * std::sin(angular_distance) * std::cos(azimuth_rad));

    double new_lon = lon_rad + std::atan2(std::sin(azimuth_rad) * std::sin(angular_distance) * std::cos(lat_rad),
                                          std::cos(angular_distance) - std::sin(lat_rad) * std::sin(new_lat));

    return to_geo(new_lat, new_lon);
}

}  // namespace radar
