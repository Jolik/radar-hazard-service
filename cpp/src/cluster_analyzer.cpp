#include "radar/cluster_analyzer.h"

#include <algorithm>
#include <queue>

namespace radar {

ClusterAnalyzer::ClusterAnalyzer(const CellGrid& grid) : grid_(grid) {}

std::vector<Cluster> ClusterAnalyzer::find_clusters(double reflectivity_threshold_dbz) const {
    std::set<std::pair<int, int>> visited;
    std::vector<Cluster> clusters;

    for (const auto& cell : grid_.cells()) {
        if (cell.reflectivity_dbz < reflectivity_threshold_dbz) {
            continue;
        }
        auto key = std::make_pair(cell.row, cell.column);
        if (visited.count(key)) {
            continue;
        }
        Cluster cluster;
        visit(cell.row, cell.column, reflectivity_threshold_dbz, visited, cluster);
        if (!cluster.cells.empty()) {
            clusters.push_back(std::move(cluster));
        }
    }
    return clusters;
}

void ClusterAnalyzer::visit(int row, int column, double threshold, std::set<std::pair<int, int>>& visited, Cluster& cluster) const {
    std::queue<std::pair<int, int>> queue;
    queue.emplace(row, column);
    visited.insert({row, column});

    while (!queue.empty()) {
        auto [current_row, current_column] = queue.front();
        queue.pop();

        auto cell_opt = grid_.find(current_row, current_column);
        if (!cell_opt.has_value()) {
            continue;
        }
        auto cell = cell_opt.value();
        if (cell.reflectivity_dbz < threshold) {
            continue;
        }
        cluster.max_reflectivity = std::max(cluster.max_reflectivity, cell.reflectivity_dbz);
        if (cell.echo_top_km.has_value()) {
            cluster.max_echo_top_km = std::max(cluster.max_echo_top_km.value_or(0.0), cell.echo_top_km.value());
        }
        cluster.cells.push_back(cell);

        for (const auto& neighbor : neighbors(current_row, current_column)) {
            if (!visited.count(neighbor)) {
                visited.insert(neighbor);
                queue.push(neighbor);
            }
        }
    }
}

std::vector<std::pair<int, int>> ClusterAnalyzer::neighbors(int row, int column) const {
    std::vector<std::pair<int, int>> result;
    for (int dr = -1; dr <= 1; ++dr) {
        for (int dc = -1; dc <= 1; ++dc) {
            if (dr == 0 && dc == 0) {
                continue;
            }
            result.emplace_back(row + dr, column + dc);
        }
    }
    return result;
}

}  // namespace radar
