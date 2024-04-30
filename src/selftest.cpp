/**
 * Copyright (c) 2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */


#include "selftest.hpp"

#include "dbus_accessor.hpp"
#include "event_detection.hpp"
#include "property_accessor.hpp"
#include "util.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <dbus_log_utils.hpp>
#include <dbus_utility.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/exception.hpp>

#include <chrono>
#include <iostream>
#include <queue>
#include <variant>

namespace selftest
{

static constexpr auto reportResultPass = "Pass";
static constexpr auto reportResultFail = "Fail";
static data_accessor::PropertyString propertyValueResultFail(std::string{
    reportResultFail});

using phosphor::logging::entry;
using phosphor::logging::level;
using phosphor::logging::log;

void Selftest::resolveLogEntry(
    const std::string& device,
    const dbus::utility::ManagedObjectType& result) const
{
    log_dbg("IN SELFTEST RESOLVE LOG ENTRY looking for log with device %s\n",
            device.c_str());
    const std::vector<std::string>* additionalDataRaw = nullptr;
    bool resolved = false;
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
                    else if (propertyMap.first == "Resolved")
                    {
                        const bool* resolveptr =
                            std::get_if<bool>(&propertyMap.second);
                        if (resolveptr == nullptr)
                        {
                            log_dbg("Resolved Property pointer is null\n");
                            return;
                        }
                        resolved = *resolveptr;
                    }
                }
            }
        }

        std::string messageArgs;
        std::string recoveryType;
        std::string eventName;
        std::string deviceName;
        if (!resolved && additionalDataRaw != nullptr)
        {
            redfish::AdditionalData additional(*additionalDataRaw);
            if (additional.count("REDFISH_MESSAGE_ARGS") > 0)
            {
                messageArgs = additional["REDFISH_MESSAGE_ARGS"];
            }
            if (additional.count("RECOVERY_TYPE") > 0)
            {
                recoveryType = additional["RECOVERY_TYPE"];
            }
            if (additional.count("EVENT_NAME") > 0)
            {
                eventName = additional["EVENT_NAME"];
            }
            if (additional.count("DEVICE_NAME") > 0)
            {
                deviceName = additional["DEVICE_NAME"];
            }
            if (messageArgs.find(device) != std::string::npos &&
                recoveryType != std::string("property_change"))
            {
                log_dbg("FOUND LOG ENTRY TO RESOLVE\n");
                log_dbg("EVENT: %s\n", eventName.c_str());
                log_dbg("DEV: %s\n", deviceName.c_str());
                log_dbg("LOG: %s\n", objectPath.first.str.c_str());

                auto bus = sdbusplus::bus::new_default_system();
                try
                {
                    std::variant<bool> v = true;
                    dbus::DelayedMethod method(
                        bus, "xyz.openbmc_project.Logging",
                        objectPath.first.str, "org.freedesktop.DBus.Properties",
                        "Set");
                    method.append("xyz.openbmc_project.Logging.Entry");
                    method.append("Resolved");
                    method.append(v);
                    auto reply = method.call();
                }
                catch (const sdbusplus::exception::exception& e)
                {
                    log_err(" Dbus Error: %s\n", e.what());
                }
            }
        }
    }
}

void Selftest::updateDeviceHealth([[maybe_unused]] const std::string& device,
                                  [[maybe_unused]] const std::string& health) const
{
#ifdef EVENTING_SERVICE_NO_DEVICE_HEALTH
    log_err("not setting device Health: Device Health service is enabled\n");
#else
    if (_dat.at(device).canSetHealthOnDbus())
    {

        try
        {
            const std::string healthInterface(
                "xyz.openbmc_project.State.Decorator.Health");
            dbus::DirectObjectMapper om;
            std::vector<std::string> objPathsToAlter =
                om.getAllDevIdObjPaths(device, healthInterface);
            if (!objPathsToAlter.empty())
            {
                for (const auto& objPath : objPathsToAlter)
                {
                    std::string healthState =
                        "xyz.openbmc_project.State.Decorator.Health.HealthType." +
                        health;

                    log_dbg("Setting Health Property for: %s healthState: %s\n",
                            objPath.c_str(), healthState.c_str());
                    bool ok = dbus::setDbusProperty(
                        objPath, "xyz.openbmc_project.State.Decorator.Health",
                        "Health", PropertyVariant(healthState));
                    if (ok == true)
                    {
                        log_dbg("Changed health property as expected\n");
                    }
                }
            }
            else // ! objPathsToAlter.empty()
            {
                log_err("No object paths found in the subtree of "
                        "'xyz.openbmc_project.ObjectMapper' "
                        "corresponding to the '%s' device id "
                        "and implementing the '%s' interface\n",
                        device.c_str(), healthInterface.c_str());
            }
        }
        catch (const sdbusplus::exception::SdBusError& e)
        {
            std::cerr << "ERROR WITH SDBUSPLUS BUS " << e.what() << "\n";
            log<level::ERR>("Failed to establish sdbusplus connection",
                            entry("SDBUSERR=%s", e.what()));
        }
    }
#endif  // EVENTING_SERVICE_NO_DEVICE_HEALTH
}

