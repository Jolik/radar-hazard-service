#pragma once

#include <map>
#include <set>
#include <vector>

#include "radar/cell_grid.h"

namespace radar {

struct Cluster {
    std::vector<CellData> cells;
    double max_reflectivity = 0.0;
    std::optional<double> max_echo_top_km;
};

class ClusterAnalyzer {
public:
    explicit ClusterAnalyzer(const CellGrid& grid);

    std::vector<Cluster> find_clusters(double reflectivity_threshold_dbz) const;

private:
    void visit(int row, int column, double threshold, std::set<std::pair<int, int>>& visited, Cluster& cluster) const;
    std::vector<std::pair<int, int>> neighbors(int row, int column) const;

    const CellGrid& grid_;
};

}  // namespace radar
