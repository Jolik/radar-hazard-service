#include "radar/config.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

#include "radar/json.h"

namespace radar {
namespace {

PipelineConfig from_json(const JsonValue& j) {
    PipelineConfig config;
    config.bufr_input = j.at("bufr_input").as_string();
    config.csv_output_dir = j.at("csv_output_dir").as_string();
    config.echo_tops_matrix = j.at("echo_tops_matrix").as_string();
    config.merged_geojson_output = j.at("merged_geojson_output").as_string();
    if (const auto* image = json_try_get(j, "image_output_path")) {
        config.image_output_path = image->as_string();
    }
    config.tables_path = j.at("tables_path").as_string();
    if (const auto* lat = json_try_get(j, "radar_latitude")) {
        config.radar_latitude = lat->as_number();
    }
    if (const auto* lon = json_try_get(j, "radar_longitude")) {
        config.radar_longitude = lon->as_number();
    }
    if (const auto* alt = json_try_get(j, "radar_altitude_m")) {
        config.radar_altitude_m = alt->as_number();
    }
    if (const auto* cell = json_try_get(j, "grid_cell_size_km")) {
        config.grid_cell_size_km = cell->as_number();
    }
    if (const auto* image_width = json_try_get(j, "image_width")) {
        config.image_width = static_cast<std::size_t>(image_width->as_number());
    }
    if (const auto* image_height = json_try_get(j, "image_height")) {
        config.image_height = static_cast<std::size_t>(image_height->as_number());
    }
    if (const auto* thresholds = json_try_get(j, "reflectivity_thresholds")) {
        for (const auto& value : thresholds->as_array()) {
            config.reflectivity_thresholds.push_back(value.as_number());
        }
    }
    if (const auto* allowed = json_try_get(j, "allowed_phenomena")) {
        for (const auto& value : allowed->as_array()) {
            config.allowed_phenomena.push_back(value.as_string());
        }
    }
    return config;
}

DescriptorDefinition descriptor_from_json(const JsonValue& j) {
    DescriptorDefinition def;
    def.mnemonic = j.at("mnemonic").as_string();
    def.scale = static_cast<int>(j.at("scale").as_number());
    def.reference = static_cast<int>(j.at("reference").as_number());
    def.bits = static_cast<int>(j.at("bits").as_number());
    if (const auto* unit = json_try_get(j, "unit")) {
        def.unit = unit->as_string();
    }
    return def;
}

}  // namespace

PipelineConfig ConfigLoader::load_pipeline(const std::string& path) {
    std::ifstream stream(path);
    if (!stream.is_open()) {
        throw std::runtime_error("Cannot open pipeline configuration: " + path);
    }
    std::ostringstream buffer;
    buffer << stream.rdbuf();
    JsonParser parser(buffer.str());
    return from_json(parser.parse());
}

std::unordered_map<std::string, DescriptorDefinition> ConfigLoader::load_tables(const std::string& path) {
    std::ifstream stream(path);
    if (!stream.is_open()) {
        throw std::runtime_error("Cannot open descriptor tables: " + path);
    }
    std::ostringstream buffer;
    buffer << stream.rdbuf();
    JsonParser parser(buffer.str());
    auto root = parser.parse();
    std::unordered_map<std::string, DescriptorDefinition> tables;
    for (const auto& [key, value] : root.as_object()) {
        tables.emplace(key, descriptor_from_json(value));
    }
    return tables;
}

}  // namespace radar