void Selftest::updateHealthBasedOnResults([[maybe_unused]] const ReportResult& reportRes)
{
#ifdef EVENTING_SERVICE_NO_DEVICE_HEALTH
    log_err("not setting device Health: Device Health service is enabled\n");
#else
    for (auto& dev : reportRes)
    {
        auto severity = getDeviceTestResult(dev.second);
        updateDeviceHealth(dev.first, severity);
    }
#endif  // EVENTING_SERVICE_NO_DEVICE_HEALTH
}

bool Selftest::evaluateDevice(const DeviceResult& deviceResult)
{
    auto isTpFailed =
        [](TestPointResult tp) { /* return true on failed tp and false on ok */
                                 if (tp.isTypeDevice)
                                 { /* ignore result of children device */
                                     return false;
                                 }

                                 return !tp.result;
        };

    for (auto& layer : deviceResult.layer)
    {
        if (std::any_of(layer.second.begin(), layer.second.end(), isTpFailed))
        {
            return false;
        }
    }

    return true;
}

std::string Selftest::getDeviceTestResult(const DeviceResult& deviceResult)
{
    util::Severity worstSeverityFound(
        util::Severity::SEVERITY::SEVERITY_OK);
    for (auto& layer : deviceResult.layer)
    {
        auto testPoints = layer.second;
        for (auto& tp : testPoints)
        {
            if (tp.result || tp.isTypeDevice)
            {
                continue;
            }
            if (tp.severity.value() > worstSeverityFound.value())
            {
                worstSeverityFound.set_severity(tp.severity.value());
            }
        }
    }

    return worstSeverityFound.string();
}

bool Selftest::evaluateTestReport(const ReportResult& reportRes)
{
    for (auto& dev : reportRes)
    {
        if (!evaluateDevice(dev.second))
        {
            return false;
        }
    }

    return true;
}

bool Selftest::isDeviceCached(const std::string& devName,
                              const ReportResult& reportRes)
{
    if (reportRes.find(devName) == reportRes.end())
    {
        return false; /* not cached */
    }

    return true; /* already cached */
}

