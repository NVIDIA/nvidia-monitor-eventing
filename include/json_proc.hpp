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

#include "device_id.hpp"

#include <event_info.hpp>
#include <nlohmann/json.hpp>

#include <variant>

/**
 * @brief Module providing useful types and functions to operate on
 * nlohmann::json objects
 *
 * - Evaluating patterned json at point (with @c PatternIndex instance)
 * - Generating json values from a patterned json
 * - Tools for addressing and accessing sub-elements of json
 * - Converting optional attributes into user-defined objects
 */
namespace json_proc
{

// JsonPattern ////////////////////////////////////////////////////////////////

/**
 * @brief A json analogue of the @c DeviceIdPattern class
 *
 * This class is to @c nlohmann::json as @c DeviceIdPattern is to @c
 * std::string, with the functionality reduced to just producing possible
 * instantiations (the @c values() method), since no other were needed.
 */
class JsonPattern
{
  public:
    explicit JsonPattern(const nlohmann::json& js) : js(js)
    {}

    /**
     * @brief Evaluate the patterned json element
     *
     * The evaluation rule is simple: if the json object is a string then make
     * @c DeviceIdPattern instance of it and evaluate at @index. If it's any
     * other primitive type leave it as it is. If it's a compound type (object
     * or array) then apply evaluation recursively.
     *
     * For example, if this object represents the patterned json element
     *
     * @code
     * {
     *   "patterns": [
     *     "PCIeSwitch_0/DOWN_[0|0-3] PCIe",
     *     "LTSSM Link Down"
     *   ],
     *   "parameters": []
     * }
     * @endcode
     *
     * then evaluating it at point '(2)' would yield the json result
     *
     * @code
     * {
     *   "patterns": [
     *     "PCIeSwitch_0/DOWN_2 PCIe",
     *     "LTSSM Link Down"
     *   ],
     *   "parameters": []
     * }
     * @endcode
     *
     @param[in] index
     */
    nlohmann::json eval(const device_id::PatternIndex& index) const;

    /**
     * @brief Generate a sequence of json object in a similar manner as @c
     * DeviceIdPattern::values()
     *
     * The logic is equivalent to converting the json given in the
     * constructor to a raw string, constructing @c DeviceIdPattern of it,
     * calling its @c values(), and parsing every element from the obtained
     * sequence back to a json object.
     *
     * For example, if the argument given to the constructor of this class is a
     * json object
     *
     * @code
     * {
     *   "type": "CMDLINE",
     *   "executable": "fpga_regtbl_wrapper",
     *   "arguments": "FPGA_NVSW[0|0-3]_EROT_RECOV_L NVSwitch_[0|0-3]",
     *   "check": {
     *     "equal": "0"
     *   }
     * }
     * @endcode
     *
     * then the jsons produced by @c values() method will be
     *
     * @code
     * {
     *   "type": "CMDLINE",
     *   "executable": "fpga_regtbl_wrapper",
     *   "arguments": "FPGA_NVSW0_EROT_RECOV_L NVSwitch_0",
     *   "check": {
     *     "equal": "0"
     *   }
     * }
     * {
     *   "type": "CMDLINE",
     *   "executable": "fpga_regtbl_wrapper",
     *   "arguments": "FPGA_NVSW1_EROT_RECOV_L NVSwitch_1",
     *   "check": {
     *     "equal": "0"
     *   }
     * }
     * {
     *   "type": "CMDLINE",
     *   "executable": "fpga_regtbl_wrapper",
     *   "arguments": "FPGA_NVSW2_EROT_RECOV_L NVSwitch_2",
     *   "check": {
     *     "equal": "0"
     *   }
     * }
     * {
     *   "type": "CMDLINE",
     *   "executable": "fpga_regtbl_wrapper",
     *   "arguments": "FPGA_NVSW3_EROT_RECOV_L NVSwitch_3",
     *   "check": {
     *     "equal": "0"
     *   }
     * }
     * @endcode
     */
    std::vector<nlohmann::json> values() const;

    /**
     * @brief Return a const reference to the private json element
     */
    const nlohmann::json& getJson() const;

