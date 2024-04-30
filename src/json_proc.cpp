/**
 * Copyright (c) 2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#include "json_proc.hpp"

#include "data_accessor.hpp"
#include "printing_util.hpp"

namespace json_proc
{

// JsonPattern ////////////////////////////////////////////////////////////////

static void evalJson(const device_id::PatternIndex& index,
                     const nlohmann::json& jsonPattern, nlohmann::json& result)
{
    if (jsonPattern.is_string())
    {
        auto str = jsonPattern.get<std::string>();
        try
        {
            result = device_id::DeviceIdPattern(str).eval(index);
        }
        catch (const std::exception& e)
        {
            // marcinw:TODO: Add information about path
            // marcinw:TODO: get rid of stringstream
            std::stringstream ss;
            ss << index;
            throw std::runtime_error("Error when evaluating pattern '" + str +
                                     "' at " + ss.str() + ": " + e.what());
        }
    }
    else if (jsonPattern.is_object())
    {
        for (const auto& [key, value] : jsonPattern.items())
        {
            evalJson(index, jsonPattern[key], result[key]);
        }
    }
    else if (jsonPattern.is_array())
    {
        for (unsigned i = 0; i < jsonPattern.size(); ++i)
        {
            evalJson(index, jsonPattern[i], result[i]);
        }
    }
    else
    {
        result = jsonPattern;
    }
}

nlohmann::json JsonPattern::eval(const device_id::PatternIndex& index) const
{
    nlohmann::json result = this->js;
    evalJson(index, this->js, result);
    return result;
}

const nlohmann::json& JsonPattern::getJson() const
{
    return js;
}

std::string getTypeAsString(const nlohmann::json& json)
{
    switch (json.type())
    {
        case json::value_t::null:
            return "null";
        case json::value_t::boolean:
            return "boolean";
        case json::value_t::number_integer:
            return "number_integer";
        case json::value_t::number_unsigned:
            return "number_unsigned";
        case json::value_t::number_float:
            return "number_float";
        case json::value_t::object:
            return "object";
        case json::value_t::array:
            return "array";
        case json::value_t::string:
            return "string";
        default:
            return "(unknown)";
    }
}

std::string to_string(const JsonPathElement& coordinate)
{
    const std::string* stringCoordinatePtr =
        std::get_if<std::string>(&coordinate);
    if (stringCoordinatePtr != nullptr)
    {
        return *stringCoordinatePtr;
    }
    else
    {
        const unsigned* numericCoordinatePtr =
            std::get_if<unsigned>(&coordinate);
        if (numericCoordinatePtr != nullptr)
        {
            // marcinw:TODO: account for spaces and then put in quotes
            return "[" + std::to_string(*numericCoordinatePtr) + "]";
        }
        else
        {
            throw std::runtime_error("Unrecognized JsonPathElement type");
        }
    }
}

std::string to_string(const JsonPath& jsonPath)
{
    std::string result;
    if (jsonPath.empty())
    {
        result = ".";
    }
    else
    {
        for (const auto& elem : jsonPath)
        {
            result += "." + to_string(elem);
        }
    }
    return result;
}

std::string to_string(const JsonFormatProblem& formatProblem)
{
    return to_string(formatProblem.first) + ": " +
           to_string(formatProblem.second);
}

JsonPath getPath(const JsonFormatProblem& formatProblem)
{
    return formatProblem.first;
}

std::string getMessage(const JsonFormatProblem& formatProblem)
{
    return formatProblem.second;
}

JsonPath dropFirst(const JsonPath& jsPath)
{
    JsonPath result = jsPath;
    result.pop_front();
    return result;
}

const nlohmann::json* optionalNodeAt(const nlohmann::json* const js,
                                     const JsonPath& jsPath)
{
    if (jsPath.empty())
    {
        return js;
    }
    else
    {
        auto coordinate = jsPath.front();
        if (js->is_object() && std::holds_alternative<std::string>(coordinate))
        {
            auto key = std::get<std::string>(coordinate);
            if (js->contains(key))
            {
                return optionalNodeAt(&js->at(key), dropFirst(jsPath));
            }
            else
            {
                return nullptr;
            }
        }
        else if (js->is_array() && std::holds_alternative<unsigned>(coordinate))
        {
            auto index = std::get<unsigned>(coordinate);
            if (index < js->size())
            {
                return optionalNodeAt(&js->at(index), dropFirst(jsPath));
            }
            else
            {
                return nullptr;
            }
        }
        else
        {
            return nullptr;
        }
    }
}

bool contains(const nlohmann::json& js, const JsonPath& jsPath)
{
    return optionalNodeAt(&js, jsPath) != nullptr;
}

const nlohmann::json& nodeAt(const nlohmann::json& js, const JsonPath& jsPath)
{
    auto res = optionalNodeAt(&js, jsPath);
    if (res != nullptr)
    {
        return *res;
    }
    else
    {
        throw std::runtime_error("Path '" + to_string(jsPath) +
                                 "' doesn't exist in the given json");
    }
}

} // namespace json_proc