eventing::RcCode Selftest::perform(const dat_traverse::Device& dev,
                              ReportResult& reportRes,
                              std::vector<std::string> layersToIgnore,
                              const bool& doEventDetermination)
{
    shortlog_dbg(<< "selftest: device visited: '" << dev.name << "'");

    bool shouldSkip = !isDeviceRegular(dev) && !dev.hasTestpoints();
    if (isDeviceCached(dev.name, reportRes) || shouldSkip)
    {
        shortlog_dbg(<< "selftest: skipping device: '" << dev.name
                     << "' reason: "
                     << (shouldSkip ? "optimized" : "already tested"));
        return eventing::RcCode::succ;
    }

    if (doEventDetermination)
    {
        shortlog_dbg(<< "doing event determination for device: '" << dev.name);
    }

    auto fillTpRes =
        [](selftest::TestPointResult& tp, const std::string& expVal,
           const data_accessor::PropertyValue& readVal, const std::string& name,
           auto& severity, bool isDevice) {
            tp.targetName = name;
            tp.valExpected = expVal;
            tp.severity = severity;
            tp.isTypeDevice = isDevice;
            // it will empty when if DataAccessor::read() has failed
            if (readVal.empty())
            {
                tp.valRead = "Error - TP read failed.";
                tp.result = false;
            }
            else
            {
                tp.valRead = readVal.getString();
                // in case of empty expected value default to positive result
                tp.result =
                    (expVal.size() == 0)
                        ? true
                        : readVal == data_accessor::PropertyValue(expVal);
            }
        };

    PROFILING_SWITCH(selftest::TsLatcher TS("selftest-perform-" + dev.name));
    auto& availableLayers = dev.test;
    DeviceResult tmpDeviceReport;
    /* Important, preinsert new device test to detect recursed device. */
    reportRes[dev.name] = tmpDeviceReport;

    for (auto& tl : availableLayers)
    {
        auto& tmpLayerReport = tmpDeviceReport.layer[tl.first];

        if (std::find(layersToIgnore.begin(), layersToIgnore.end(), tl.first) !=
            layersToIgnore.end())
        {
            continue;
        }

        for (auto& tp : tl.second.testPoints)
        {
            auto& testPoint = tp.second;
            auto acc = testPoint.accessor;
            TestPointResult tmpTestPointResult;

            if (acc.isValidDeviceAccessor())
            {
                const std::string& devName = acc.read();
                if (this->_dat.count(devName) == 0)
                {
                    std::cerr << "Error: invalid device key: " << devName
                              << " in nested tp in selftest perform"
                              << std::endl;
                    return eventing::RcCode::error;
                }

                auto& devNested = this->_dat.at(devName);
                eventing::RcCode selftestRes = eventing::RcCode::succ;
                if (!isDeviceCached(devName, reportRes))
                {
                    selftestRes =
                        this->perform(devNested, reportRes, layersToIgnore,
                                      doEventDetermination);
                }

                bool devEvalRes = (selftestRes == eventing::RcCode::succ) &&
                                  evaluateDevice(reportRes[devName]);

                fillTpRes(tmpTestPointResult, testPoint.expectedValue,
                          (devEvalRes) ? data_accessor::PropertyValue(
                                             testPoint.expectedValue)
                                       : propertyValueResultFail,
                          devName, testPoint.severity, true);
            }
            else
            {
                acc.read(dev.name);
                fillTpRes(tmpTestPointResult, testPoint.expectedValue,
                          acc.getDataValue(), tp.first, testPoint.severity,
                          false);
            }
            tmpLayerReport.insert(tmpLayerReport.begin(), tmpTestPointResult);
            if (doEventDetermination && !tmpTestPointResult.result)
            {
                acc.setDevice(dev.name);
                event_detection::EventDetection::eventDiscovery(acc, true);
            }
        }
    }
    reportRes[dev.name] = tmpDeviceReport;
    return eventing::RcCode::succ;
}

eventing::RcCode Selftest::performEntireTree(ReportResult& reportRes,
                                        std::vector<std::string> layersToIgnore,
                                        const bool& doEventDetermination)
{
    PROFILING_SWITCH(selftest::TsLatcher TS("selftest-perform-entire-tree"));

    if (doEventDetermination)
    {
        for (auto& dev : _dat)
        {
            try
            {
                event_detection::EventDetection::resolveDeviceLogs(
                     std::string(""), dev.second.name);
            }
            catch (std::runtime_error& e)
            {
                log_err("Failed to resolve device logs for %s due to %s\n.",
                        dev.second.name.c_str(), e.what());
            }
        }
    }
    for (auto& dev : _dat)
    {
        if (perform(dev.second, reportRes, layersToIgnore,
                    doEventDetermination) != eventing::RcCode::succ)
        {
            return eventing::RcCode::error;
        }
    }
    return eventing::RcCode::succ;
}

Selftest::Selftest(const std::string& name,
                   const std::map<std::string, dat_traverse::Device>& dat) :
    event_handler::EventHandler(name),
    _dat(dat){};

/* ========================= report ========================= */

static std::map<std::string, std::string> layerToKeyLut = {
    {"power_rail", "power-rail-status"},
    {"erot_control", "erot-control-status"},
    {"pin_status", "pin-status"},
    {"interface_status", "interface-status"},
    {"firmware_status", "firmware-status"},
    {"protocol_status", "protocol-status"},
    {"data_dump", "data-dump"}};

