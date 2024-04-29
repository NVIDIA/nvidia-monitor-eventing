
/*
 *
 */

#pragma once

#include "common.hpp"
#include "check_accessor.hpp"
#include "dat_traverse.hpp"
#include "dbus_accessor.hpp"
#include "event_handler.hpp"
#include "event_info.hpp"
#include "object.hpp"
#include "pc_event.hpp"
#include "selftest.hpp"
#include "threadpool_manager.hpp"

#include <boost/container/flat_map.hpp>
#include <boost/algorithm/string/join.hpp>
#include <dbus_log_utils.hpp>
#include <dbus_utility.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/bus.hpp>

#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <variant>
#include <unordered_set>

#ifndef PROPERTIESCHANGED_QUEUE_SIZE
#define PROPERTIESCHANGED_QUEUE_SIZE 100
#endif

namespace event_detection
{
extern std::unique_ptr<PcQueueType> queue;

extern event_info::EventTriggerView eventTriggerView;
extern event_info::EventAccessorView eventAccessorView;
extern event_info::EventRecoveryView eventRecoveryView;

extern std::unique_ptr<ThreadpoolManager> threadpoolManager;

/** check() from event.trigger against dbusAcc returned false */
constexpr auto msgFalseCheckTriggerCriteria =
    "event.trigger check criteria does not match dbusAcc property value";

/** Fields from event.trigger and dbusAcc are not the same */
constexpr auto msgTriggerFieldsDoNotMatch =
    "event.trigger fields do not match dbusAcc fields";

/** check() from event.accessor against dbusAcc returned false */
constexpr auto msgFalseCheckAccessorCriteria =
    "event.accessor check criteria does not match dbusAcc property value";

/** empty event.trigger and event.accessor fields do not match dbusAcc fields */
constexpr auto msgEmptyTriggerAccessorFieldsDoNotMatch =
    "event.trigger is empty and event.accessor fields do not match dbusAcc fields";

constexpr int invalidIntParam = -1;
using DbusEventHandlerList =
    std::vector<std::unique_ptr<sdbusplus::bus::match_t>>;

/**
 *  This list is used in EventsDetection(),
 *
 *  Each event carries an assertedDeviceList (list of devices and their data)
 *  The tuple will also carry a flag for whether the device will follow recovery
 *  flow or not.
 */
using EventCandidateList =
    std::vector<std::tuple<std::shared_ptr<event_info::EventNode>,
                           data_accessor::AssertedDeviceList, bool>>;

/**
 *   A map to check if a pair object-path + interface has already been
 *    registered for receiving PropertyChange signal from dbus
 *
 *   if a object-path + interface is registred more than once, more than once
 *    message will arrive.
 */
using RegisteredObjectInterfaceMap = std::map<const std::string, int>;

/**
 * @class EventDetection
 * @brief Responsible for catching D-Bus signals and GPIO state change
 * conditions.
 */
class EventDetection : public object::Object
{
  public:
    EventDetection(const std::string& name, event_info::EventMap* eventMap,
                   event_info::PropertyFilterSet* propertyFilterSet,
                   event_handler::EventHandlerManager* hdlrMgr) :
        object::Object(name),
        _eventMap(eventMap), _propertyFilterSet(propertyFilterSet),
        _hdlrMgr(hdlrMgr)
    {}
    ~EventDetection() = default;

  public:
    void identifyEventCandidate(const std::string& objPath,
                                const std::string& signature,
                                const std::string& property);

    /**
     * @brief This is the main function of the worker thread
     */
    static void workerThreadMainLoop();

    /**
     * @brief This is the loop which actually processes PropertiesChanged
     * messages
     */
    static void workerThreadProcessEvents();

    /**
     * @brief This is the callback which handles DBUS properties changes
     */
    static void dbusEventHandlerCallback(sdbusplus::message::message& msg);

