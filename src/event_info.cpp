/**
 * Copyright (c) 2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */


#include "event_info.hpp"

#include "json_proc.hpp"
#include "util.hpp"

#include <boost/algorithm/string/split.hpp>
#include <nlohmann/json.hpp>

#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <regex>
#include <string>
#include <vector>

using json = nlohmann::json;

namespace event_info
{

std::string MessageArg::getStringMessageArg(const EventNode& event)
{
    std::vector<std::string> values;
    for (auto& accessor : parameters)
    {
        values.push_back(accessor.read(event));
    }
    return pattern.substPlaceholders(values);
}

std::string redfish::getStringMessageArgsDynamic(const EventNode& event)
{
    std::stringstream ss;
    std::string delimiter = ", ";
    for (unsigned i = 0u; i < messageArgs.size(); ++i)
    {
        if (i != 0)
        {
            ss << delimiter;
        }
        ss << messageArgs.at(i).getStringMessageArg(event);
    }
    return ss.str();
}

std::string redfish::getStringMessageArgsStatic(const EventNode& event)
{
    std::vector<std::string> args;
    args.push_back(event.device);
    args.push_back(event.event);
    std::string msg = "";
    for (auto it = args.begin(); it != std::prev(args.end()); it++)
    {
        msg += *it + ", ";
    }
    msg += args.back();
    return msg;
}

std::string redfish::getStringMessageArgs(const EventNode& event)
{
    if (messageArgs.size() > 0)
    {
        return getStringMessageArgsDynamic(event);
    }
    else
    {
        return getStringMessageArgsStatic(event);
    }
}

unsigned MessageArgPattern::placeholdersCount() const
{
    static const std::regex reg("^[^\\{]*\\{[^\\}]*\\}");
    std::smatch match;
    std::string str = pattern;
    unsigned matchesCount = 0;
    while (std::regex_search(str, match, reg))
    {
        str = match.suffix();
        matchesCount++;
    }
    return matchesCount;
}

std::string MessageArgPattern::substPlaceholders(
    const std::vector<std::string>& values) const
{
    static const std::regex matchReg("^[^\\{]*\\{[^\\}]*\\}");
    static const std::regex substReg("\\{[^\\}]*\\}");
    std::smatch matchObj;
    std::string str = pattern;
    std::string result;

    for (const auto& elem : values)
    {
        std::regex_search(str, matchObj, matchReg);
        std::string matchedStr = matchObj.str(0);
        str = matchObj.suffix();
        result += std::regex_replace(matchedStr, substReg, elem);
    }
    result += str;
    return result;
}

void addEventToPropertyFilterSet(const EventNode& eventNode,
    PropertyFilterSet& propertyFilterSet)
{
    for (const auto& accessor : {eventNode.accessor, eventNode.trigger,
        eventNode.recovery_accessor})
    {
        if (accessor.isEmpty() || !accessor.isTypeDbus())
        {
            continue;
        }
        device_id::DeviceIdPattern pattern(accessor.getDbusObjectPath());
        std::string iface = accessor.getDbusInterface();
        std::string prop = accessor.getProperty();
        for (auto path : pattern.values())
        {
            propertyFilterSet.emplace(prop, iface, path);
        }
    }
}

void loadFromJson(EventMap& eventMap, PropertyFilterSet& propertyFilterSet,
    EventTriggerView& eventTriggerView, EventAccessorView& eventAccessorView,
    EventRecoveryView& eventRecoveryView, const nlohmann::json& j)
{
    for (const auto& el : j.items())
    {
        std::stringstream ss;
        auto deviceType = el.key();
        std::vector<event_info::EventNode> v = {};
        ss.str(std::string()); // Clearing the stream first
        ss << "new device type: " << deviceType << "\r\n";
        logs_dbg("%s", ss.str().c_str());

        for (const auto& event : el.value())
        {

            ss.str(std::string()); // Clearing the stream first
            ss << "\tcreate event (" << event["event"] << ").\n";
            logs_dbg("%s", ss.str().c_str());

            auto eventNode = std::make_shared<event_info::EventNode>(event["event"]);

            ss.str(std::string()); // Clearing the stream first
            ss << "\tload event (" << event["event"] << ").\n";
            logs_dbg("%s", ss.str().c_str());
            eventNode->loadFrom(event);

            ss.str(std::string()); // Clearing the stream first
            ss << "\tpush event (" << event["event"] << ").\n";
            logs_dbg("%s", ss.str().c_str());

            v.push_back(*eventNode);

            // also add eventNode's accessors to the dbus property filter set
            addEventToPropertyFilterSet(*eventNode, propertyFilterSet);

            eventTriggerView.insert({eventNode->trigger, eventNode});
            eventAccessorView.insert({eventNode->accessor, eventNode});
            if (!eventNode->recovery_accessor.isEmpty())
            {
                eventRecoveryView.insert({eventNode->recovery_accessor, eventNode});
            }
        }
        eventMap.insert(
            std::pair<std::string, std::vector<event_info::EventNode>>(
                deviceType, v));
    }
    logs_err("size of propertyFilterSet: %zu\n", propertyFilterSet.size());
    // printSet(propertyFilterSet);
}

void loadFromFile(EventMap& eventMap, PropertyFilterSet& propertyFilterSet,
    EventTriggerView& eventTriggerView, EventAccessorView& eventAccessorView,
    EventRecoveryView& eventRecoveryView, const std::string& file)
{
    std::stringstream ss;
    ss << "loadFromFile func (" << file << ").";
    logs_dbg("%s\n", ss.str().c_str());
    std::ifstream i(file);
    json j;
    i >> j;

    loadFromJson(eventMap, propertyFilterSet, eventTriggerView,
        eventAccessorView, eventRecoveryView, j);
}

void printMap(const EventMap& eventMap)
{
    for (const auto& dev : eventMap)
    {
        std::cerr << dev.first << " events:\n";

        for (const auto& event : dev.second)
        {
            std::cerr << event.event << "\n";
            event.print();
        }

        std::cerr << "\n";
    }
}

void printSet(const PropertyFilterSet& propertyFilterSet)
{
    for (const auto& tuple : propertyFilterSet)
    {
        std::cerr << "<";
        std::cerr << std::get<0>(tuple);
        std::cerr << ", ";
        std::cerr << std::get<1>(tuple);
        std::cerr << ", ";
        std::cerr << std::get<2>(tuple);
        std::cerr << ">";
        std::cerr << std::endl;
    }
}

/**
 * @brief Read "patterns" property of the "redfish.message_args"
 * entry in event info node of the json config file.
 *
 * Auxiliary function to the @c EventNode::loadFrom(const json& j)
 *
 * Given the @c messageArgsJson parameter representing an object of the form,
 * eg:
 *
 * @code
 * {
 *   "patterns": [
 *     "{GPUId} Memory",
 *     "Uncontained ECC Error"
 *   ],
 *   "parameters": [
 *     {
 *       "type": "DIRECT",
 *       "field": "CurrentDeviceName"
 *     }
 *   ]
 * }
 * @endcode
 *
 * return a vector of elements
 *
 * @code
 * [0]: "{GPUId} Memory"
 * [1]: "Uncontained ECC Error"
 * @endcode
 */
std::vector<MessageArgPattern>
    loadMessageArgsPatterns(const json& messageArgsJson)
{
    std::vector<MessageArgPattern> result;
    for (const json& elem : messageArgsJson["patterns"])
    {
        result.push_back(MessageArgPattern{elem.get<std::string>()});
    }
    return result;
}

/**
 * @brief Parse the subsequence of "parameters" attribute of the
 * "redfish.message_args" entry in event info node of the json config file.
 *
 * Auxiliary function to the @c EventNode::loadFrom(const json& j)
 *
 * Given the @c accessorsJson parameter representing an array of the form,
 * eg:
 *
 * @code
 * [
 *   {
 *     "type": "DIRECT",
 *     "field": "CurrentDeviceName"
 *   },
 *   {
 *     "type": "CONSTANT",
 *     "value": "plug"
 *   }
 * ]
 * @endcode
 *
 * and the @c fromIncl, @c toExcl paraemeters being 0, 1,
 * resepctively, return a vector of one object representing the
 *
 * @code
 * {
 *   "type": "DIRECT",
 *   "field": "CurrentDeviceName"
 * }
 * @endcode
 *
 * accessor definition.
 *
 * @param[in] accessorsJson The json array object representing the "parameters"
 * attribute of the "redfish.message_args" entry in event info node of the json
 * config file.
 *
 * @param[in] fromIncl The index in the json array @c accessorsJson from which
 * to start constructing @c DataAccessor objects, inclusive
 *
 * @param[in] toExcl The index in the json array @c accessorsJson to which end
 * constructing @c DataAccessor objects, exclusive
 */
std::vector<data_accessor::DataAccessor>
    loadMessageArgsAccessors(const json& accessorsJson, unsigned fromIncl,
                             unsigned toExcl)
{
    std::vector<data_accessor::DataAccessor> result;
    for (unsigned i = fromIncl; i < toExcl; ++i)
    {
        result.push_back(data_accessor::DataAccessor(accessorsJson[i]));
    }
    return result;
}

/**
 * @brief Parse the "message_args" property of the "redfish" entry in the event
 * info node of the 'event_info.json' file into a vector of objects.
 *
 * Auxiliary function to the @c EventNode::loadFrom(const json& j)
 *
 * The number of elements in the result correspond to the number of elements in
 * the "patterns" property. The "parameters" array is split into chunks and
 * coupled with the objects holding the pattern associated with it. For example,
 * given the "redfish" entry like
 *
 * @code
 * "redfish": {
 *   "message_id": "ResourceEvent.1.0.ResourceErrorThresholdExceeded",
 *   "message_args": {
 *     "patterns": [
 *       "{NVSwitchId} Temperature",
 *       "{UpperCritical Threshold}"
 *     ],
 *     "parameters": [
 *       {
 *         "type": "DIRECT",
 *         "field": "CurrentDeviceName"
 *       },
 *       {
 *         "type": "CONSTANT",
 *         "value": "plug"
 *       }
 *     ]
 *   }
 * }
 * @endcode
 *
 * the resulting vector will contain two @c MessageArg objects,
 * which can be represented as
 *
 * @code
 * [0]: ("{GPUId} Memory", { "type": "DIRECT","field": "CurrentDeviceName" })
 * [1]: ("Uncontained ECC Error", { "type": "CONSTANT","value": "plug" })
 * @endcode
 *
 * Including the literal characters '{' and '}' can be accomplished by using the
 * "CONSTANT"-type accessor containing those characters. For example, to obtain
 * the message arg "{x__x}" use this construct:
 *
 * @code
 *     "patterns": [
 *       "{}",
 *     ],
 *     "parameters": [
 *       {
 *         "type": "CONSTANT",
 *         "value": "{x__x}"
 *       }
 *     ]
 * @endcode
 *
 * @param[in] messageArgsJson A json object representing the value of
 * "redfish.message_args" entry in the even info node
 */
std::vector<event_info::MessageArg> loadMessageArgs(const json& messageArgsJson)
{
    std::vector<MessageArgPattern> patterns =
        loadMessageArgsPatterns(messageArgsJson);

    json accessorsJson;
    if (messageArgsJson.contains("parameters"))
    {
        accessorsJson = messageArgsJson["parameters"];
    }
    else
    {
        // assume the empty "parameters" array was passed
        accessorsJson = json::array();
    }

    unsigned totalPlaceholders =
        std::accumulate(patterns.begin(), patterns.end(), 0u,
                        [](const unsigned& res, const MessageArgPattern& p) {
                            return res + p.placeholdersCount();
                        });
    if (totalPlaceholders != accessorsJson.size())
    {
        std::stringstream ss;
        ss << "The total number of placeholders in \"patterns\" property (= "
           << totalPlaceholders
           << ") is different from the number of elements in the \"parameters\" property (= "
           << accessorsJson.size() << ") for the \"message_args\" value '"
           << messageArgsJson << "'" << std::endl;
        logs_err("%s", ss.str().c_str());
        throw std::runtime_error(ss.str().c_str());
    }

    std::vector<event_info::MessageArg> result;
    unsigned placeholdersRead = 0u;
    for (const auto& pattern : patterns)
    {
        unsigned count = pattern.placeholdersCount();
        result.push_back(MessageArg{
            pattern, loadMessageArgsAccessors(accessorsJson, placeholdersRead,
                                              placeholdersRead + count)});
        placeholdersRead += count;
    }

    return result;
}

void EventNode::readDeviceTypes(const json& js, const std::string& eventName)
{
    try
    {
        this->setDeviceTypes(js);
    }
    catch (const std::exception& e)
    {
        std::stringstream ss;
        ss << "Error reading \"device_type\" property in event entry '"
           << eventName << "': " << e.what();
        shortlog_err(<< ss.str());
        throw std::runtime_error(ss.str());
    }
}

void EventNode::loadFrom(const json& j)
{
    this->configEventNode = j;

    this->event = j.at("event");
    if (j.contains("error_id"))
    {
        this->errorId = j.at("error_id");
    }
    readDeviceTypes(j.at("device_type"), this->event);
    this->triggerCount = j.at("trigger_count").get<int>();
    // this->eventTrigger = j.at("event_trigger");

    this->eventCategories =
        json_proc::getOptionalAttribute<std::vector<EventCategory>>(j,
                                                                    "category");

    this->subType = j.value("sub_type", "");

    for (auto& entry : j.at("telemetries"))
    {
        this->telemetries.push_back((data_accessor::DataAccessor)entry);
    }
    this->action = j.at("action");
    this->device = "";

    this->counterReset = j.at("event_counter_reset");

    // std::vector<event_info::MessageArg> messageArgs;
    // if (j["redfish"].contains("message_args"))
    // {
    //     messageArgs = loadMessageArgs(j["redfish"]["message_args"]);
    // }
    // // otherwise leave `messageArgs' empty

    this->messageRegistry = {
        j.at("redfish").at("message_id").get<std::string>(),
        {j.at("severity"), j.at("resolution")},
        {},
        j.at("redfish").contains("message_args")
            ? j.at("redfish").at("message_args")
            : nlohmann::json()};

    this->accessor = j.at("accessor");
    this->trigger = j.at("event_trigger").empty()
        ? j.at("accessor")
        : j.at("event_trigger");

    std::stringstream ss;
    ss << "Loaded accessor: " << this->accessor << ", j: " << j;
    log_dbg("%s\n", ss.str().c_str());

    if (j.contains("recovery"))
    {
        this->recovery_accessor = j.at("recovery");
        std::stringstream ss2;
        ss2 << "Loaded accessor: " << this->recovery_accessor << ", j: " << j;
        log_dbg("%s\n", ss2.str().c_str());
    }
    // else
    // {
    //     std::stringstream ss2;
    //     ss2 << "did not load recovery accessor, the value in the node is "
    //         << this->recovery_accessor << ", j: " << j << std::endl
    //         << "is the accessor empty? " << this->recovery_accessor.isEmpty();
    //     log_err("%s\n", ss2.str().c_str());
    // }

    this->valueAsCount = j.contains("value_as_count")
                             ? j.at("value_as_count").get<bool>()
                             : false;

    this->originOfCondition = json_proc::getOptionalAttribute<std::string>(
        j.at("redfish"), "origin_of_condition");

    // this->originOfCondition = j.contains("origin_of_condition")
    //                               ?
    //                               j["origin_of_condition"].get<std::string>()
    //                               : std::nullptr;
    // log_dbg("origin_of_condition is [%s].\n",
    //         this->originOfCondition.asString());
}

void EventNode::print() const
{
    std::stringstream ss;
    ss << accessor << "\n";
    ss << "\tDumping event     " << event << "\n";

    ss << "\t\tdeviceType      " << getStringifiedDeviceType() << "\n";
    ss << "\t\teventTrigger    " << eventTrigger << "\n";
    ss << "\t\taccessor        "
       << "todo"
       << "\n";
    ss << accessor << "\n";

    ss << "\t\tcount(map)      " << count.size() << "\n";
    for (auto& p : count)
    {
        ss << "\t\t\t[" << p.first << "] = " << p.second << "\n";
    }

    ss << "\t\ttriggerCount    " << triggerCount << "\n";
    ss << "\t\tcounterReset    "
       << "todo"
       << "\n";
    ss << counterReset << "\n";

    ss << "\t\tredfish:" << std::endl;
    messageRegistry.print(ss, "\t\t\t");

    ss << "\t\t\t" << messageRegistry.message.severity << "\n";
    ss << "\t\t\t" << messageRegistry.message.resolution << "\n";

    ss << "\t\ttelemetries     " << telemetries.size() << "\n";
    for (auto& v : telemetries)
    {
        ss << "\t\t\t" << v << "\n";
    }

    ss << "\t\taction          " << action << "\n";
    ss << "\t\tdevice          " << device << "\n";

    // Should be compatible with 'printMap'
    std::cerr << ss.str();
}

void EventNode::setDeviceIndexTuple(
    const device_id::PatternIndex& deviceIndexTuple)
{
    this->deviceIndexTuple = deviceIndexTuple;
}

bool EventNode::isEventNodeEvaluated() const
{
    return deviceIndexTuple.has_value();
}

std::string EventNode::getSeverity() const
{
    return messageRegistry.message.severity;
}

std::string EventNode::getResolution() const
{
    return messageRegistry.message.resolution;
}

std::string EventNode::getMessageId() const
{
    return messageRegistry.messageId;
}

std::string EventNode::getStringMessageArgs()
{
    if (isEventNodeEvaluated())
    {
        try
        {
            log_dbg("messageRegistry.messageArgsJsonPattern: %s\n",
                    messageRegistry.messageArgsJsonPattern.dump().c_str());
            if (messageRegistry.messageArgsJsonPattern.is_null())
            {
                messageRegistry.messageArgs.clear();
            }
            else
            {
                nlohmann::json messageArgsJsonEvaluation =
                    json_proc::JsonPattern(
                        messageRegistry.messageArgsJsonPattern)
                        .eval(*deviceIndexTuple);
                log_dbg("messageArgsJsonEvaluation: %s",
                        messageArgsJsonEvaluation.dump().c_str());
                messageRegistry.messageArgs =
                    loadMessageArgs(messageArgsJsonEvaluation);
            }
            return messageRegistry.getStringMessageArgs(*this);
        }
        catch (const std::exception& e)
        {
            shortlog_err(
                << "An error occured during \"message_args\" processing: "
                << e.what() << ". Returning empty message args string");
            return "";
        }
    }
    else
    {
        shortlog_err(
            << "Called 'getStringMessageArgs' on an unevaluated event node. "
            << "Returning empty message args string");
        return "";
    }
}

void EventNode::setDeviceTypes(const json& js)
{
    if (js.is_string())
    {
        std::vector<std::string> elements;
        boost::split(elements, js.get<std::string>(), boost::is_any_of("/"));
        setDeviceTypes(elements);
    }
    else if (js.is_array())
    {
        std::vector<std::string> result;
        for (const auto& elem : js)
        {
            if (elem.is_string())
            {
                result.push_back(elem.get<std::string>());
            }
            else // ! elem.is_string()
            {
                std::stringstream ss;
                ss << "Value '" << elem.dump() << "' expected to be a string";
                throw std::runtime_error(ss.str());
            }
        }
        setDeviceTypes(std::move(result));
    }
    else
    {
        std::stringstream ss;
        ss << "value '" << js.dump()
           << "' expected to be a string or an array of strings";
        throw std::runtime_error(ss.str());
    }
}

void EventNode::setDeviceTypes(std::vector<std::string>&& values)
{
    if (values.size() > 0)
    {
        this->deviceTypes = std::move(values);
    }
    else
    {
        std::stringstream ss;
        ss << "Device types array is expected to have at least 1 element. "
           << "Actual size: " << values.size();
        throw std::runtime_error(ss.str());
    }
}

std::string EventNode::getStringifiedDeviceType() const
{
    return boost::algorithm::join(this->deviceTypes, "/");
}

util::DeviceIdData EventNode::getDataDeviceType() const
{
    device_id::PatternIndex indexTuple;
    if (deviceIndexTuple.has_value())
    {
         indexTuple = *deviceIndexTuple;
    }
    return util::DeviceIdData(getStringifiedDeviceType(), indexTuple);
}

std::string EventNode::getMainDeviceType() const
{
    if (deviceTypes.size() < 1)
    {
        throw std::runtime_error(
            "No main device type associated with this event");
    }
    return this->deviceTypes.at(0);
}

std::string EventNode::getFullDeviceName(device_id::PatternIndex& index) const
{
    std::string fullDevice{""};
    if (deviceTypes.size() > 0)
    {
        device_id::DeviceIdPattern devTypePattern(getStringifiedDeviceType());
        if (devTypePattern.dim() == index.dim() ||
            (devTypePattern.dim() == 0 && index.dim() == 1 && index[0] == 0))
        {
              fullDevice = devTypePattern.eval(index);
        }
        else
        if (devTypePattern.dim() > 0 && devTypePattern.dim() < index.dim())
        {
          // device_type such as "PCIeSwitch_0/Down_[0-3]" must work for both
          // device_id::DeviceIdPattern(x) and device_id::DeviceIdPattern(0,x)
              device_id::PatternIndex indexAdjusted;
              for (unsigned dim = 1; dim <= devTypePattern.dim(); ++dim)
              {
                indexAdjusted.set(dim -1, index[dim]);
              }
              fullDevice = devTypePattern.eval(indexAdjusted);
        }
    }
    return fullDevice;
}

std::string EventNode::getFullDeviceName() const
{
    device_id::PatternIndex index(0);
    if (this->deviceIndexTuple.has_value())
    {
        index = *(this->deviceIndexTuple);
    }
    return getFullDeviceName(index);
}

std::vector<std::string>
EventNode::getFullDeviceNameSeparated(device_id::PatternIndex& index) const
{
    std::vector<std::string> fullDevices;
    auto fullDeviceNameSlash = getFullDeviceName(index);
    return EventNode::separateFullDeviceName(fullDeviceNameSlash);
}

std::vector<std::string>
    EventNode::separateFullDeviceName(const std::string& fullName)
{
    std::vector<std::string> fullDevices;
    if (!fullName.empty())
    {
         boost::split(fullDevices, fullName, boost::is_any_of("/"));
    }
    return fullDevices;
}

// marcinw:TODO: solve the issue of 'getCategories', 'getStringCategories'
// better
std::vector<EventCategory> EventNode::getCategories() const
{
    if (eventCategories.has_value())
    {
        return *eventCategories;
    }
    else // ! eventCategories.has_value()
    {
        return std::vector<EventCategory>();
    }
}

std::vector<std::string> EventNode::getStringCategories() const
{
    std::vector<EventCategory> input = getCategories();
    std::vector<std::string> output;
    std::transform(input.begin(), input.end(), std::back_inserter(output),
                   [](const EventCategory& elem) -> std::string {
                       return (std::string)elem;
                   });
    return output;
}

// EventCategory //////////////////////////////////////////////////////////////

const std::vector<std::string> EventCategory::valuesAllowed{
    "association",      "power_rail",      "erot_control",   "pin_status",
    "interface_status", "protocol_status", "firmware_status"};

EventCategory::EventCategory(const std::string& category)
{
    set(category);
}

std::string EventCategory::get()
{
    return category;
}

void EventCategory::set(const std::string& category)
{
    // disabled the code below to allow any Category be used (it is free)
#if 0
    if (std::find(valuesAllowed.cbegin(), valuesAllowed.cend(), category) ==
        valuesAllowed.cend())
    {
        // Object 'category' not found in 'valuesAllowed'
        throw std::runtime_error("Invalid event category: '" + category + "'");
    }
#endif

    this->category = category;
}

EventCategory::operator std::string() const
{
    return category;
}

bool EventNode::getIsAccessorInterestingToEvent(const EventNode& event,
    const data_accessor::DataAccessor& otherAccessor)
{
    return event.accessor == otherAccessor ||
        (!event.trigger.isEmpty() && event.trigger == otherAccessor) ||
        (!event.recovery_accessor.isEmpty() && event.recovery_accessor == otherAccessor);
}

} // namespace event_info
