
/*
 *
 */

#include "message_composer.hpp"

#include "aml.hpp"
#include "dbus_accessor.hpp"
#include "event_handler.hpp"
#include "event_info.hpp"

#include <boost/algorithm/string.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/exception.hpp>

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <numeric>
#include <utility>
#include <vector>

namespace message_composer
{

using phosphor::logging::entry;
using phosphor::logging::level;
using phosphor::logging::log;

/**
 * This maps Json severity to Phosphor Logging severity
 */
std::map<std::string, std::string> severityMapper{
    {"OK", "Informational"}, {"Ok", "Informational"}, {"ok", "Informational"},
    {"Warning", "Warning"},  {"warning", "Warning"},  {"Critical", "Critical"},
    {"critical", "Critical"}};

MessageComposer::~MessageComposer()
{}

#ifndef EVENTING_FEATURE_ONLY
std::string MessageComposer::getOriginOfConditionObjectPath(
    const std::string& deviceId) const
{
    if (this->dat.at(deviceId).hasDbusObjectOocSpecificExplicit())
    {
        return *(this->dat.at(deviceId).getDbusObjectOocSpecificExplicit());
    }
    else
    {
        // If no ooc object path was provided in dat.json explicitly then try to
        // obtain it by looking what is available on dbus and seems to
        // correspond to the 'deviceId' given in argument
        dbus::DirectObjectMapper om;
        auto paths = om.getPrimaryDevIdPaths(deviceId);
        if (paths.size() == 0)
        {
            logs_err("No object path found in ObjectMapper subtree "
                     "corresponding to the device '%s'. "
                     "Returning empty origin of condition.\n",
                     deviceId.c_str());
            return deviceId;
        }
        else
        {
            if (paths.size() > 1)
            {
                logs_wrn("Multiple object paths in ObjectMapper subtree "
                         "corresponding to the device '%s'. "
                         "Choosing the first one as origin of condition.\n",
                         deviceId.c_str());
            }
            return *paths.begin();
        }
    }
}

std::string MessageComposer::getOriginOfCondition(event_info::EventNode& event)
{
    std::string oocDevice{""};
    std::string originOfCondition{""};
    if (event.getOriginOfCondition().has_value())
    {
        {
            std::string val = event.getOriginOfCondition().value();
            if (boost::starts_with(val, "/redfish/v1"))
            {
                logs_dbg("Message Composer to use fixed redfish URI OOC '%s'\n",
                         val.c_str());
                originOfCondition = val;
            }
        }
    }
    if (originOfCondition.empty())
    {
        if (this->dat.count(event.device) > 0)
        {
            oocDevice =
                this->dat.at(event.device).healthStatus.originOfCondition;
            if (false == oocDevice.empty())
            {
                originOfCondition = getOriginOfConditionObjectPath(oocDevice);
            }
        }
        else
        {
            logs_dbg("Device does not exist in DAT: '%s'\n",
                     event.device.c_str());
            return "";
        }
    }
    return originOfCondition;
}
#endif // EVENTING_FEATURE_ONLY

bool MessageComposer::createLog(event_info::EventNode& event)
{
    auto bus = sdbusplus::bus::new_default_system();
    auto method = bus.new_method_call(
        "xyz.openbmc_project.Logging", "/xyz/openbmc_project/logging",
        "xyz.openbmc_project.Logging.Create", "Create");
    method.append(event.event);
    method.append(makeSeverity(event.getSeverity()));

    auto messageArgs = event.getStringMessageArgs();
    auto telemetries = collectDiagData(event);

#ifdef EVENTING_FEATURE_ONLY
    std::string originOfCondition{"Not supported"};
#else

    std::string originOfCondition = getOriginOfCondition(event);
    if (originOfCondition.empty())
    {
        log_err("Origin of Condition for event %s is empty\n",
                event.event.c_str());
        return false;
    }

    log_dbg("originOfCondition = '%s'\n", originOfCondition.c_str());
#endif // EVENTING_FEATURE_ONLY

    auto pNamespace = getPhosphorLoggingNamespace(event);

    method.append(std::array<std::pair<std::string, std::string>, 10>(
        {{{"xyz.openbmc_project.Logging.Entry.Resolution",
           event.getResolution()},
          {"REDFISH_MESSAGE_ID", event.getMessageId()},
          {"DEVICE_EVENT_DATA", telemetries},
          {"namespace", pNamespace},
          {"REDFISH_MESSAGE_ARGS", messageArgs},
          {"REDFISH_ORIGIN_OF_CONDITION", originOfCondition},
          {"DEVICE_NAME", event.device},
          {"FULL_DEVICE_NAME", event.getFullDeviceName()},
          {"EVENT_NAME", event.event},
          {"RECOVERY_TYPE", !event.recovery_accessor.isEmpty()
                                ? "property_change"
                                : "other"}}}));

    try
    {
        bus.call(method);
        return true;
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        std::cerr << "ERROR CREATING LOG " << e.what() << "\n";
        log<level::ERR>("Failed to create log for event",
                        entry("SDBUSERR=%s", e.what()));
        return false;
    }
}

std::string MessageComposer::makeSeverity(const std::string& severityJson)
{
    // so far only "OK" generates a problem, replace it by "Informational"
    std::string severity{"xyz.openbmc_project.Logging.Entry.Level."};
    if (severityMapper.count(severityJson) != 0)
    {
        severity += severityMapper.at(severityJson);
    }
    else
    {
        severity += severityJson;
    }
    return severity;
}

} // namespace message_composer