    /**
     * @brief Register DBus propertiesChanged handler.
     *
     * @param evtDet
     * @param conn
     * @param iface
     * @return a list of std::unique_ptr<sdbusplus::bus::match_t>
     */
    DbusEventHandlerList
        startEventDetection(EventDetection* evtDet,
                            std::shared_ptr<sdbusplus::asio::connection> conn);
    /**
     * @brief  Reset device health on DBus back to OK
     *
     * @param devId
     */
    static void resetDeviceHealth([[maybe_unused]] const std::string& devId)
    {
#ifdef EVENTING_SERVICE_NO_DEVICE_HEALTH
        logs_err("not setting device Health: Device Health service is enabled\n");
#else
        dbus::DirectObjectMapper om;
        const std::string healthInterface(
            "xyz.openbmc_project.State.Decorator.Health");
        std::vector<std::string> objPathsToAlter =
            om.getAllDevIdObjPaths(devId, healthInterface);
        if (!objPathsToAlter.empty())
        {
            std::string healthState =
                "xyz.openbmc_project.State.Decorator.Health.HealthType.OK";
            for (const auto& objPath : objPathsToAlter)
            {
                logs_dbg("Setting Health Property for: %s healthState: %s\n",
                         objPath.c_str(), healthState.c_str());
                bool ok = dbus::setDbusProperty(
                    objPath, "xyz.openbmc_project.State.Decorator.Health",
                    "Health", PropertyVariant(healthState));
                if (ok == true)
                {
                    logs_dbg("Changed health property as expected\n");
                }
            }
        }
#endif  // EVENTING_SERVICE_NO_DEVICE_HEALTH
    }

    /**
     * @brief  Sets Resolved property of log entry to true
     *
     * @param objPath
     * @param bus
     */
    static void setLogEntryResolved(const std::string objPath,
                                    sdbusplus::bus::bus& bus)
    {
        try
        {
            std::variant<bool> v = true;
            dbus::DelayedMethod method(
                bus, "xyz.openbmc_project.Logging", objPath,
                "org.freedesktop.DBus.Properties", "Set");
            method.append("xyz.openbmc_project.Logging.Entry");
            method.append("Resolved");
            method.append(v);
            auto reply = method.call();
        }
        catch (const sdbusplus::exception::exception& e)
        {
            logs_err(" Dbus Error: %s\n", e.what());
            throw std::runtime_error(e.what());
        }
    }

    /**
     * @brief  Recover from events which can only be recovered after power cycle
     *
     * @param devId
     * @param result
     * @param bus
     */
    static void recoverFromPowerCycleEvents(
        const std::string& devId,
        const dbus::utility::ManagedObjectType& result,
        sdbusplus::bus::bus& bus)
    {
        for (auto& objectPath : result)
        {
            logs_err("Resolving log %s for device %s\n",
                     objectPath.first.str.c_str(), devId.c_str());
            setLogEntryResolved(objectPath.first.str, bus);
        }
        resetDeviceHealth(devId);
    }

    /**
     * @brief Update recovered device's health/healthrollup and log's Resolved
     * DBus property
     *
     * @param eventName
     *
     * @param fullDeviceName the full device considering multi devices
     *    example: "PCIeSwitch_0/Down_3" or just "PCIeSwitch_0" if single device
     */
    static void resolveDeviceLogs(const std::string& eventName,
                                  const std::string& fullDeviceName)
    {
        auto bus = sdbusplus::bus::new_default_system();
        dbus::utility::ManagedObjectType result;
        std::string devId{""};
        auto deviceNames = event_info::EventNode::separateFullDeviceName(
            fullDeviceName);
        try
        {
            devId = deviceNames.at(0);
            dbus::DelayedMethod method(bus, "xyz.openbmc_project.Logging",
                                       "/xyz/openbmc_project/logging",
                                       "xyz.openbmc_project.Logging.Namespace",
                                       "GetAll");
            method.append(devId);
            method.append(
                "xyz.openbmc_project.Logging.Namespace.ResolvedFilterType.Unresolved");
            auto reply = method.call();
            reply.read(result);
        }
        catch (const sdbusplus::exception::exception& e)
        {
            logs_err(" Dbus Error: %s\n", e.what());
            throw std::runtime_error(e.what());
        }

        PROFILING_SWITCH(selftest::TsLatcher TS(
            "resolveDeviceLogs-" + fullDeviceName + "-" + eventName + "-logsNumber-" +
            std::to_string(result.size())));

        if (eventName.length() == 0)
        {
            recoverFromPowerCycleEvents(devId, result, bus);
            return;
        }

        const std::vector<std::string>* additionalDataRaw = nullptr;
        for (auto& objectPath : result)
        {
            for (auto& interfaceMap : objectPath.second)
            {
                if (interfaceMap.first == "xyz.openbmc_project.Logging.Entry")
                {
                    for (auto& propertyMap : interfaceMap.second)
                    {
                        if (propertyMap.first == "AdditionalData")
                        {
                            additionalDataRaw =
                                std::get_if<std::vector<std::string>>(
                                    &propertyMap.second);
                        }
                    }
                }
            }
            std::string messageArgs;
            std::string addlEventName;
            std::string addDeviceName;
            std::string addFullDeviceName;
            if (additionalDataRaw == nullptr)
            {
                continue;
            }
            redfish::AdditionalData additional(*additionalDataRaw);

            if (additional.count("REDFISH_MESSAGE_ARGS") > 0)
            {
                messageArgs = additional["REDFISH_MESSAGE_ARGS"];
            }
            if (additional.count("EVENT_NAME") > 0)
            {
                addlEventName = additional["EVENT_NAME"];
            }
            if (additional.count("DEVICE_NAME") > 0)
            {
                addDeviceName = additional["DEVICE_NAME"];
            }
            if (additional.count("FULL_DEVICE_NAME") > 0)
            {
                addFullDeviceName = additional["FULL_DEVICE_NAME"];
            }
            // both eventName and devId must match
            // corresponding AdditionalData fields
            if (eventName != addlEventName || addDeviceName.empty() ||
                devId != addDeviceName)
            {
                continue;
            }

            bool foundAllDevices = fullDeviceName == addFullDeviceName;
            // if addFullDeviceName is empty foundAllDevices is false
            if (addFullDeviceName.empty() && deviceNames.size() > 1)
            {
                // case where new AML version performing entries generated
                //  by an old AML version which does not save FULL_DEVICE_NAME
                auto reverseCounter = deviceNames.size() -1;
                // main device devId was already checked, check only others
                foundAllDevices = true;
                for (; reverseCounter > 0; reverseCounter--)
                {
                    if (messageArgs.find(deviceNames.at(reverseCounter)) ==
                        std::string::npos)
                    {
                        foundAllDevices = false;
                        break;
                    }
                }
            }
            if (foundAllDevices)
            {

                logs_err("Resolving log %s for device '%s' for event '%s'\n",
                         objectPath.first.str.c_str(), fullDeviceName.c_str(),
                         eventName.c_str());
                setLogEntryResolved(objectPath.first.str, bus);
            }
        } // for all objects/entries from phosphor logging
    }

