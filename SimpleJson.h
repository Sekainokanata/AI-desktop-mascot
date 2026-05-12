#pragma once

#include <string>
#include <unordered_map>
#include <vector>

struct JsonValue
{
    enum class Type
    {
        Null,
        Number,
        String,
        Array,
        Object,
        Bool
    };

    Type type = Type::Null;
    double number = 0.0;
    bool boolean = false;
    std::string string;
    std::vector<JsonValue> array;
    std::unordered_map<std::string, JsonValue> object;

    bool IsObject() const { return type == Type::Object; }
    bool IsArray() const { return type == Type::Array; }
    bool IsString() const { return type == Type::String; }
    bool IsNumber() const { return type == Type::Number; }
    bool IsBool() const { return type == Type::Bool; }

    const JsonValue* Find(const std::string& key) const;
};

bool ParseJson(const std::string& text, JsonValue& value, std::string* error = nullptr);
