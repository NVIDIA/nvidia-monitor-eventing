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

#include "common.hpp"
#include "dat_traverse.hpp"
#include "data_accessor.hpp"
#include "dbus_accessor.hpp"
#include "event_handler.hpp"
#include "event_info.hpp"

#include <boost/algorithm/string.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/exception.hpp>

#include <string>

namespace message_composer
{

/**
 * @brief A class for composing event log entry and create it in
 * phosphor-logging.
 *
 */
class MessageComposer : public event_handler::EventHandler
{
  public:
#ifdef EVENTING_FEATURE_ONLY
    MessageComposer(const std::string& name = __PRETTY_FUNCTION__) :
        event_handler::EventHandler(name)
    {}
#else
    MessageComposer(std::map<std::string, dat_traverse::Device>& datMap,
                    const std::string& name = __PRETTY_FUNCTION__) :
        event_handler::EventHandler(name),
        dat(datMap)

    {}
#endif // EVENTING_FEATURE_ONLY

    ~MessageComposer();

  public:
    /**
     * @brief Turn event into phosphor-logging Entry format and create the log
     * based on device namespace.
     *
     * @param event
     * @return eventing::RcCode
     */
    eventing::RcCode process([[maybe_unused]] event_info::EventNode& event) override
    {
        bool success = createLog(event);
        if (success)
        {
            return eventing::RcCode::succ;
        }
        return eventing::RcCode::error;
    }

    /**
     * @brief Return an object path from @c xyz.openbmc_project.ObjectMapper
     * service which corresponds to the given @c deviceId.
     *
     * This will be put in the REDFISH_ORIGIN_OF_CONDITION Log additional's data
     * property.
     *
     * For example, for the "PCIeRetimer_0" given as the @c deviceId a
     * "/xyz/openbmc_project/inventory/system/chassis/HGX_PCIeRetimer_0" path
     * should be returned.
     *
     * If no associated object path could be found return an empty string.
     */
    template <typename ObjectMapperType = dbus::DirectObjectMapper>
    std::string getOriginOfConditionObjectPath(const std::string& deviceId) const
    {
#ifndef EVENTING_FEATURE_ONLY
        if (this->dat.at(deviceId).hasDbusObjectOocSpecificExplicit())
        {
            return *(this->dat.at(deviceId).getDbusObjectOocSpecificExplicit());
        }
        else
#endif
        {
            // If no ooc object path was provided in dat.json explicitly then try to
            // obtain it by looking what is available on dbus and seems to
            // correspond to the 'deviceId' given in argument
            ObjectMapperType om;
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

    /**
     * @brief Gets origin of condition of the event
     * @param event
     * @return string representation of OOC
     */
    template <typename ObjectMapperType = dbus::DirectObjectMapper>
    std::string getOriginOfCondition(event_info::EventNode& event)
    {
        std::string oocDevice{""};
        // NOTE: expectation is now that in selftest, even if the original JSON
        // does not have a fixed OOC defined, it will be filled in by the
        // RootCauseTracer, so this branch should always be entered
        // unless there was an error
        if (event.getOriginOfCondition().has_value())
        {
            std::string val = event.getOriginOfCondition().value();
            if (boost::starts_with(val, "/redfish/v1"))
            {
                logs_dbg("Message Composer to use fixed redfish URI OOC '%s'\n",
                            val.c_str());
                return val;
            }
            else
            {
                oocDevice = val;
            }
        }

        if (!oocDevice.empty())
        {
            std::string path = getOriginOfConditionObjectPath<ObjectMapperType>(oocDevice);
            logs_dbg("Got path '%s' from oocDevice '%s'\n", path.c_str(),
                oocDevice.c_str());
            return path;
        }
        logs_err("Invalid JSON definition!! No fixed or dynamic OOC found for event: "
                "'%s', device: '%s' !\n",
                event.getName().c_str(), event.device.c_str());
        return event.device;
    }

    /**
     * @brief Return phosphor logging Namespace to be used for log
     *
     * This will be put in the REDFISH_ORIGIN_OF_CONDITION Log additional's data
     * property.
     *
     * For example, for the "PCIeRetimer_0" given as the @c deviceId a
     * "/xyz/openbmc_project/inventory/system/chassis/HGX_PCIeRetimer_0" path
     * should be returned.
     *
     * If no associated object path could be found return an empty string.
     */
    static std::string
        getPhosphorLoggingNamespace(const event_info::EventNode& event)
    {
        std::string pNamespace = event.device;
        if (event.subType != "")
        {
            pNamespace += "_" + event.subType;
        }

        return pNamespace;
    }

    /**
     * @brief Use stored in event node data; Collect event related telemetries
     *        values as diag data.
     *
     * @param event
     * @return std::string&
     */
    static std::string collectDiagData(const event_info::EventNode& event)
    {
        nlohmann::ordered_json output;

        if (!event.trigger.isEmpty())
        {
            output["trigger"] = event.trigger.getDataValue().getString();
        }
        else
        {
            output["trigger"] = "empty";
        }

        if (!event.accessor.isEmpty())
        {
            output["accessor"] = event.accessor.getDataValue().getString();
        }
        else
        {
            output["accessor"] = "empty";
        }

        output["selftest"] = event.selftestReport;

        for (auto telemetry : event.telemetries)
        {
            std::string telemetryName = telemetry[data_accessor::nameKey];
            // using telemetry.read() at Event level to use Event information
            std::string telemetryValue = telemetry.read(event);
            output[telemetryName] = telemetryValue;
        }
        return output.dump();
    }

    /**
     *  Creates the full Phosphor Logging Severity Level string
     *  If necessary converts a Json severity to  Phosphor Logging severity
     *
     * @return the whole Phosphor Logging string
     */
    static std::string makeSeverity(const std::string& severityJson);

  private:
    /**
     * @brief Establishes D-Bus connection and creates log with eventNode
     * data in phosphor-logging
     *
     * @param event
     * @return boolean for whether or not it was a success
     */
    bool createLog(event_info::EventNode& event);

#ifndef EVENTING_FEATURE_ONLY
    std::map<std::string, dat_traverse::Device>& dat;
#endif
};

} // namespace message_composer