std::ostream& operator<<(std::ostream& os, const Report& rpt)
{
    os << rpt._report.dump(4);
    return os;
}

static std::string getTimestampString(void)
{
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm* tm = std::localtime(&time);

    std::stringstream stream;
    stream << std::put_time(tm, "%Y-%m-%dT%H:%M:%SZ");
    return stream.str();
}

bool Report::processLayer(nlohmann::ordered_json& jdev,
                          const std::string& layerName,
                          std::vector<selftest::TestPointResult>& testpoints)
{
    if (layerToKeyLut.count(layerName) == 0)
    {
        std::cerr << "Error: generate report invalid layer key: " << layerName
                  << std::endl;
        return false;
    }

    const auto layerKey = layerToKeyLut.at(layerName);
    auto layerPassOrFail = true;
    jdev[layerKey]["test-points"] = nlohmann::json::array();

    for (auto& tp : testpoints)
    {
        this->tpTotal++;
        if (!tp.result)
        {
            this->tpFailed++;
            layerPassOrFail = false;
        }

        nlohmann::ordered_json newTp{};
        newTp["name"] = tp.targetName;

        if (tp.isTypeDevice)
        {
            newTp["device"] = true;
        }

        newTp["value"] = tp.valRead;

        if (tp.valExpected.size() != 0)
        {
            auto resStr = tp.result ? reportResultPass : reportResultFail;
            newTp["value-expected"] = tp.valExpected;
            newTp["result"] = resStr;
        }

        jdev[layerKey]["test-points"] += newTp;
    }

    jdev[layerKey]["result"] =
        layerPassOrFail ? reportResultPass : reportResultFail;

    return true;
}

bool Report::generateReport(ReportResult& reportRes)
{
    auto currentTimestamp = getTimestampString();
    this->tpTotal = 0;
    this->tpFailed = 0;

    /* preinsert primary keys to force keys order */
    this->_report["header"] = nlohmann::ordered_json::object();
    this->_report["tests"] = nlohmann::ordered_json::array();

    for (auto& dev : reportRes)
    {
        auto& devTestLayers = dev.second.layer;
        nlohmann::ordered_json jdev;
        jdev["device-name"] = dev.first;
        jdev["timestamp"] = currentTimestamp;

        for (auto& layer : devTestLayers)
        {
            /* to keep this key last, skip here and handle explicitly last */
            if (layer.first == "data_dump")
            {
                continue;
            }

            if (!processLayer(jdev, layer.first, layer.second))
            {
                return false;
            }
        }

        auto dataDumpLayer = devTestLayers.find("data_dump");
        if (dataDumpLayer != devTestLayers.end())
        {
            if (!processLayer(jdev, dataDumpLayer->first,
                              dataDumpLayer->second))
            {
                return false;
            }
        }

        this->_report["tests"] += (nlohmann::ordered_json)jdev;
    }

    this->writeSummaryHeader();

    return true;
}

const nlohmann::ordered_json& Report::getReport(void)
{
    return this->_report;
}

void Report::writeSummaryHeader(void)
{
    std::string fwVersion = "Reading fw version disabled.";
    if (getFwVersionClbk)
    {
        fwVersion = getFwVersionClbk();
    }

    this->_report["header"]["name"] = "Self test report";
    this->_report["header"]["version"] = "1.2";
    this->_report["header"]["HMC-version"] = fwVersion;
    this->_report["header"]["timestamp"] = getTimestampString();
    this->_report["header"]["summary"]["test-case-total"] = this->tpTotal;
    this->_report["header"]["summary"]["test-case-failed"] = this->tpFailed;
}

/* ========================= free function todo ========================= */

eventing::RcCode DoSelftest([[maybe_unused]] const dat_traverse::Device& dev,
                       [[maybe_unused]] const std::string& report)
{
    return eventing::RcCode::error;
}

} // namespace selftest

/* ========================= RootCauseTracer ========================= */

