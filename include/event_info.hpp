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

#include "data_accessor.hpp"
#include "device_id.hpp"
#include "json_proc.hpp"
#include "object.hpp"
#include "util.hpp"

#include <nlohmann/json.hpp>

#include <map>
#include <memory>
#include <unordered_map>
#include <string>
#include <vector>

using json = nlohmann::json;

namespace event_info
{
class EventNode;

struct counterReset
{
    std::string type;
    std::string metadata;
};

struct Message
{
    std::string severity;
    std::string resolution;

  public:
    /**
     * @brief Print this object to the output stream @c os (e.g. std::cout,
     * std::cerr, std::stringstream) with every line prefixed with @c indent.
     *
     * For the use with logging framework use the following construct:
     *
     * @code
     *   std::stringstream ss;
     *   obj.print(ss, indent);
     *   log_dbg("%s", ss.str().c_str());
     * @endcode
     */
    template <class CharT>
    void print(std::basic_ostream<CharT>& os = std::cout,
               std::string indent = std::string("")) const
    {
        os << indent << "severity:"
           << "\t" << severity << std::endl;
        os << indent << "resolution:"
           << "\t" << resolution << std::endl;
    }
};

struct MessageArgPattern
{
    std::string pattern;

  public:
    /**
     * @brief Get the number of placeholders (substrings of the form '{...}') in
     * the @c pattern
     */
    unsigned placeholdersCount() const;

    /**
     * @brief Get a string with all placeholders (substrings of the form
     * '{...}') replaced with the corresponding values in @c values
     *
     * For example, given object @c x with @c x.pattern being "name: {TheName},
     * age: {TheAge}" the @c substPlaceholders({"Karen", "42"}) returns "name:
     * Karen, age: 42".
     *
     * The @c pattern field is not modified.
     *
     * @param[in] values It's assumed the length of the vector is equal to the
     * value @c placeholdersCount() would return.
     */
    std::string substPlaceholders(const std::vector<std::string>& values) const;

    /**
     * @brief Print this object to the output stream @c os (e.g. std::cout,
     * std::cerr, std::stringstream) with every line prefixed with @c indent.
     *
     * For the use with logging framework use the following construct:
     *
     * @code
     *   std::stringstream ss;
     *   obj.print(ss, indent);
     *   log_dbg("%s", ss.str().c_str());
     * @endcode
     */
    template <class CharT>
    void print(std::basic_ostream<CharT>& os = std::cout,
               std::string indent = std::string("")) const
    {
        os << indent << pattern << std::endl;
    }
};

struct MessageArg
{
    MessageArgPattern pattern;
    std::vector<data_accessor::DataAccessor> parameters;

  public:
    /**
     * @brief Print this object to the output stream @c os (e.g. std::cout,
     * std::cerr, std::stringstream) with every line prefixed with @c indent.
     *
     * For the use with logging framework use the following construct:
     *
     * @code
     *   std::stringstream ss;
     *   obj.print(ss, indent);
     *   log_dbg("%s", ss.str().c_str());
     * @endcode
     */
    template <class CharT>
    void print(std::basic_ostream<CharT>& os = std::cout,
               std::string indent = std::string("")) const
    {
        os << indent << "pattern:" << std::endl;
        pattern.print(os, indent + "\t");
        os << indent << "parameters:" << std::endl;
        util::print(parameters, os, indent + "\t");
    }

    /**
     * @brief Get a string with values from the accessors inserted into '{...}'
     * placeholders in @c pattern
     *
     * Call the accessors in sequence and insert the values they return in the
     * placeholders in `pattern'. For instance, assuming this object represents
     * the first message arg ("{GPUId} Temperature") of the json snippet
     *
     * ,----
     * | "message_args": [
     * |   "{GPUId} Temperature",
     * |   "{UpperFatal Threshold}"
     * | ],
     * | "parameters": [
     * |   {
     * |     "type": "DIRECT",
     * |     "field": "CurrentDeviceName"
     * |   },
     * |   {
     * |     "type": "...",
     * |     "interface": "...",
     * |     "object": "...",
     * |     "property": "..."
     * |   }
     * | ]
     * `----
     *
     * and the event occured on "GPU3" the string "GPU3 Temperature" will be
     * returned.
     *
     * @param[in] event Information needed to compose the message args about the
     * event. The object passed can (and most probably should) be the one @c
     * *this is contained in as the @c messageRegistry.messageArgs[...] field.
     */
    std::string getStringMessageArg(const EventNode& event);
};

struct redfish
{
    std::string messageId;
    Message message;
    // marcinw:TODO: to remove
    std::vector<MessageArg> messageArgs;
    nlohmann::json messageArgsJsonPattern;

