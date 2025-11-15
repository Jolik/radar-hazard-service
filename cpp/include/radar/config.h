#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace radar {

struct DescriptorDefinition {
    std::string mnemonic;
    int scale = 0;
    int reference = 0;
    int bits = 0;
    std::string unit;
};

struct PipelineConfig {
    std::string bufr_input;
    std::string csv_output_dir;
    std::string echo_tops_matrix;
    std::string merged_geojson_output;
    std::string image_output_path;
    std::string tables_path;
    double radar_latitude = 0.0;
    double radar_longitude = 0.0;
    double radar_altitude_m = 0.0;
    double grid_cell_size_km = 1.0;
    std::size_t image_width = 1024;
    std::size_t image_height = 1024;
    std::vector<double> reflectivity_thresholds;
    std::vector<std::string> allowed_phenomena;
};

class ConfigLoader {
public:
    static PipelineConfig load_pipeline(const std::string& path);
    static std::unordered_map<std::string, DescriptorDefinition> load_tables(const std::string& path);
};

}  // namespace radar
