#pragma once

#include <cctype>
#include <map>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>
#include <utility>

namespace radar {

class JsonValue {
public:
    using object_t = std::map<std::string, JsonValue>;
    using array_t = std::vector<JsonValue>;

    JsonValue() = default;
    explicit JsonValue(double value) : data_(value) {}
    explicit JsonValue(std::string value) : data_(std::move(value)) {}
    explicit JsonValue(object_t value) : data_(std::move(value)) {}
    explicit JsonValue(array_t value) : data_(std::move(value)) {}
    explicit JsonValue(bool value) : data_(value) {}

    bool is_object() const { return std::holds_alternative<object_t>(data_); }
    bool is_array() const { return std::holds_alternative<array_t>(data_); }
    bool is_string() const { return std::holds_alternative<std::string>(data_); }
    bool is_number() const { return std::holds_alternative<double>(data_); }
    bool is_bool() const { return std::holds_alternative<bool>(data_); }

    const object_t& as_object() const { return std::get<object_t>(data_); }
    const array_t& as_array() const { return std::get<array_t>(data_); }
    const std::string& as_string() const { return std::get<std::string>(data_); }
    double as_number() const { return std::get<double>(data_); }
    bool as_bool() const { return std::get<bool>(data_); }

    const JsonValue& at(const std::string& key) const {
        const auto& obj = as_object();
        auto it = obj.find(key);
        if (it == obj.end()) {
            throw std::out_of_range("Key not found: " + key);
        }
        return it->second;
    }

private:
    std::variant<std::monostate, double, std::string, object_t, array_t, bool> data_;
};

class JsonParser {
public:
    explicit JsonParser(const std::string& text) : text_(text) {}

    JsonValue parse();

private:
    JsonValue parse_value();
    JsonValue parse_object();
    JsonValue parse_array();
    JsonValue parse_string();
    JsonValue parse_number();

    void skip_ws();
    char peek() const;
    char get();

    const std::string& text_;
    std::size_t pos_ = 0;
};

// Utility functions
inline double json_number(const JsonValue& value) { return value.as_number(); }
inline std::string json_string(const JsonValue& value) { return value.as_string(); }
inline bool json_bool(const JsonValue& value) { return value.as_bool(); }

inline const JsonValue* json_try_get(const JsonValue& obj, const std::string& key) {
    const auto& map = obj.as_object();
    auto it = map.find(key);
    if (it == map.end()) {
        return nullptr;
    }
    return &it->second;
}

}  // namespace radar
