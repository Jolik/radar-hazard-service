#include "radar/cell_grid.h"

namespace radar {

void CellGrid::add_cell(CellData cell) {
    std::pair<int, int> key{cell.row, cell.column};
    auto it = index_.find(key);
    if (it == index_.end()) {
        index_[key] = cells_.size();
        cells_.push_back(std::move(cell));
    } else {
        cells_[it->second] = std::move(cell);
    }
}

std::optional<CellData> CellGrid::find(int row, int column) const {
    auto it = index_.find({row, column});
    if (it == index_.end()) {
        return std::nullopt;
    }
    return cells_[it->second];
}

}  // namespace radar
