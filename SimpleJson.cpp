#include "SimpleJson.h"

#include <cctype>

namespace
{
    struct Parser
    {
        const char* current = nullptr;
        const char* end = nullptr;
        std::string* error = nullptr;

        void SkipWhitespace()
        {
            while (current < end && std::isspace(static_cast<unsigned char>(*current))) {
                ++current;
            }
        }

        bool Match(char c)
        {
            SkipWhitespace();
            if (current < end && *current == c) {
                ++current;
                return true;
            }
            return false;
        }

        bool ParseValue(JsonValue& out)
        {
            SkipWhitespace();
            if (current >= end) {
                return false;
            }
            switch (*current) {
            case '{': return ParseObject(out);
            case '[': return ParseArray(out);
            case '"': return ParseString(out);
            case 't': return ParseLiteral("true", JsonValue::Type::Bool, out, true);
            case 'f': return ParseLiteral("false", JsonValue::Type::Bool, out, false);
            case 'n': return ParseLiteral("null", JsonValue::Type::Null, out, false);
            default:
                if (*current == '-' || std::isdigit(static_cast<unsigned char>(*current))) {
                    return ParseNumber(out);
                }
                return false;
            }
        }

        bool ParseLiteral(const char* literal, JsonValue::Type type, JsonValue& out, bool boolValue)
        {
            const char* start = current;
            while (*literal) {
                if (current >= end || *current != *literal) {
                    current = start;
                    return false;
                }
                ++current;
                ++literal;
            }
            out.type = type;
            out.boolean = boolValue;
            return true;
        }

        bool ParseString(JsonValue& out)
        {
            if (!Match('"')) {
                return false;
            }
            std::string result;
            while (current < end) {
                char c = *current++;
                if (c == '"') {
                    out.type = JsonValue::Type::String;
                    out.string = result;
                    return true;
                }
                if (c == '\\') {
                    if (current >= end) {
                        return false;
                    }
                    char esc = *current++;
                    switch (esc) {
                    case '"': result.push_back('"'); break;
                    case '\\': result.push_back('\\'); break;
                    case '/': result.push_back('/'); break;
                    case 'b': result.push_back('\b'); break;
                    case 'f': result.push_back('\f'); break;
                    case 'n': result.push_back('\n'); break;
                    case 'r': result.push_back('\r'); break;
                    case 't': result.push_back('\t'); break;
                    default: return false;
                    }
                    continue;
                }
                result.push_back(c);
            }
            return false;
        }

        bool ParseNumber(JsonValue& out)
        {
            const char* start = current;
            if (*current == '-') {
                ++current;
            }
            while (current < end && std::isdigit(static_cast<unsigned char>(*current))) {
                ++current;
            }
            if (current < end && *current == '.') {
                ++current;
                while (current < end && std::isdigit(static_cast<unsigned char>(*current))) {
                    ++current;
                }
            }
            std::string number(start, current);
            try {
                out.type = JsonValue::Type::Number;
                out.number = std::stod(number);
                return true;
            }
            catch (...) {
                return false;
            }
        }

        bool ParseArray(JsonValue& out)
        {
            if (!Match('[')) {
                return false;
            }
            out.type = JsonValue::Type::Array;
            out.array.clear();
            SkipWhitespace();
            if (Match(']')) {
                return true;
            }
            while (current < end) {
                JsonValue value;
                if (!ParseValue(value)) {
                    return false;
                }
                out.array.push_back(std::move(value));
                SkipWhitespace();
                if (Match(']')) {
                    return true;
                }
                if (!Match(',')) {
                    return false;
                }
            }
            return false;
        }

        bool ParseObject(JsonValue& out)
        {
            if (!Match('{')) {
                return false;
            }
            out.type = JsonValue::Type::Object;
            out.object.clear();
            SkipWhitespace();
            if (Match('}')) {
                return true;
            }
            while (current < end) {
                JsonValue key;
                if (!ParseString(key)) {
                    return false;
                }
                if (!Match(':')) {
                    return false;
                }
                JsonValue value;
                if (!ParseValue(value)) {
                    return false;
                }
                out.object.emplace(key.string, std::move(value));
                SkipWhitespace();
                if (Match('}')) {
                    return true;
                }
                if (!Match(',')) {
                    return false;
                }
            }
            return false;
        }
    };
}

const JsonValue* JsonValue::Find(const std::string& key) const
{
    if (type != Type::Object) {
        return nullptr;
    }
    auto it = object.find(key);
    if (it == object.end()) {
        return nullptr;
    }
    return &it->second;
}

bool ParseJson(const std::string& text, JsonValue& value, std::string* error)
{
    Parser parser;
    parser.current = text.c_str();
    parser.end = text.c_str() + text.size();
    parser.error = error;
    if (!parser.ParseValue(value)) {
        if (error) {
            *error = "JSON parse error";
        }
        return false;
    }
    parser.SkipWhitespace();
    if (parser.current != parser.end) {
        if (error) {
            *error = "Extra trailing data";
        }
        return false;
    }
    return true;
}
