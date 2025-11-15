#include "radar/bufr_decoder.h"

#include <cmath>
#include <cstdio>
#include <fstream>
#include <stdexcept>

namespace radar {

namespace {
constexpr std::size_t kSectionHeaderSize = 3;

std::uint16_t read_uint16(const std::vector<std::uint8_t>& data, std::size_t offset) {
    return static_cast<std::uint16_t>((data[offset] << 8) | data[offset + 1]);
}

std::uint32_t read_uint24(const std::vector<std::uint8_t>& data, std::size_t offset) {
    return static_cast<std::uint32_t>((data[offset] << 16) | (data[offset + 1] << 8) | data[offset + 2]);
}

std::uint32_t read_uint32(const std::vector<std::uint8_t>& data, std::size_t offset) {
    return static_cast<std::uint32_t>((data[offset] << 24) | (data[offset + 1] << 16) |
                                      (data[offset + 2] << 8) | data[offset + 3]);
}

std::vector<std::uint8_t> read_section(std::ifstream& stream, std::size_t size) {
    std::vector<std::uint8_t> data(size);
    stream.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(size));
    if (stream.gcount() != static_cast<std::streamsize>(size)) {
        throw std::runtime_error("Unexpected EOF while reading BUFR section");
    }
    return data;
}

}  // namespace

BufrDecoder::BufrDecoder(std::unordered_map<std::string, DescriptorDefinition> tables)
    : tables_(std::move(tables)) {}

std::vector<BufrMessage> BufrDecoder::decode_file(const std::filesystem::path& path) const {
    std::ifstream stream(path, std::ios::binary);
    if (!stream.is_open()) {
        throw std::runtime_error("Cannot open BUFR file: " + path.string());
    }

    std::vector<BufrMessage> messages;
    while (stream) {
        char header[4];
        stream.read(header, 4);
        if (!stream) {
            break;
        }
        if (std::string(header, 4) != "BUFR") {
            throw std::runtime_error("Invalid BUFR start signature");
        }

        // Section 0 already consumed (length 8). Skip rest of section 0.
        stream.seekg(4, std::ios::cur);

        // Section 1 length and data
        auto section1_length = read_uint24(read_section(stream, kSectionHeaderSize), 0);
        auto section1 = read_section(stream, section1_length - kSectionHeaderSize);
        (void)section1;  // Not used explicitly but read to advance stream.

        // Section 2 optional: peek next 3 bytes to check length
        auto header2 = read_section(stream, kSectionHeaderSize);
        std::vector<std::uint8_t> section2;
        if (header2[0] != 0 || header2[1] != 0 || header2[2] != 0) {
            auto len2 = read_uint24(header2, 0);
            section2 = read_section(stream, len2 - kSectionHeaderSize);
            header2 = read_section(stream, kSectionHeaderSize);
        }
        (void)section2;

        auto len3 = read_uint24(header2, 0);
        auto section3 = read_section(stream, len3 - kSectionHeaderSize);

        auto header4 = read_section(stream, kSectionHeaderSize);
        auto len4 = read_uint24(header4, 0);
        auto section4 = read_section(stream, len4 - kSectionHeaderSize);

        // Section 5 (7777)
        char trailer[4];
        stream.read(trailer, 4);
        if (std::string(trailer, 4) != "7777") {
            throw std::runtime_error("Invalid BUFR end signature");
        }

        auto descriptors = parse_section3(section3);
        BitReader reader(section4);
        auto values = decode_data(reader, descriptors);
        messages.push_back(BufrMessage{std::move(values)});
    }

    return messages;
}

std::uint32_t BufrDecoder::BitReader::read_bits(std::size_t bit_count) const {
    std::uint32_t value = 0;
    for (std::size_t i = 0; i < bit_count; ++i) {
        if (bit_pos >= buffer.size() * 8) {
            throw std::runtime_error("Attempt to read past end of BUFR bitstream");
        }
        std::size_t byte_index = bit_pos / 8;
        std::size_t bit_index = 7 - (bit_pos % 8);
        std::uint8_t bit = (buffer[byte_index] >> bit_index) & 1;
        value = (value << 1) | bit;
        ++bit_pos;
    }
    return value;
}

DescriptorDefinition BufrDecoder::resolve(const Descriptor& descriptor) const {
    char key[16];
    std::snprintf(key, sizeof(key), "%d-%03d-%03d", descriptor.f, descriptor.x, descriptor.y);
    auto it = tables_.find(key);
    if (it == tables_.end()) {
        throw std::runtime_error("Descriptor not found in tables: " + std::string(key));
    }
    return it->second;
}

std::vector<BufrDecoder::Descriptor> BufrDecoder::parse_section3(const std::vector<std::uint8_t>& section) const {
    if (section.size() < 1) {
        throw std::runtime_error("Section 3 is too small");
    }
    std::size_t pos = 1;  // Skip reserved byte after flags.
    std::vector<Descriptor> descriptors;
    while (pos + 1 < section.size()) {
        Descriptor descriptor;
        descriptor.f = (section[pos] & 0b11000000) >> 6;
        descriptor.x = section[pos] & 0b00111111;
        descriptor.y = section[pos + 1];
        descriptors.push_back(descriptor);
        pos += 2;
    }
    return descriptors;
}

std::vector<BufrValue> BufrDecoder::decode_data(const BitReader& reader, const std::vector<Descriptor>& descriptors) const {
    std::vector<BufrValue> values;
    for (const auto& descriptor : descriptors) {
        auto def = resolve(descriptor);
        if (def.bits == 0) {
            continue;
        }
        auto raw = reader.read_bits(static_cast<std::size_t>(def.bits));
        if (raw == static_cast<std::uint32_t>((1u << def.bits) - 1)) {
            continue;  // Missing value
        }
        double value = (static_cast<double>(raw) + def.reference) / std::pow(10.0, def.scale);
        values.push_back(BufrValue{def.mnemonic, value, def.unit});
    }
    return values;
}

}  // namespace radar
