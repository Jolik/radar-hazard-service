#include "radar/echo_tops.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace radar {

void EchoTops::load(const std::string& path) {
    std::ifstream stream(path);
    if (!stream.is_open()) {
        throw std::runtime_error("Cannot open echo tops matrix: " + path);
    }
    data_.clear();
    std::string line;
    while (std::getline(stream, line)) {
        std::stringstream ss(line);
        std::vector<double> row;
        double value;
        while (ss >> value) {
            row.push_back(value);
            if (ss.peek() == ',' || ss.peek() == ';') {
                ss.ignore();
            }
        }
        if (!row.empty()) {
            data_.push_back(std::move(row));
        }
    }
}

std::optional<double> EchoTops::value(int row, int column) const {
    if (row < 0 || column < 0) {
        return std::nullopt;
    }
    if (static_cast<std::size_t>(row) >= data_.size()) {
        return std::nullopt;
    }
    const auto& row_data = data_[static_cast<std::size_t>(row)];
    if (static_cast<std::size_t>(column) >= row_data.size()) {
        return std::nullopt;
    }
    return row_data[static_cast<std::size_t>(column)];
}

}  // namespace radar