    /**
     * @brief EventsDetection
     * @param pcTrigger the DataAccessor made from DBUS PropertyChange signal
     * @param possibleEventsPatternList list of possible event patterns
     * @return an EventCandidateList with the Events to be generated
     */
    EventCandidateList EventsDetection(
        const data_accessor::DataAccessor& pcTrigger,
        const std::vector<std::shared_ptr<event_info::EventNode>>&
            possibleEventPatternList)
    {
        EventCandidateList eventCandidateList;
        std::stringstream ss;
        pcTrigger.print(ss);
        log_dbg("In Detect Event Method for PC trigger %s\n", ss.str().c_str());
        std::vector<std::shared_ptr<event_info::EventNode>>  eventsOnlyPattern;

        std::unordered_set<std::string> recoveredEventDevices;
        std::unordered_set<std::string> rootCauseTracerDevice;
        unsigned recoveredCnt = 0;

        // this loop performs all recoveries first
        for (auto& eventPtr : possibleEventPatternList)
        {
            auto deviceType = eventPtr->getStringifiedDeviceType();
            bool passedRecoveryCheck = true;
            bool checkForRecovery = (!eventPtr->recovery_accessor.isEmpty()) &&
                                    (eventPtr->recovery_accessor == pcTrigger);
            if (checkForRecovery)
            {
                data_accessor::CheckAccessor recoveryCheck(deviceType);
                if (!recoveryCheck.check(eventPtr->recovery_accessor,
                                         pcTrigger))
                {
                    passedRecoveryCheck = false;
                }
                else
                {
                    log_dbg("Recovery Case: performing recovery actions for "
                            "event %s\n", eventPtr->event.c_str());

                    PROFILING_SWITCH(selftest::TsLatcher TS(
                        "event-detection-recovery-flow-" + eventPtr->event +
                        "-" + eventPtr->getMainDeviceType()));

                    for (auto& entry : recoveryCheck.getAssertedDevices())
                    {
                        // for multi devices, example:  "PCIeSwitch_0/Down_3"
                        auto fullDeviceName = eventPtr->getFullDeviceName(
                            entry.deviceIndexTuple);
                        if (fullDeviceName.empty())
                        {
                            log_err("Empty Device Name Event: '%s'\n",
                                    eventPtr->event.c_str());
                            continue;
                        }
                        auto eventFullKey = fullDeviceName + eventPtr->event;
                        if (recoveredEventDevices.count(eventFullKey) == 0)
                        {
                            recoveredEventDevices.insert(eventFullKey);
                            // "PCIeSwitch_0" if
                            //  fullDeviceName == "PCIeSwitch_0/Down_3
                            auto& mainDeviceKey = eventPtr->device;
                            if (rootCauseTracerDevice.count(mainDeviceKey) == 0)
                            {
                                rootCauseTracerDevice.insert(mainDeviceKey);
                                eventCandidateList.push_back(std::make_tuple(
                                   eventPtr, recoveryCheck.getAssertedDevices(),
                                   true));
                            }
                            else
                            {
                                log_dbg("skipping, RootCauseTracer already set "
                                        "for device %s", mainDeviceKey.c_str());
                            }
                            log_err("doing Recovery Device: '%s' Event: '%s'\n",
                               fullDeviceName.c_str(), eventPtr->event.c_str());
                            try
                            {
                                // devices.at(0) =  Phosphor logging namespace
                                recoveredCnt++;
                                resolveDeviceLogs(eventPtr->event, fullDeviceName);
                            }
                            catch (std::runtime_error& e)
                            {
                                log_err(
                                    "Failed to recover from fault %s on %s due "
                                    "to %s. Corresponding log may not have been"
                                    " resolved.\n", eventPtr->event.c_str(),
                                     fullDeviceName.c_str(),
                                    e.what());
                            }
                        }
                        else
                        {
                            log_dbg("skipping, already done Recovery "
                                    "Device:'%s' Event:'%s'\n",
                               fullDeviceName.c_str(), eventPtr->event.c_str());
                        }
                    }
                }
            }
            if (!checkForRecovery || !passedRecoveryCheck)
            {
                eventsOnlyPattern.push_back(eventPtr);
            }
        }
        auto rootCauseTracerCnt = eventCandidateList.size();

        // now perform the check only for events, recoveries already performed
        for (auto& eventPtr : eventsOnlyPattern)
        {
            auto deviceType = eventPtr->getStringifiedDeviceType();

            log_dbg("Standard Event Detection Case for event %s\n",
                    eventPtr->event.c_str());
            std::unique_ptr<data_accessor::CheckAccessor>
                checkObj(new data_accessor::CheckAccessor(deviceType));

            // eventPtr->trigger doesn't exist or pcTrigger is Boot Selftest
            if (eventPtr->accessor == pcTrigger)
            {
                checkObj->check(eventPtr->accessor, pcTrigger);
                log_dbg("Check=%d against event.accessor Event:'%s'\n",
                        checkObj->passed(), eventPtr->event.c_str());
            }
            else if (eventPtr->accessor.isEmpty())
            {
                // Only eventPtr->trigger exists, check it
                checkObj->check(eventPtr->trigger, pcTrigger);
                log_dbg("Check=%d against event.trigger Event:'%s'\n",
                        checkObj->passed(), eventPtr->event.c_str());
            }
            else
            {   // both event.trigger and event.accessor will be checked
                checkObj->check(eventPtr->trigger, eventPtr->accessor,
                                pcTrigger);
                log_dbg("Check=%d against both event.trigger and "
                        "event.accessor Event:'%s'\n",
                        checkObj->passed(), eventPtr->event.c_str());
            }

            auto check = checkObj.get();
            if (check != nullptr && check->passed())
            {
                log_dbg("asserted Event:'%s' Accessor: %s\n",
                        eventPtr->event.c_str(), ss.str().c_str());
                eventCandidateList.push_back(
                    std::make_tuple(
                        eventPtr, check->getAssertedDevices(),false));
            }
            else
            {
                log_dbg("NOT asserted Event:'%s' Accessor: %s\n",
                        eventPtr->event.c_str(), ss.str().c_str());
            }
        }

        auto assertedCnt = eventCandidateList.size() - rootCauseTracerCnt;
        log_dbg("[total] recovered=%u asserted=%u rootCauseTracers=%u\n",
                  recoveredCnt, assertedCnt, rootCauseTracerCnt);
        return eventCandidateList;
    }