  public:
    /**
     * @brief Print this object to the output stream @c os (e.g. std::cout,
     * std::cerr, std::stringstream) with every line prefixed with @c indent.
     *
     * For the use with logging framework use the following construct:
     *
     * @code
     *   std::stringstream ss;
     *   obj.print(ss, indent);
     *   log_dbg("%s", ss.str().c_str());
     * @endcode
     */
    template <class CharT>
    void print(std::basic_ostream<CharT>& os = std::cout,
               std::string indent = std::string("")) const
    {
        os << indent << "messageId:"
           << "\t" << this->messageId << std::endl;
        os << indent << "message:" << std::endl;
        message.print(os, indent + "\t");
        os << indent << "messageArgs:" << std::endl;
        util::print(messageArgs, os, indent + "\t");
    }

    /**
     * @brief Evaluate the dynamic message arg and encode them into a single
     * string
     *
     * Concatenate the results of evaluating each dynamic message arg with a
     * comma. For instance, assuming this object represents the json snippet
     *
     * @code
     * "message_args": {
     *   "patterns": [
     *     "{GPUId} Temperature",
     *     "{UpperFatal Threshold}"
     *   ],
     *   "parameters": [
     *     {
     *       "type": "DIRECT",
     *       "field": "CurrentDeviceName"
     *     },
     *     {
     *       "type": "DBUS",
     *       "interface": "...",
     *       "object": "...",
     *       "property": "..."
     *     }
     *   ]
     * }
     *
     * @endcode
     *
     * and
     * 1. the event occured on "GPU3" device
     * 2. dbus call returned "120" string
     *
     * the string "GPU3 Temperature, 120" will be returned.
     *
     * @param[in] event Information needed to compose the message args about the
     * event. The object passed can (and most probably should) be the one @c
     * *this is contained in as the @c messageRegistry field.
     */
    std::string getStringMessageArgsDynamic(const EventNode& event);

    /**
     * @brief Compose REDFISH_MESSAGE_ARGS from event
     *
     * Behave like @c getStringMessageArgsDynamic except ignore the actual @c
     * messageArgs value and act as if it corresponded to the following
     * configuration:
     *
     * @code
     * "message_args": {
     *   "message_args": [
     *     "{}",
     *     EVENT_NAME
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
     * where EVENT_NAME is simply the value of @c event.event.
     *
     * This function is provided for backward compatibility.
     *
     * @param event
     * @return std::string&
     */
    std::string getStringMessageArgsStatic(const EventNode& event);

    /**
     * @brief Fall back to @c getStringMessageArgsDynamic or @c
     * getStringMessageArgsStatic depending on @c messageArgs size.
     */
    std::string getStringMessageArgs(const EventNode& event);
};


/**
 * @brief Represent a test layer the given event corresponds to
 *
 * Basically an enum over string values {"power_rail", "erot_control",
 * "pin_status", "interface_status", "protocol_status", "firmware_status"}
 * defined by the static field @c valuesAllowed. All the code around is for
 * guarding the @c category field not to ever assume anything outside this set.
 */
class EventCategory
{
  public:
    EventCategory() = delete;
    EventCategory(const std::string& category);

    EventCategory(const EventCategory& other) = default;
    EventCategory& operator=(const EventCategory& other) = default;
    EventCategory(EventCategory&& other) noexcept = default;
    EventCategory& operator=(EventCategory&& other) noexcept = default;

    std::string get();
    void set(const std::string& category);

    /** @brief A set of values the @c category field is allowed to assume **/
    static const std::vector<std::string> valuesAllowed;

    bool operator==(const EventCategory& other) const = default;
    operator std::string() const;

  private:
    std::string category;
};

/** @class EventNode
 *  @brief Object to represent HMC events and store contents
 *         from event_info json profile
 */
class EventNode : public object::Object
{

  public:
    EventNode(const std::string& name = __PRETTY_FUNCTION__) :
        object::Object(name), triggerCount(0), valueAsCount(false)
    {}

    EventNode(const EventNode& r) = default;

  public:
    /** @brief Load class contents from JSON profile
     *
     * @param[in]  j  - json object
     *
     */
    void loadFrom(const json& j);

    /** @brief Dumps current object class content to stdout
     */
    void print(void) const;

  public:
    /** @brief The copy of the original json event node **/
    nlohmann::json configEventNode;

    /** @brief A sequence of numbers identifying a device this event occured on
     *
     *  Example, Object path: /xyz/openbmc_project/NVSwitch_2/Ports/NVLink_23
     *  Supposing the example above, it will contain (2,23)
     **/
    std::optional<device_id::PatternIndex> deviceIndexTuple;

