#pragma once

#include <optional>
#include <string>
#include <vector>

namespace radar {

class EchoTops {
public:
    void load(const std::string& path);
    std::optional<double> value(int row, int column) const;

private:
    std::vector<std::vector<double>> data_;
};

}  // namespace radar
