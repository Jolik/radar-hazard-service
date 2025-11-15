#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "radar/config.h"

namespace radar {

struct BufrValue {
    std::string mnemonic;
    double value = 0.0;
    std::string unit;
};

struct BufrMessage {
    std::vector<BufrValue> values;
};

class BufrDecoder {
public:
    explicit BufrDecoder(std::unordered_map<std::string, DescriptorDefinition> tables);

    std::vector<BufrMessage> decode_file(const std::filesystem::path& path) const;

private:
    struct Descriptor {
        int f = 0;
        int x = 0;
        int y = 0;
    };

    struct BitReader {
        std::vector<std::uint8_t> buffer;
        mutable std::size_t bit_pos = 0;

        explicit BitReader(std::vector<std::uint8_t> data) : buffer(std::move(data)) {}

        std::uint32_t read_bits(std::size_t bit_count) const;
        void reset() const { bit_pos = 0; }
    };

    DescriptorDefinition resolve(const Descriptor& descriptor) const;
    std::vector<Descriptor> parse_section3(const std::vector<std::uint8_t>& section) const;
    std::vector<BufrValue> decode_data(const BitReader& reader, const std::vector<Descriptor>& descriptors) const;

    std::unordered_map<std::string, DescriptorDefinition> tables_;
};

}  // namespace radar