    /** @brief Redfish fields struct **/
    redfish messageRegistry;

    /** @brief Redfish OriginOfCondition field **/
    std::optional<std::string> originOfCondition;

    /** @brief Type of device **/
    std::vector<std::string> deviceTypes;

    /** @brief Classification of the event which steers the selftest process */
    std::optional<std::vector<EventCategory>> eventCategories;

  public:
    /** @brief Name of the event **/
    std::string event;

    /** @brief Error ID of the event **/
    std::string errorId;

    /** @brief The Device this Error will impact **/
    std::string rollupDevicePattern;

    /** @brief Type of device subtype **/
    std::string subType;

    /** @brief What triggered event **/
    std::string eventTrigger;

    /** @brief Accesssor info **/
    data_accessor::DataAccessor accessor;

    /** @brief Accesssor info **/
    data_accessor::DataAccessor recovery_accessor;

    /** @brief Trigger accessor info **/
    data_accessor::DataAccessor trigger;

    /** @brief Event count map: maps device name to count **/
    std::map<std::string, int> count;

    /** @brief Count that will trigger event **/
    int triggerCount;

    /** @brief Struct containing type and metadata **/
    data_accessor::DataAccessor counterReset;

    /** @brief List of the event's telemetries **/
    std::vector<data_accessor::DataAccessor> telemetries;

    /** @brief Action HMC should take **/
    std::string action;

    /** @brief Particular device name .. i.e. GPU0 **/
    std::string device;

    /** @brief Flag to indicate if we should use event value as trigger count
     * **/
    bool valueAsCount;

    /** @brief Report of selftest **/
    nlohmann::ordered_json selftestReport;

    /** @brief Severities to base device health update on **/
    std::vector<util::Severity> severities;

    /**
     * @brief Setter for @c deviceIndexTuple field
     */
    void setDeviceIndexTuple(const device_id::PatternIndex& deviceIndexTuple);

    // No getter provided for 'deviceIndexTuple'. This is an abstract identifier
    // used for inner mechanics of determining the source of an event. If a user
    // of 'EventNode' requires this information from this class then he's
    // probably trying to implement something that should be placed here.

    /**
     * @brief True if @c setDeviceIndexTuple has been called on this object,
     * false otherwise
     */
    bool isEventNodeEvaluated() const;

    /**
     * @brief Return the severity of the event
     *
     * E.g. "Critical", "Warning", "OK"
     */
    std::string getSeverity() const;

    /**
     * @brief Return the resolution of the problem specified by this event
     *
     * E.g "Reset the link. If problem persists, isolate the server for RMA
     * evaluation."
     */
    std::string getResolution() const;

    /**
     * @brief Return a code specifying the message format string describing the
     * event in redfish
     *
     * E.g. "ResourceEvent.1.0.ResourceErrorsDetected"
     */
    std::string getMessageId() const;

    /**
     * @brief Return the arguments provided to the format string specified by @c
     * getMessageId concatenated with comma
     *
     * This is the format expected by bmcweb.
     *
     * E.g. "NVSwitch_1 PCIe, Abnormal Speed Change", "GPU_SXM_4 Memory,
     * Row-Remapping Pending"
     */
    std::string getStringMessageArgs();

    bool hasFixedOriginOfCondition() const
    {
        return originOfCondition.has_value();
    }

    /**
     * @brief Set the origin_of_condition.
     *
     */
    void setOriginOfCondition(const std::string& ooc)
    {
        originOfCondition.emplace(ooc);
    }

    /**
     * @brief Return the origin_of_condition instance.
     *
     */
    std::optional<std::string> getOriginOfCondition() const
    {
        if (isEventNodeEvaluated())
        {
            if (originOfCondition.has_value())
            {
                return device_id::DeviceIdPattern(*originOfCondition)
                    .eval(*deviceIndexTuple);
            }
            else
            {
                return std::nullopt;
            }
        }
        else
        {
            // This shouldn't happen. Force quit to bring attention.
            throw std::runtime_error(
                "Trying to get OOC on an unevaluated event node. Quit...");
        }
    }

    /**
     * @brief Call @c setDeviceTypes(js) wrapping it in a sensible error message
     */
    void readDeviceTypes(const json& js, const std::string& eventName);

    /**
     * @brief Set the @c deviceTypes field directly from a corresponding
     * attribute of the event node in the event info json config file
     */
    void setDeviceTypes(const json& js);

    /**
     * @brief Setter for @c deviceTypes field
     */
    void setDeviceTypes(std::vector<std::string>&& values);

