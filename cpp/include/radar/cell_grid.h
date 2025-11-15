#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "radar/geo_utils.h"

namespace radar {

struct CellData {
    int row = 0;
    int column = 0;
    double reflectivity_dbz = 0.0;
    double velocity_ms = 0.0;
    double spectrum_width = 0.0;
    CellGeometry geometry;
    std::optional<double> echo_top_km;
    std::string phenomenon_type;
};

class CellGrid {
public:
    void add_cell(CellData cell);
    const std::vector<CellData>& cells() const { return cells_; }
    std::optional<CellData> find(int row, int column) const;

private:
    std::vector<CellData> cells_;
    std::map<std::pair<int, int>, std::size_t> index_;
};

}  // namespace radar