    /**
     * @brief Check if the candidate is an event based on Leaky Bucket logic.
     *
     * @param candidate
     * @return true
     * @return false
     */
    bool IsEvent(event_info::EventNode& candidate,
                 const std::string& device,
                 int compareCount = invalidIntParam)
    {
        int count = candidate.count[device] + 1;
        if (candidate.valueAsCount)
        {
            count = (compareCount == -1) ? std::stoi(candidate.accessor.read())
                                         : compareCount;
        }

        if (candidate.triggerCount == -1)
        {
            return false;
        }

        auto countDiff = candidate.triggerCount - count;

        if (countDiff <= 0)
        {
            if (!candidate.valueAsCount)
            {
                candidate.count[device] = candidate.triggerCount;
            }
            return true;
        }

        if (!candidate.valueAsCount)
        {
            candidate.count[device]++;
        }
        return false;
    }

    /**
     * @brief Throw out a thread to run all event handlers on the Event.
     *
     * @param event
     */
    void RunEventHandler(event_info::EventNode& event, const std::string& name)
    {
        log_err("entered RunEventHandler\n");
        auto guard = std::make_shared<ThreadpoolGuard>(threadpoolManager.get());
        if (!guard->was_successful())
        {
            log_err(
                "Thread pool over maxTotal tasks limit, not creating thread\n");
            log_err("finished RunEventHandlers\n");
            return;
        }
        auto thread =
            std::make_unique<std::thread>([this, event, guard, name]() mutable {
                log_err("started event thread\n");
                std::stringstream ss;
                ss << "calling hdlrMgr: " << this->_hdlrMgr->getName()
                   << " event: " << event.event;
                log_dbg("%s\n", ss.str().c_str());
                auto hdlrMgr = *this->_hdlrMgr;
                hdlrMgr.RunHandler(event, name);
                log_err("finished event thread\n");
            });

        if (thread != nullptr)
        {
            thread->detach(); // separate, this no longer owns this thread
        }
        else
        {
            log_err("Create thread to process event failed!\n");
        }
        log_err("finished RunEventHandler\n");
    }