    /**
     * @brief Return a string representation of the devices set this event is
     * associated with
     *
     * If @c deviceTypes has one element return the content of this element. If
     * it has more elements concatenate them with '/' and return as a single
     * string. The situation of @c deviceTypes having no elements will not
     * happen because @c setDeviceTypes will reject it.
     *
     * E.g. return "GPU_SXM_[1-8]" for @c deviceTypes being
     *
     * @code
     * ["GPU_SXM_[1-8]"]
     * @endcode
     *
     * Return "NVSwitch_[0|0-3]/NVLink_[1|0-39]" for @c deviceTypes being
     *
     * @code
     * ["NVSwitch_[0|0-3]", "NVLink_[1|0-39]"]
     * @endcode
     */
    std::string getStringifiedDeviceType() const;

    /**
     * @brief getDataDeviceType() returns util::DeviceIdData about device_type
     * @return full event.device_type information
     */
    util::DeviceIdData getDataDeviceType() const;

    /**
     * @brief Return the first element of @c deviceTypes
     */
    std::string getMainDeviceType() const;

    std::vector<EventCategory> getCategories() const;
    std::vector<std::string> getStringCategories() const;

    /**
     * @brief Return whether the DataAccessor is interesting
     * (matches our D-Bus object/interface/property)
     */
    static bool getIsAccessorInterestingToEvent(const EventNode& event,
        const data_accessor::DataAccessor& otherAccessor);

    /** @returns The full (single/multi) device name for the @a index
     *  @param [in] index another index which can be different from the Event
     */
    std::string getFullDeviceName(device_id::PatternIndex& index) const;

    /** @returns The current full device name based on internal device index */
    std::string getFullDeviceName() const;

    /** @returns the full device name not joined by a separator such as '/' */
    std::vector<std::string> getFullDeviceNameSeparated(
        device_id::PatternIndex& index) const;

    /** just separates a full device name @sa getFullDeviceName() */
    static std::vector<std::string>
        separateFullDeviceName(const std::string& fullName);
};

using EventMap = std::map<std::string, std::vector<event_info::EventNode>>;
/** @brief tuple (property, interface, object path)
 * Note it is in REVERSE order for operator< efficiency
 */
using PropertyFilterTuple = std::tuple<std::string, std::string, std::string>;
using PropertyFilterSet = std::set<PropertyFilterTuple>;

using EventTriggerView = std::unordered_multimap<data_accessor::DataAccessor,
    std::shared_ptr<event_info::EventNode>, data_accessor::DataAccessor::Hash>;

using EventAccessorView = std::unordered_multimap<data_accessor::DataAccessor,
    std::shared_ptr<event_info::EventNode>, data_accessor::DataAccessor::Hash>;

using EventRecoveryView = std::unordered_multimap<data_accessor::DataAccessor,
    std::shared_ptr<event_info::EventNode>, data_accessor::DataAccessor::Hash>;

/** @brief Add D-Bus properties from @c eventNode to @c propertyFilterSet
 *
 * @param[in]      eventNode
 * @param[in,out]  propertyFilterSet
*/
void addEventToPropertyFilterSet(const EventNode& eventNode,
    PropertyFilterSet& propertyFilterSet);

/** @brief Load class contents from JSON profile
 *
 * Wrapper method for loadFrom
 *
 * @param[in]  eventMap
 * @param[in]  file
 *
 */
void loadFromFile(EventMap& eventMap, PropertyFilterSet& propertyFilterSet,
    EventTriggerView& eventTriggerView, EventAccessorView& eventAccessorView,
    EventRecoveryView& eventRecoveryView, const std::string& file);

/**
 * @brief Read data from the json object @c j into @c eventMap
 *
 * @param[out] eventMap
 * @param[in] j
 */
void loadFromJson(EventMap& eventMap, PropertyFilterSet& propertyFilterSet,
    EventTriggerView& eventTriggerView, EventAccessorView& eventAccessorView,
    EventRecoveryView& eventRecoveryView, const nlohmann::json& j);

/** @brief Prints out memory map to verify field population
 *
 * @param[in]  eventMap
 *
 */
void printMap(const EventMap& eventMap);

/** @brief Prints out D-Bus property set to verify field population
 *
 * @param[in]  propertyFilterSet
 *
 */
void printSet(const PropertyFilterSet& propertyFilterSet);

} // namespace event_info

namespace nlohmann
{

/**
 * @brief Allow for 'js.get<EventCategory>()' syntax
 */
template <>
struct adl_serializer<event_info::EventCategory>
{
    static event_info::EventCategory from_json(const json& js)
    {
        return {js.get<std::string>()};
    }

    static void to_json(json& js, event_info::EventCategory ec)
    {
        js = ec.get();
    }
};

} // namespace nlohmann
