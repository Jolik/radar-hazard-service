#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <unordered_map>

#include "radar/bufr_decoder.h"
#include "radar/cell_grid.h"
#include "radar/cluster_analyzer.h"
#include "radar/config.h"
#include "radar/contour_merger.h"
#include "radar/echo_tops.h"
#include "radar/geo_utils.h"
#include "radar/image_renderer.h"

namespace fs = std::filesystem;

using namespace radar;

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: radar_hazard_app <config.json>\n";
        return 1;
    }

    try {
        auto config = ConfigLoader::load_pipeline(argv[1]);
        auto tables = ConfigLoader::load_tables(config.tables_path);

        BufrDecoder decoder(std::move(tables));
        auto messages = decoder.decode_file(config.bufr_input);

        GeoCalculator geo(config.radar_latitude, config.radar_longitude, config.radar_altitude_m);
        EchoTops echo_tops;
        echo_tops.load(config.echo_tops_matrix);

        CellGrid grid;
        fs::create_directories(config.csv_output_dir);
        std::ofstream csv(config.csv_output_dir + "/cells.csv");
        csv << "row,column,reflectivity_dbz,velocity_ms,spectrum_width,echo_top_km,phenomenon,center_lat,center_lon\n";

        for (const auto& message : messages) {
            std::unordered_map<std::string, double> numeric;
            for (const auto& value : message.values) {
                numeric[value.mnemonic] = value.value;
            }
            if (!numeric.count("ROW") || !numeric.count("COLUMN") || !numeric.count("DBZH")) {
                continue;
            }
            CellData cell;
            cell.row = static_cast<int>(numeric["ROW"]);
            cell.column = static_cast<int>(numeric["COLUMN"]);
            cell.reflectivity_dbz = numeric["DBZH"];
            cell.velocity_ms = numeric.count("VRAD") ? numeric["VRAD"] : 0.0;
            cell.spectrum_width = numeric.count("SWRAD") ? numeric["SWRAD"] : 0.0;
            RadarObservation obs{
                .azimuth_deg = numeric.count("AZIMUTH") ? numeric["AZIMUTH"] : 0.0,
                .range_km = numeric.count("RANGE") ? numeric["RANGE"] : 0.0,
                .elevation_deg = numeric.count("ELEVATION") ? numeric["ELEVATION"] : 0.0,
            };
            cell.geometry = geo.compute_geometry(obs, config.grid_cell_size_km);
            cell.echo_top_km = echo_tops.value(cell.row, cell.column);
            if (numeric.count("PHENOMENON")) {
                cell.phenomenon_type = std::to_string(static_cast<int>(numeric["PHENOMENON"]));
            }

            bool allowed = config.allowed_phenomena.empty();
            if (!config.allowed_phenomena.empty()) {
                allowed = std::find(config.allowed_phenomena.begin(), config.allowed_phenomena.end(), cell.phenomenon_type) != config.allowed_phenomena.end();
            }
            if (!allowed) {
                continue;
            }
            double min_threshold = config.reflectivity_thresholds.empty() ? -std::numeric_limits<double>::infinity()
                                                                         : config.reflectivity_thresholds.front();
            if (cell.reflectivity_dbz < min_threshold) {
                continue;
            }

            grid.add_cell(cell);
            csv << cell.row << ',' << cell.column << ',' << cell.reflectivity_dbz << ',' << cell.velocity_ms << ','
                << cell.spectrum_width << ',';
            if (cell.echo_top_km.has_value()) {
                csv << cell.echo_top_km.value();
            }
            csv << ',' << cell.phenomenon_type << ',' << cell.geometry.center.latitude_deg << ','
                << cell.geometry.center.longitude_deg << '\n';
        }

        ClusterAnalyzer analyzer(grid);
        double threshold = config.reflectivity_thresholds.empty() ? 35.0 : config.reflectivity_thresholds.front();
        auto clusters = analyzer.find_clusters(threshold);

        ContourMerger merger;
        auto merged = merger.merge(clusters);
        merger.write_geojson(merged, config.merged_geojson_output);

        if (!config.image_output_path.empty()) {
            if (!fs::path(config.image_output_path).parent_path().empty()) {
                fs::create_directories(fs::path(config.image_output_path).parent_path());
            }
            ImageRenderer renderer(ImageRenderOptions{
                .width = config.image_width,
                .height = config.image_height,
            });
            renderer.render(merged, config.image_output_path);
        }

        std::cout << "Processed " << messages.size() << " BUFR messages" << std::endl;
        std::cout << "Generated " << merged.size() << " merged contours" << std::endl;
        if (!config.image_output_path.empty()) {
            std::cout << "Rendered contour map to " << config.image_output_path << std::endl;
        }
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