namespace event_handler
{

bool RootCauseTracer::findRootCause(const std::string& triggeringDevice,
                                    const selftest::ReportResult& report,
                                    const event_info::EventNode& eventNode,
                                    std::string& rootCauseDevice)
{
    if (eventNode.hasFixedOriginOfCondition())
    {
        rootCauseDevice = *eventNode.getOriginOfCondition();
        return true;
    }
    else
    {
        return findRootCauseGeneral(triggeringDevice, report, eventNode,
                                    rootCauseDevice);
    }
}

bool RootCauseTracer::findRootCauseGeneral(
    const std::string& triggeringDevice, const selftest::ReportResult& report,
    const event_info::EventNode& eventNode, std::string& rootCauseDevice)
{
    auto childVec = DATTraverse::getTestLayerSubAssociations(
        _dat, triggeringDevice, eventNode);

    /* test every device in order from most nested to least; first failed device
     * is the root cause of condition for device triggering the tracing */
    for (auto it = childVec.rbegin(); it != childVec.rend(); it++)
    {
        if (!selftest::Selftest::evaluateDevice(report.at(*it)))
        {
            rootCauseDevice = *it;
            return true;
        }
    }

    rootCauseDevice = triggeringDevice;
    return true;
}

void RootCauseTracer::updateRootCause(
    dat_traverse::Device& dev, std::string& rootCauseDeviceName,
    std::map<std::string, dat_traverse::Device>& dat)
{
    dat_traverse::Status status;
    status.health = "Critical";
    status.healthRollup = "Critical";
    if (dat.count(rootCauseDeviceName) > 0)
    {
        status.originOfCondition = dat.at(rootCauseDeviceName).name;
    }
    else
    {
        log_err("Root Cause Candidate %s not in DAT\n",
                rootCauseDeviceName.c_str());
    }
    status.triState = "Error";
    DATTraverse::setHealthProperties(dev, status);
    DATTraverse::setOriginOfCondition(dev, status);
}

eventing::RcCode RootCauseTracer::process([
    [maybe_unused]] event_info::EventNode& event)
{
    std::string problemDevice = event.device;
    if ((problemDevice.length() == 0) || (_dat.count(problemDevice) == 0))
    {
        std::cerr << "Error: rootCauseTracer device: [" << problemDevice
                  << "] is invalid key - cannot process rootCause" << std::endl;
        return eventing::RcCode::error;
    }

    PROFILING_SWITCH(
        selftest::TsLatcher TS("RootCauseTracer-proces-" + problemDevice));
    selftest::ReportResult completeReportRes;
    selftest::Selftest selftester("rootCauseSelftester", _dat);
    std::string rootCauseCandidateName{};

    /* TODO: refactor ignoring layers during selftest into selftest options
        tracked in design doc https://jirasw.nvidia.com/browse/DGXOPENBMC-5375
        current solution is quick, temporary, hardcoded patch */
    if (selftester.perform(_dat.at(problemDevice), completeReportRes,
                           std::vector<std::string>{"data_dump"}) !=
        eventing::RcCode::succ)
    {
        std::cerr << "Error: rootCauseTracer failed to perform selftest "
                  << "for device " << problemDevice << std::endl;
        return eventing::RcCode::error;
    }

    /* append to already existing event severities a selftest severity then find
    worst */
    PROFILING_SWITCH(TS.addTimepoint("update health"));
    auto selftestSeverity = selftester.getDeviceTestResult(
                                            completeReportRes[problemDevice]);
    event.severities.push_back(selftestSeverity);

    std::string health = util::Severity::findMaxSeverity(event.severities);

    selftester.updateDeviceHealth(problemDevice, health);

    PROFILING_SWITCH(TS.addTimepoint("find root cause"));

    if (findRootCause(problemDevice, completeReportRes, event,
                      rootCauseCandidateName))
    {
        // TODO: revisit if calling updateRootCause is necessary
        updateRootCause(_dat.at(problemDevice), rootCauseCandidateName, _dat);
        event.setOriginOfCondition(rootCauseCandidateName);
    }

    PROFILING_SWITCH(TS.addTimepoint("generate report"));

    selftest::Report reportGenerator(selftest::Report::getDBusFwVersionString);
    if (!reportGenerator.generateReport(completeReportRes))
    {
        std::cerr << "Error: rootCauseTracer failed to generate report!"
                  << std::endl;
        return eventing::RcCode::error;
    }

    event.selftestReport = reportGenerator.getReport();

    return eventing::RcCode::succ;
}

} // namespace event_handler
