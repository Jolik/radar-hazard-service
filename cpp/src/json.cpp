#include "radar/json.h"

namespace radar {

JsonValue JsonParser::parse() {
    skip_ws();
    auto value = parse_value();
    skip_ws();
    if (pos_ != text_.size()) {
        throw std::runtime_error("Unexpected characters at end of JSON string");
    }
    return value;
}

JsonValue JsonParser::parse_value() {
    skip_ws();
    if (pos_ >= text_.size()) {
        throw std::runtime_error("Unexpected end of JSON");
    }
    char c = peek();
    if (c == '"') {
        return parse_string();
    }
    if (c == '{') {
        return parse_object();
    }
    if (c == '[') {
        return parse_array();
    }
    if (c == 't') {
        if (text_.compare(pos_, 4, "true") != 0) {
            throw std::runtime_error("Invalid token: expected true");
        }
        pos_ += 4;
        return JsonValue(true);
    }
    if (c == 'f') {
        if (text_.compare(pos_, 5, "false") != 0) {
            throw std::runtime_error("Invalid token: expected false");
        }
        pos_ += 5;
        return JsonValue(false);
    }
    if (c == 'n') {
        if (text_.compare(pos_, 4, "null") != 0) {
            throw std::runtime_error("Invalid token: expected null");
        }
        pos_ += 4;
        return JsonValue();
    }
    if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) {
        return parse_number();
    }
    throw std::runtime_error("Unexpected token in JSON");
}

JsonValue JsonParser::parse_object() {
    if (get() != '{') {
        throw std::runtime_error("Expected '{'");
    }
    JsonValue::object_t object;
    skip_ws();
    if (peek() == '}') {
        get();
        return JsonValue(std::move(object));
    }
    while (true) {
        skip_ws();
        auto key = parse_string().as_string();
        skip_ws();
        if (get() != ':') {
            throw std::runtime_error("Expected ':' in object");
        }
        skip_ws();
        object.emplace(std::move(key), parse_value());
        skip_ws();
        char next = get();
        if (next == '}') {
            break;
        }
        if (next != ',') {
            throw std::runtime_error("Expected ',' in object");
        }
    }
    return JsonValue(std::move(object));
}

JsonValue JsonParser::parse_array() {
    if (get() != '[') {
        throw std::runtime_error("Expected '['");
    }
    JsonValue::array_t array;
    skip_ws();
    if (peek() == ']') {
        get();
        return JsonValue(std::move(array));
    }
    while (true) {
        array.push_back(parse_value());
        skip_ws();
        char next = get();
        if (next == ']') {
            break;
        }
        if (next != ',') {
            throw std::runtime_error("Expected ',' in array");
        }
    }
    return JsonValue(std::move(array));
}

JsonValue JsonParser::parse_string() {
    if (get() != '"') {
        throw std::runtime_error("Expected '\"' to start string");
    }
    std::string result;
    while (pos_ < text_.size()) {
        char c = get();
        if (c == '"') {
            break;
        }
        if (c == '\\') {
            if (pos_ >= text_.size()) {
                throw std::runtime_error("Invalid escape sequence");
            }
            char esc = get();
            switch (esc) {
                case '"': result.push_back('"'); break;
                case '\\': result.push_back('\\'); break;
                case '/': result.push_back('/'); break;
                case 'b': result.push_back('\b'); break;
                case 'f': result.push_back('\f'); break;
                case 'n': result.push_back('\n'); break;
                case 'r': result.push_back('\r'); break;
                case 't': result.push_back('\t'); break;
                default:
                    throw std::runtime_error("Unsupported escape sequence in string");
            }
        } else {
            result.push_back(c);
        }
    }
    return JsonValue(std::move(result));
}

JsonValue JsonParser::parse_number() {
    std::size_t start = pos_;
    if (peek() == '-') {
        ++pos_;
    }
    while (pos_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[pos_]))) {
        ++pos_;
    }
    if (pos_ < text_.size() && text_[pos_] == '.') {
        ++pos_;
        while (pos_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[pos_]))) {
            ++pos_;
        }
    }
    if (pos_ < text_.size() && (text_[pos_] == 'e' || text_[pos_] == 'E')) {
        ++pos_;
        if (pos_ < text_.size() && (text_[pos_] == '+' || text_[pos_] == '-')) {
            ++pos_;
        }
        while (pos_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[pos_]))) {
            ++pos_;
        }
    }
    double value = std::stod(text_.substr(start, pos_ - start));
    return JsonValue(value);
}

void JsonParser::skip_ws() {
    while (pos_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[pos_]))) {
        ++pos_;
    }
}

char JsonParser::peek() const {
    if (pos_ >= text_.size()) {
        throw std::runtime_error("Unexpected end of JSON");
    }
    return text_[pos_];
}

char JsonParser::get() {
    if (pos_ >= text_.size()) {
        throw std::runtime_error("Unexpected end of JSON");
    }
    return text_[pos_++];
}

}  // namespace radar
