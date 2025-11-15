#pragma once

#include <array>
#include <vector>

namespace radar {

struct GeoCoordinate {
    double latitude_deg = 0.0;
    double longitude_deg = 0.0;
};

struct RadarObservation {
    double azimuth_deg = 0.0;
    double range_km = 0.0;
    double elevation_deg = 0.0;
};

struct CellGeometry {
    GeoCoordinate center;
    std::array<GeoCoordinate, 4> vertices;
};

class GeoCalculator {
public:
    GeoCalculator(double radar_lat_deg, double radar_lon_deg, double radar_alt_m);

    CellGeometry compute_geometry(const RadarObservation& obs, double gate_length_km) const;

private:
    GeoCoordinate move(const GeoCoordinate& start, double distance_km, double azimuth_deg) const;

    double radar_lat_rad_;
    double radar_lon_rad_;
    double radar_alt_m_;
};

}  // namespace radar