  private:
    nlohmann::json js;
};

/**
 * @brief Return a value from @c js object under @c attr key or @c std::nullopt
 * if the key is not present.
 *
 * Makes it easy to read optional attributes from json config files, eg.
 * "origin_of_condition".
 *
 * Requires support for the 'js.get<T>()' syntax for the target @c T type of the
 * optional object read. See
 * https://github.com/nlohmann/json#arbitrary-types-conversions for details.
 *
 * Example usage:
 *
 * @code
 *     this->dbus_set_health =
 *         json_proc::getOptionalAttribute<bool>(js, "dbus_set_health");
 * @endcode
 *
 * @param[in] js A json object potentially containing the @c attr key. If @c js
 * is not a json object a runtime exception is thrown.
 *
 * @param[in] attr
 */
template <typename T>
std::optional<T> getOptionalAttribute(const nlohmann::json& js,
                                      const std::string& attr)
{
    if (js.is_object())
    {
        if (js.contains(attr))
        {
            return js.at(attr).get<T>();
        }
        else // ! js.contains(attr)
        {
            return std::nullopt;
        }
    }
    else
    {
        throw std::runtime_error("! js.is_object()");
    }
}

/**
 * @brief String representation of the @c json element's type
 *
 * @code
 * | Type                           | String repr       |
 * |--------------------------------+-------------------|
 * | json::value_t::null            | "null"            |
 * | json::value_t::boolean         | "boolean"         |
 * | json::value_t::number_integer  | "number_integer"  |
 * | json::value_t::number_unsigned | "number_unsigned" |
 * | json::value_t::number_float    | "number_float"    |
 * | json::value_t::object          | "object"          |
 * | json::value_t::array           | "array"           |
 * | json::value_t::string          | "string"          |
 * | <other>                        | "(unknown)"       |
 * @endcode
 *
 * @param[in] json
 */
std::string getTypeAsString(const nlohmann::json& json);

// Model of json path /////////////////////////////////////////////////////////

/**
 * @brief A single json path's coordinate
 *
 * It can be either a key in an object (string) or an index in an array
 * (unsigned).
 */
using JsonPathElement = std::variant<std::string, unsigned>;

/**
 * @brief Convert a json coordinate into a string representation
 *
 * For keys in objects it's just their name, eg. "interface". For indexes in
 * arrays it's the index put into square bracket, eg. "[5]".
 *
 * @param[in] coordinate
 */
std::string to_string(const JsonPathElement& coordinate);

/**
 * @brief Address of some sub-json in the given larger json
 *
 * For example, given the json element (the "telemetries" of the "FPGA Temp
 * Alert" event):
 *
 * @code
 * [
 *   {
 *     "name": "FPGA Thermal Parameter",
 *     "type": "DeviceCoreAPI",
 *     "property": "fpga.thermal.alert"
 *   },
 *   {
 *     "name": "FPGA Temperature",
 *     "type": "DeviceCoreAPI",
 *     "property": "fpga.thermal.temperature.singlePrecision"
 *   }
 * ]
 * @endcode
 *
 * the "fpga.thermal.alert" string can be uniquely located by a series 2 values:
 *
 * 1. integer 0 :: index of the first object
 *
 * 2. string "property" :: key under which the "fpga.thermal.alert" value can be
 * found in the first object.
 *
 * This address can be represented by the sequence [0, "property"] which the @c
 * JsonPath type specifies.
 */
using JsonPath = std::deque<JsonPathElement>;

/**
 * @brief Convert a json path into a string representation
 *
 * A series fo coordinate representation connected with dots, eg.
 * ".[0].property", ".NVSwitch_1.interface_status.[0]", ".GPU.[3]".
 *
 * @param[in] jsonPath
 */
std::string to_string(const JsonPath& jsonPath);

// Regarding some other implemetations if one wished: anything that supports
// - empty()
// - iteration (as in 'for (auto elem : path)')
// - 'push_front(JsonPathElement)'
// is ok

/**
 * @brief Representation of a json path optimized for recursive descending
 * algorithms
 *
 * Transforming json element usually lends itself to an elegant recursive
 * algorithm terminating on primitive types (null, integer, float, string) and
 * descending recursively on the composite types (object, array). At the same
 * time a path to the currently visited node is useful for providing error
 * messages or even the result itself (like when the json is being searched for
 * some value). The @c JsonPath type can be used for that but it requires
 * awkward for recursion and inefficient construction of mutable objects, like
 *
 * @code
 * void someFunc(nlohmann::json jsonCurrentElem, JsonPath jsonCurrentElemPath)
 * {
 *     // ...
 *     JsonPath jsonSubElemPath = jsonCurrentElemPath;
 *     jsonSubElemPath.push_back(jsonSubElemCoordinate);
 *     someFunc(jsonSubElem, jsonSubElemPath);
 *     // ...
 * }
 * @endcode
 *
 * This struct provides a simple, efficient and recursion-friendly way to
 * construct a json path by saving pointers to the json path elements already
 * allocated on the stack:
 *
 * @code
 * void someFunc(nlohmann::json jsonCurrentElem, StackJsonPath
 * jsonCurrentElemPath)
 * {
 *     // ...
 *     someFunc(jsonSubElem, {&jsonCurrentElemPath, jsonSubElemCoordinate});
 *     // ...
 * }
 * @endcode
 *
 * At any given moment a function @c getPath() may be invoked on the object to
 * obtain the standard path representation.
 *
 * @code
 * StackJsonPath::getPath(&jsonCurrentElemPath)
 * @endcode
 */
struct StackJsonPath
{
    const StackJsonPath* const previous;
    JsonPathElement element;

