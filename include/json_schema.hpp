/**
 * Copyright (c) 2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#pragma once
#include "json_proc.hpp"

#include <boost/algorithm/string/join.hpp>
#include <nlohmann/json.hpp>

#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <variant>

namespace json_schema
{

// Utils //////////////////////////////////////////////////////////////////////

std::string singleQuote(const std::string& sth);

template <typename T>
std::string
    setToString(const std::vector<T>& elements,
                const std::function<std::string(const T& elem)>& transform)
{
    std::vector<std::string> output;
    std::transform(elements.begin(), elements.end(), std::back_inserter(output),
                   transform);
    return std::string("{") + boost::algorithm::join(output, ", ") + "}";
}

// JsonSchema /////////////////////////////////////////////////////////////////

class JsonSchema
{
  public:
    virtual ~JsonSchema() = default;

    bool check(const nlohmann::json& element,
               json_proc::ProblemsCollector& problemsCollector)
    {
        return rawCheck(element, nullptr, problemsCollector);
    }

    // these two should not be public, but limiting their visibility causes
    // problems in the library which I have no time to fix now
    bool rawCheckDescend(const nlohmann::json& element,
                         const json_proc::StackJsonPath elementPath,
                         json_proc::ProblemsCollector& problemsCollector)
    {
        return rawCheck(element, &elementPath, problemsCollector);
    }

    virtual bool rawCheck(const nlohmann::json& element,
                          const json_proc::StackJsonPath* const elementPath,
                          json_proc::ProblemsCollector& problemsCollector) = 0;
};

class JsonFeatureChecker : public JsonSchema
{
  public:
    virtual bool forType(nlohmann::json::value_t elemType) = 0;
};

// JsonLiteralSchema //////////////////////////////////////////////////////////

class JsonLiteralSchema : public JsonSchema
{
    bool value;

    JsonLiteralSchema(bool value) : value(value)
    {}

  public:
    bool rawCheck(const nlohmann::json& element,
                  const json_proc::StackJsonPath* const elementPath,
                  json_proc::ProblemsCollector& problemsCollector);

    static std::shared_ptr<JsonLiteralSchema> create(bool value)
    {
        return std::shared_ptr<JsonLiteralSchema>(new JsonLiteralSchema(value));
    }
};

std::shared_ptr<JsonSchema> literal(bool value);

// JsonFeaturesCheckerSchema //////////////////////////////////////////////////

class JsonFeaturesCheckerSchema : public JsonSchema
{
    std::vector<std::shared_ptr<JsonFeatureChecker>> checkers;

    template <typename... Args>
    JsonFeaturesCheckerSchema(Args... values) : checkers{values...}
    {}

  public:
    bool rawCheck(const nlohmann::json& element,
                  const json_proc::StackJsonPath* const elementPath,
                  json_proc::ProblemsCollector& problemsCollector);

    template <typename... Args>
    static std::shared_ptr<JsonFeaturesCheckerSchema> create(Args... values)
    {
        return std::shared_ptr<JsonFeaturesCheckerSchema>(
            new JsonFeaturesCheckerSchema(values...));
    }
};

template <typename... Args>
std::shared_ptr<JsonSchema> schema(Args... values)
{
    return JsonFeaturesCheckerSchema::create(values...);
}

// JsonTypeChecker ////////////////////////////////////////////////////////////

class JsonTypeChecker : public JsonFeatureChecker
{
    std::vector<nlohmann::json::value_t> expectedTypes;

    template <typename... Args>
    JsonTypeChecker(Args... args) : expectedTypes{args...}
    {}

  public:
    bool rawCheck(const nlohmann::json& element,
                  const json_proc::StackJsonPath* const elementPath,
                  json_proc::ProblemsCollector& problemsCollector);

    bool forType([[maybe_unused]] nlohmann::json::value_t elemType)
    {
        return true;
    }

    template <typename... Args>
    static std::shared_ptr<JsonTypeChecker> create(Args... args)
    {
        return std::shared_ptr<JsonTypeChecker>(new JsonTypeChecker(args...));
    }
};

template <typename... Args>
std::shared_ptr<JsonFeatureChecker> types(Args... expectedTypes)
{
    return JsonTypeChecker::create(expectedTypes...);
}

// JsonPropertiesCheck ////////////////////////////////////////////////////////

class JsonPropertiesCheck : public JsonFeatureChecker
{
    std::shared_ptr<JsonSchema> additionalPropertiesSchema;
    std::map<std::string, std::shared_ptr<JsonSchema>> propertiesCheckers;

    template <typename... Args>
    JsonPropertiesCheck(std::shared_ptr<JsonSchema> additionalPropertiesSchema,
                        Args... args) :
        additionalPropertiesSchema(additionalPropertiesSchema),
        propertiesCheckers{args...}
    {}

  public:
    bool rawCheck(const nlohmann::json& element,
                  const json_proc::StackJsonPath* const elementPath,
                  json_proc::ProblemsCollector& problemsCollector);

    bool forType(nlohmann::json::value_t elemType)
    {
        return elemType == nlohmann::json::value_t::object;
    }

    template <typename... Args>
    static std::shared_ptr<JsonPropertiesCheck>
        create(std::shared_ptr<JsonSchema> additionalPropertiesSchema,
               Args... args)
    {
        return std::shared_ptr<JsonPropertiesCheck>(
            new JsonPropertiesCheck(additionalPropertiesSchema, args...));
    }
};

std::pair<std::string, std::shared_ptr<JsonSchema>>
    property(std::string name, std::shared_ptr<JsonSchema> checker);

template <typename... Args>
std::shared_ptr<JsonFeatureChecker>
    properties(std::shared_ptr<JsonSchema> additionalPropertiesSchema,
               Args... args)
{
    return JsonPropertiesCheck::create(additionalPropertiesSchema, args...);
}

// JsonItemsCheck /////////////////////////////////////////////////////////////

class JsonItemsCheck : public JsonFeatureChecker
{
    std::shared_ptr<JsonSchema> elementSchema;

    JsonItemsCheck(std::shared_ptr<JsonSchema> elementSchema) :
        elementSchema(elementSchema)
    {}

  public:
    bool rawCheck(const nlohmann::json& element,
                  const json_proc::StackJsonPath* const elementPath,
                  json_proc::ProblemsCollector& problemsCollector);

    bool forType(nlohmann::json::value_t elemType)
    {
        return elemType == nlohmann::json::value_t::array;
    }

    static std::shared_ptr<JsonItemsCheck>
        create(std::shared_ptr<JsonSchema> elementSchema)
    {
        return std::shared_ptr<JsonItemsCheck>(
            new JsonItemsCheck(elementSchema));
    }
};

std::shared_ptr<JsonFeatureChecker>
    items(std::shared_ptr<JsonSchema> elementSchema);

// JsonEnumCheck //////////////////////////////////////////////////////////////

class JsonEnumCheck : public JsonFeatureChecker
{
    std::vector<nlohmann::json> allowedValues;

    template <typename... Args>
    JsonEnumCheck(const Args&... args) : allowedValues{args...}
    {}

  public:
    bool rawCheck(const nlohmann::json& element,
                  const json_proc::StackJsonPath* const elementPath,
                  json_proc::ProblemsCollector& problemsCollector);

    bool forType([[maybe_unused]] nlohmann::json::value_t elemType)
    {
        return true;
    }

    template <typename... Args>
    static std::shared_ptr<JsonEnumCheck> create(const Args&... args)
    {
        return std::shared_ptr<JsonEnumCheck>(new JsonEnumCheck(args...));
    }
};

template <typename... Args>
std::shared_ptr<JsonFeatureChecker> values(const Args&... args)
{
    return JsonEnumCheck::create(args...);
}

// JsonRequiredPropertiesCheck ////////////////////////////////////////////////

class JsonRequiredPropertiesCheck : public JsonFeatureChecker
{
    std::vector<std::string> requiredProperties;

    template <typename... Args>
    JsonRequiredPropertiesCheck(const Args&... args) :
        requiredProperties{args...}
    {}

  public:
    bool rawCheck(const nlohmann::json& element,
                  const json_proc::StackJsonPath* const elementPath,
                  json_proc::ProblemsCollector& problemsCollector);

    bool forType(nlohmann::json::value_t elemType)
    {
        return elemType == nlohmann::json::value_t::object;
    }

    template <typename... Args>
    static std::shared_ptr<JsonRequiredPropertiesCheck>
        create(const Args&... args)
    {
        return std::shared_ptr<JsonRequiredPropertiesCheck>(
            new JsonRequiredPropertiesCheck(args...));
    }
};

template <typename... Args>
std::shared_ptr<JsonFeatureChecker> requiredProperties(const Args&... args)
{
    return JsonRequiredPropertiesCheck::create(args...);
}

// JsonBoundCheck /////////////////////////////////////////////////////////////

class JsonBoundCheck : public JsonFeatureChecker
{
  protected:
    double bound;

    JsonBoundCheck(const double& bound) : bound(bound)
    {}

    std::string boundCheckErrorMsg(const nlohmann::json& element,
                                   const std::string& relation);

  public:
    bool forType(nlohmann::json::value_t elemType)
    {
        return elemType == nlohmann::json::value_t::number_integer ||
               elemType == nlohmann::json::value_t::number_unsigned ||
               elemType == nlohmann::json::value_t::number_float;
    }
};

// JsonLowerBoundCheck ////////////////////////////////////////////////////////

class JsonLowerBoundCheck : public JsonBoundCheck
{
    JsonLowerBoundCheck(const double& bound) : JsonBoundCheck(bound)
    {}

  public:
    bool rawCheck(const nlohmann::json& element,
                  const json_proc::StackJsonPath* const elementPath,
                  json_proc::ProblemsCollector& problemsCollector);

    static std::shared_ptr<JsonLowerBoundCheck> create(const double& arg)
    {
        return std::shared_ptr<JsonLowerBoundCheck>(
            new JsonLowerBoundCheck(arg));
    }
};

std::shared_ptr<JsonFeatureChecker> minimum(const double& arg);

// JsonUpperBoundCheck ////////////////////////////////////////////////////////

class JsonUpperBoundCheck : public JsonBoundCheck
{
    JsonUpperBoundCheck(const double& bound) : JsonBoundCheck(bound)
    {}

  public:
    bool rawCheck(const nlohmann::json& element,
                  const json_proc::StackJsonPath* const elementPath,
                  json_proc::ProblemsCollector& problemsCollector);

    static std::shared_ptr<JsonUpperBoundCheck> create(const double& arg)
    {
        return std::shared_ptr<JsonUpperBoundCheck>(
            new JsonUpperBoundCheck(arg));
    }
};

std::shared_ptr<JsonFeatureChecker> maximum(const double& arg);

// JsonAnyOfCheck /////////////////////////////////////////////////////////////

class JsonAnyOfCheck : public JsonFeatureChecker
{
    std::vector<std::shared_ptr<JsonSchema>> schemas;

    template <typename... Args>
    JsonAnyOfCheck(const Args&... args) : schemas{args...}
    {}

  public:
    bool rawCheck(const nlohmann::json& element,
                  const json_proc::StackJsonPath* const elementPath,
                  json_proc::ProblemsCollector& problemsCollector);

    bool forType([[maybe_unused]] nlohmann::json::value_t elemType)
    {
        return true;
    }

    template <typename... Args>
    static std::shared_ptr<JsonAnyOfCheck> create(const Args&... args)
    {
        return std::shared_ptr<JsonAnyOfCheck>(new JsonAnyOfCheck(args...));
    }
};

template <typename... Args>
std::shared_ptr<JsonFeatureChecker> anyOf(const Args&... args)
{
    return JsonAnyOfCheck::create(args...);
}

// JsonStringLengthCheck //////////////////////////////////////////////////////

class JsonStringLengthCheck : public JsonFeatureChecker
{
  protected:
    unsigned bound;
    std::string relation;

    JsonStringLengthCheck(unsigned bound, const std::string& relation) :
        bound(bound), relation(relation)
    {}

  public:
    bool rawCheck(const nlohmann::json& element,
                  const json_proc::StackJsonPath* const elementPath,
                  json_proc::ProblemsCollector& problemsCollector);

    virtual bool checkLength(unsigned elementLength) = 0;

    bool forType(nlohmann::json::value_t elemType)
    {
        return elemType == nlohmann::json::value_t::string;
    }
};

// JsonStringMinLengthCheck ///////////////////////////////////////////////////

class JsonStringMinLengthCheck : public JsonStringLengthCheck
{
    JsonStringMinLengthCheck(unsigned bound) :
        JsonStringLengthCheck(bound, "no shorter than")
    {}

  public:
    bool checkLength(unsigned elementLength)
    {
        return elementLength >= bound;
    }

    static std::shared_ptr<JsonStringMinLengthCheck> create(unsigned arg)
    {
        return std::shared_ptr<JsonStringMinLengthCheck>(
            new JsonStringMinLengthCheck(arg));
    }
};

std::shared_ptr<JsonFeatureChecker> minLength(unsigned length);

// JsonStringMaxLengthCheck ///////////////////////////////////////////////////

class JsonStringMaxLengthCheck : public JsonStringLengthCheck
{
    JsonStringMaxLengthCheck(unsigned bound) :
        JsonStringLengthCheck(bound, "no longer than")
    {}

  public:
    bool checkLength(unsigned elementLength)
    {
        return elementLength <= bound;
    }

    static std::shared_ptr<JsonStringMaxLengthCheck> create(unsigned arg)
    {
        return std::shared_ptr<JsonStringMaxLengthCheck>(
            new JsonStringMaxLengthCheck(arg));
    }
};

std::shared_ptr<JsonFeatureChecker> maxLength(unsigned length);

} // namespace json_schema