    /**
     * @brief Throw out a thread to run all event handlers on the Event.
     *
     * @param event
     */
    void RunEventHandlers(event_info::EventNode& event)
    {
        log_err("entered RunEventHandlers\n");
        auto guard = std::make_shared<ThreadpoolGuard>(threadpoolManager.get());
        if (!guard->was_successful())
        {
            // the threadpool has reached the max queued tasks limit,
            // don't run this event thread
            log_err(
                "Thread pool over maxTotal tasks limit, not creating thread\n");
            log_err("finished RunEventHandlers\n");
            return;
        }
        // capturing guard in the lambda is needed to extend the lifetime
        // of the ThreadpoolGuard so that it only gets destructed
        // (and the thread slot released) when both this trigger function
        // and the thread have finished executing.
        auto thread =
            std::make_unique<std::thread>([this, event, guard]() mutable {
                log_err("started event thread\n");
                std::stringstream ss;
                ss << "calling hdlrMgr: " << this->_hdlrMgr->getName()
                   << " event: " << event.event;
                log_dbg("%s\n", ss.str().c_str());
                auto hdlrMgr = *this->_hdlrMgr;
                hdlrMgr.RunAllHandlers(event);
                log_err("finished event thread\n");
            });

        if (thread != nullptr)
        {
            thread->detach(); // separate, this no longer owns this thread
        }
        else
        {
            log_err("Create thread to process event failed!\n");
        }
        log_err("finished RunEventHandlers\n");
    }

    /**
     * @brief  Check if an incoming D-bus signal is "interesting" (used by any
     * event)
     * @param  objectPath the D-Bus object path the signal occurred on
     * @param  msgInterface the D-Bus interface where a property changed
     * @param  eventProperty the name of the property whose value changed
     */
    bool getIsAccessorInteresting(const data_accessor::DataAccessor& accessor);

    /**
     * @brief Return trigger and vector of possible events
     *
     * @param  accessor the trigger/accessor which came from pc signal or
     * selftest, if selftest it must have had successful DataAccessor::read()

     * @param  bootup whether or not we are generating events from bootup
     * selftest,  selftest
     *
     * @return number of events pushed into the queue
     */
    static int eventDiscovery(const data_accessor::DataAccessor& accessor,
                               const bool& bootup = false);

  private:
    /**
     * @brief  Subscribe Accessor Dbus objects to receive PropertyChanged signal
     * @param  acc the DataAccessor
     * @param  map a global map for all events to make sure unique subscribing
     *           for a single object + interface
     * @param  conn the current Dbus IO connection
     * @param  The handler list of subscribing to be populated
     */
    void subscribeAcc(const data_accessor::DataAccessor& acc,
                      RegisteredObjectInterfaceMap& map,
                      std::shared_ptr<sdbusplus::asio::connection>& conn,
                      DbusEventHandlerList& handlerList);

  private:
    /**
     * @brief Pointer to the eventMap.
     *
     */
    event_info::EventMap* _eventMap;

    /**
     * @brief Pointer to the propertyFilterSet.
     * Despite object path being expandable, the possible options are finite
     * and thanks to util::expandDeviceRange(getDbusObjectPath()), we can
     * just add all possible tuples to a set
     */
    event_info::PropertyFilterSet* _propertyFilterSet;

    /**
     * @brief Pointer to the eventHanlderManager.
     *
     */
    event_handler::EventHandlerManager* _hdlrMgr;
};

} // namespace event_detection