    /**
     * @brief Convert the @c StackJsonPath object into standard @c JsonPath
     * representation
     *
     * @param[in] startNode The last coordinate of the json path to be included
     * in the result.
     */
    static JsonPath getPath(const StackJsonPath* const startNode)
    {
        JsonPath result;
        const StackJsonPath* node = startNode;
        while (node != nullptr)
        {
            result.push_front(node->element);
            node = node->previous;
        }
        return result;
    }
};

/**
 * @brief Type representing some problem with a json value
 *
 * The first element is a path to the json value and the second one contains the
 * description of the problem.
 */
using JsonFormatProblem = std::pair<JsonPath, std::string>;

/**
 * @brief Convert a json format problem into a string representation
 *
 * The format is "{path}: {proglem}", eg.
 * ".NVSwitch_1.interface_status.[0].accessor.type: Unrecognized accessor type
 * 'foo'"
 *
 * @param[in] formatProblem
 */
std::string to_string(const JsonFormatProblem& formatProblem);

/**
 * @brief Get the path part of the formatting problem
 *
 * @param[in] formatProblem
 */
JsonPath getPath(const JsonFormatProblem& formatProblem);

/**
 * @brief Get the description part of the formatting problem
 *
 * @param[in] formatProblem
 */
std::string getMessage(const JsonFormatProblem& formatProblem);

/**
 * @brief Type used to aggregate problems with json sub-element
 */
using ProblemsCollector = std::vector<JsonFormatProblem>;
// Regarding the possible other imlementations: anything which provides
// 'push_back(JsonFormatProblem)' is ok

/**
 * @brief Check if the given path @c jsPath is a valid address of some
 * sub-element of @c js
 *
 * For example, the json value
 * @code
 * {
 *   "type": "DBUS",
 *   "object": "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_[1-8]",
 *   "interface": "xyz.openbmc_project.State.ResetStatus",
 *   "property": "DrainAndResetRequired",
 *   "check": {
 *     "equal": "true"
 *   }
 * }
 * @endcode
 *
 * contains the address ".type" or ".check.equal", but it doesn't contain the
 * address ".interface.[0]" or ".name".
 *
 * @param[in] js
 * @param[in] jsPath
 */
bool contains(const nlohmann::json& js, const JsonPath& jsPath);

/**
 * @brief Return a sub-value of @c js located at address @c jsPath
 *
 * Example: For the @c js being
 *
 * @code
 * {
 *   "description" : "",
 *   "type" : "object",
 *   "properties" : {
 *     "type" : {
 *       "type" : "string",
 *       "enum" : ["DEVICE"]
 *     },
 *     "device_name" : {
 *       "type" : "number"
 *     }
 *   },
 *   "additionalProperties": false,
 *   "required": [
 *     "type",
 *     "device_name"
 *   ]
 * }
 * @endcode
 *
 * and @c jsPath being ".properties.type" return the json value
 *
 * @code
 * {
 *   "type" : "string",
 *   "enum" : ["DEVICE"]
 * }
 * @endcode
 *
 * @param[in] js The master json value relative to which the sub-element is
 * addressed.
 *
 * @param[in] jsPath A path to the sub-element of the @c. If the path doesn't
 * exist in @c js (in other words if 'contains(js, jsPath)' return false) a
 * runtime error is thrown
 */
const nlohmann::json& nodeAt(const nlohmann::json& js, const JsonPath& jsPath);

} // namespace json_proc
