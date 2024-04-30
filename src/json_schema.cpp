/**
 * Copyright (c) 2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#include "json_schema.hpp"

namespace json_schema
{

std::string singleQuote(const std::string& sth)
{
    return "'" + sth + "'";
}

// Utils //////////////////////////////////////////////////////////////////////

bool singleCheck(bool condition, std::string messageIfConditionNotMet,
                 const json_proc::StackJsonPath* const elementPath,
                 json_proc::ProblemsCollector& problemsCollector)
{
    if (!condition)
    {
        problemsCollector.push_back(json_proc::JsonFormatProblem(
            json_proc::StackJsonPath::getPath(elementPath),
            messageIfConditionNotMet));
    }
    return condition;
}

// JsonLiteralSchema //////////////////////////////////////////////////////////

bool JsonLiteralSchema::rawCheck(
    [[maybe_unused]] const nlohmann::json& element,
    const json_proc::StackJsonPath* const elementPath,
    json_proc::ProblemsCollector& problemsCollector)
{
    return singleCheck(value, "Not allowed", elementPath, problemsCollector);
}

std::shared_ptr<JsonSchema> literal(bool value)
{
    return JsonLiteralSchema::create(value);
}

// JsonFeaturesCheckerSchema //////////////////////////////////////////////////

bool JsonFeaturesCheckerSchema::rawCheck(
    const nlohmann::json& element,
    const json_proc::StackJsonPath* const elementPath,
    json_proc::ProblemsCollector& problemsCollector)
{
    bool result = true;
    for (auto& checker : checkers)
    {
        if (checker->forType(element.type()))
        {
            bool testResult =
                checker->rawCheck(element, elementPath, problemsCollector);
            result = result && testResult;
        }
    }
    return result;
}

// JsonTypeChecker ////////////////////////////////////////////////////////////

bool JsonTypeChecker::rawCheck(
    const nlohmann::json& element,
    const json_proc::StackJsonPath* const elementPath,
    json_proc::ProblemsCollector& problemsCollector)
{
    return singleCheck(
        std::find(expectedTypes.cbegin(), expectedTypes.cend(),
                  element.type()) != expectedTypes.cend(),
        (expectedTypes.size() == 1
             ? ("Type expected to be " +
                singleQuote(json_proc::getTypeAsString(expectedTypes.at(0))))
             : ("Type expected to be one of " +
                setToString<nlohmann::json::value_t>(
                    expectedTypes,
                    [](const nlohmann::json::value_t& elem) {
                        return singleQuote(json_proc::getTypeAsString(elem));
                    }))) +
            " (actual type: " +
            singleQuote(json_proc::getTypeAsString(element.type())) + ")",
        elementPath, problemsCollector);
}

// JsonPropertiesCheck ////////////////////////////////////////////////////////

bool JsonPropertiesCheck::rawCheck(
    const nlohmann::json& element,
    const json_proc::StackJsonPath* const elementPath,
    json_proc::ProblemsCollector& problemsCollector)
{
    bool result = true;
    for (const auto& [key, value] : element.items())
    {
        bool testResult =
            (propertiesCheckers.contains(key) ? propertiesCheckers.at(key)
                                              : additionalPropertiesSchema)
                ->rawCheckDescend(value, {elementPath, key}, problemsCollector);
        result = result && testResult;
    }
    return result;
}

std::pair<std::string, std::shared_ptr<JsonSchema>>
    property(std::string name, std::shared_ptr<JsonSchema> checker)
{
    return std::make_pair(name, checker);
}

// JsonItemsCheck /////////////////////////////////////////////////////////////

bool JsonItemsCheck::rawCheck(const nlohmann::json& element,
                              const json_proc::StackJsonPath* const elementPath,
                              json_proc::ProblemsCollector& problemsCollector)
{
    bool result = true;
    for (unsigned i = 0u; i < element.size(); ++i)
    {
        bool testResult = elementSchema->rawCheckDescend(
            element.at(i), {elementPath, i}, problemsCollector);
        result = result && testResult;
    }
    return result;
}

std::shared_ptr<JsonFeatureChecker>
    items(std::shared_ptr<JsonSchema> elementSchema)
{
    return JsonItemsCheck::create(elementSchema);
}

// JsonEnumCheck //////////////////////////////////////////////////////////////

bool JsonEnumCheck::rawCheck(const nlohmann::json& element,
                             const json_proc::StackJsonPath* const elementPath,
                             json_proc::ProblemsCollector& problemsCollector)
{
    return singleCheck(
        std::find(allowedValues.cbegin(), allowedValues.cend(), element) !=
            allowedValues.cend(),
        "Value expected to be one of " +
            setToString<nlohmann::json>(
                allowedValues,
                [](const nlohmann::json& val) { return val.dump(); }) +
            " (actual value: " + element.dump() + ")",
        elementPath, problemsCollector);
}

// JsonRequiredPropertiesCheck ////////////////////////////////////////////////

bool JsonRequiredPropertiesCheck::rawCheck(
    const nlohmann::json& element,
    const json_proc::StackJsonPath* const elementPath,
    json_proc::ProblemsCollector& problemsCollector)
{
    bool result = true;
    for (const auto& requiredProperty : requiredProperties)
    {
        bool testResult = element.contains(requiredProperty);
        if (!testResult)
        {
            problemsCollector.push_back(json_proc::JsonFormatProblem(
                json_proc::StackJsonPath::getPath(elementPath),
                std::string("Required property ") +
                    singleQuote(requiredProperty) + " not found"));
        }
        result = result && testResult;
    }
    return result;
}

// JsonBoundCheck /////////////////////////////////////////////////////////////

std::string JsonBoundCheck::boundCheckErrorMsg(const nlohmann::json& element,
                                               const std::string& relation)
{
    return "Value expected to be " + relation + " " +
           (element.is_number_float() ? nlohmann::json(bound).dump()
                                      : nlohmann::json((int)bound).dump()) +
           " (actual value: " + element.dump() + ")";
}

// JsonLowerBoundCheck ////////////////////////////////////////////////////////

bool JsonLowerBoundCheck::rawCheck(
    const nlohmann::json& element,
    const json_proc::StackJsonPath* const elementPath,
    json_proc::ProblemsCollector& problemsCollector)
{
    return singleCheck(element.get<double>() >= bound,
                       boundCheckErrorMsg(element, "greater than or equal to"),
                       elementPath, problemsCollector);
}

std::shared_ptr<JsonFeatureChecker> minimum(const double& arg)
{
    return JsonLowerBoundCheck::create(arg);
}

// JsonUpperBoundCheck ////////////////////////////////////////////////////////

bool JsonUpperBoundCheck::rawCheck(
    const nlohmann::json& element,
    const json_proc::StackJsonPath* const elementPath,
    json_proc::ProblemsCollector& problemsCollector)
{
    return singleCheck(element.get<double>() <= bound,
                       boundCheckErrorMsg(element, "lesser than or equal to"),
                       elementPath, problemsCollector);
}

std::shared_ptr<JsonFeatureChecker> maximum(const double& arg)
{
    return JsonUpperBoundCheck::create(arg);
}

// JsonAnyOfCheck /////////////////////////////////////////////////////////////

bool JsonAnyOfCheck::rawCheck(const nlohmann::json& element,
                              const json_proc::StackJsonPath* const elementPath,
                              json_proc::ProblemsCollector& problemsCollector)
{
    bool someSchemaMatched = false;
    std::vector<json_proc::ProblemsCollector> localProblemsCollectors(
        schemas.size());
    for (unsigned i = 0u; i < schemas.size() && !someSchemaMatched; ++i)
    {
        someSchemaMatched =
            someSchemaMatched ||
            schemas.at(i)->rawCheck(element, elementPath,
                                    localProblemsCollectors.at(i));
    }
    if (!someSchemaMatched)
    {
        problemsCollector.push_back(json_proc::JsonFormatProblem(
            json_proc::StackJsonPath::getPath(elementPath),
            "Element doesn't match any of the alllowed schemas"));
        for (unsigned i = 0u; i < localProblemsCollectors.size(); ++i)
        {
            for (const auto& problem : localProblemsCollectors.at(i))
            {
                problemsCollector.push_back(json_proc::JsonFormatProblem(
                    json_proc::getPath(problem),
                    json_proc::getMessage(problem) + " (schema no. " +
                        std::to_string(i) + ")"));
            }
        }
    }
    return someSchemaMatched;
}

// JsonStringLengthCheck //////////////////////////////////////////////////////

bool JsonStringLengthCheck::rawCheck(
    const nlohmann::json& element,
    const json_proc::StackJsonPath* const elementPath,
    json_proc::ProblemsCollector& problemsCollector)
{
    unsigned len = element.get<std::string>().length();
    return singleCheck(checkLength(len),
                       "String expected to be " + relation + " " +
                           std::to_string(bound) +
                           " (actual length: " + std::to_string(len) + ")",
                       elementPath, problemsCollector);
}

// JsonStringMinLengthCheck ///////////////////////////////////////////////////

std::shared_ptr<JsonFeatureChecker> minLength(unsigned length)
{
    return JsonStringMinLengthCheck::create(length);
}

// JsonStringMaxLengthCheck ///////////////////////////////////////////////////

std::shared_ptr<JsonFeatureChecker> maxLength(unsigned length)
{
    return JsonStringMaxLengthCheck::create(length);
}

} // namespace json_schema
