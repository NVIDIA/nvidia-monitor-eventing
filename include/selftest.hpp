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
#include "event_handler.hpp"

#include <dbus_utility.hpp>
#include <util.hpp>
#include <nlohmann/json.hpp>

#include <chrono>
#include <ostream>
#include <string>
#include <vector>

namespace selftest
{

/**
 * @brief TestPointResult contains single test point result attributes
 */
struct TestPointResult
{
    /** @brief this flag is needed to speed up result evaluation and used to
     *         skip TP result if set to true **/
    bool isTypeDevice;

    /** @brief test point severity **/
    util::Severity severity;

    /** @brief test point name, eg. "PGOOD", "DEVICE_HSC0" **/
    std::string targetName;

    /** @brief value read from accessor, eg. "OK", "HW_PIN_ABNORMAL_HIGH" **/
    std::string valRead;

    /** @brief DAT configured expected value, eg. "OK" **/
    std::string valExpected;

    /** @brief true when valRead equals valExpected **/
    bool result;
};

/**
 * @brief DeviceResult contains multiple test layer results of multiple TP
 * results
 */
struct DeviceResult
{
    /**
     * @brief eg. maps "power_rail" test layer onto its test points results
     * vector as there can be many TP's
     */
    std::map<std::string, std::vector<TestPointResult>> layer;
};

/**
 * @brief ReportResult is a container of multiple DeviceResults and its
 * underlying containers
 * @note  eg. DEV    |  layer      | test point  | testpoint result
 *            GPU0  -> power_rail
 *                                -> TP_PGOOD   -> (tp_result)
 *                                -> EXAMPLE_TP -> (tp_result)
 *                  -> pin_status
 *                                -> EXAMPLE_TP -> (...)
 *                  -> (...)
 *            HSC0  -> power_rail -> (...)
 *                  -> (...)
 *            (...)
 */
using ReportResult = std::map<std::string, DeviceResult>;

/**
 * @brief A class to generate aggregated report from ReportResult.
 */
class Report
{
  public:
    Report(std::function<std::string()> fwVerClbk) :
        tpTotal(0), tpFailed(0), getFwVersionClbk(fwVerClbk)
    {}
    ~Report() = default;

  public:
    /**
     * @brief Default implementation of fw version getter for report,
     *        read from dbus in this case.
     * @return fw version in text format
     */
    static std::string getDBusFwVersionString()
    {
        std::string fwVersion = "Failed to read firmware version.";
        auto propVariant = dbus::readDbusProperty(
            "/xyz/openbmc_project/software/HGX_FW_BMC_0",
            "xyz.openbmc_project.Software.Version", "Version");

        if (isValidVariant(propVariant))
        {
            fwVersion = data_accessor::PropertyValue(propVariant).getString();
        }

        return fwVersion;
    }

    /**
     * @brief Generates internal json report.
     *
     * @param[in] reportRes
     *
     * @return true when report successfuly generated, otherwise false
     */
    bool generateReport(ReportResult& reportRes);

    /**
     * @brief Returns internal json report object.
     *
     * @return json
     */
    const nlohmann::ordered_json& getReport(void);

    /**
     * @brief Output internal json report. Has to be generated first.
     *
     * @param os
     * @param rpt
     * @return std::ostream&
     */
    friend std::ostream& operator<<(std::ostream& os, const Report& rpt);

  private:
    /**
     * @brief Writes header data during report generation. Must be called in
     *        the end of report generation.
     */
    void writeSummaryHeader(void);

    /**
     * @brief used to process a layer's testpoints for a given device
     *
     * @param[out] jdev device to write given layer report piece to
     * @param[in] layerName layer key name (its support is validated internally)
     * @param[in] testpoints TPs results of given layer to process
     *
     * @return returns true on success, false on unsupported layer key
     */
    bool processLayer(nlohmann::ordered_json& jdev,
                      const std::string& layerName,
                      std::vector<selftest::TestPointResult>& testpoints);

    /** @brief Internal report storage. **/
    nlohmann::ordered_json _report;

    /** @brief Total test points number (counted during report generation) **/
    uint32_t tpTotal;

    /** @brief Failed test points count (counted during report generation) **/
    uint32_t tpFailed;

    /** @brief Method to fill report *version* field. **/
    std::function<std::string()> getFwVersionClbk = getDBusFwVersionString;
};

/**
 * @brief A class to perform selftest.
 */
class Selftest : public event_handler::EventHandler
{
  public:
    Selftest(const std::string& name,
             const std::map<std::string, dat_traverse::Device>& dat);
    ~Selftest() = default;

  public:
    /**
     * @brief Resolves DBUS log entry associated with device
     *
     * @param[in] device - name of device
     * @param[in] result - log results
     */
    void resolveLogEntry(const std::string& device,
                         const dbus::utility::ManagedObjectType& result) const;

    /**
     * @brief Updates device health on the DBUS
     *
     * @param[in] device - name of device
     * @param[in] health - health status of device
     */
    void updateDeviceHealth(const std::string& device,
                            const std::string& health) const;

    /**
     * @brief Updates healths on all devices present in results
     *
     * @param[in] reportRes
     */
    void updateHealthBasedOnResults(const ReportResult& reportRes);

    /**
     * @brief Aggregates selftest results of report->devices->testpoints
     * to simple ok or not ok.
     *
     * @param[in] reportRes
     * @return true - all TP in report passed, false - any TP failed
     */
    bool evaluateTestReport(const ReportResult& reportRes);

    /**
     * @brief Checks if a device report is already in report container.
     *
     * @param[in] devName device name to check presence in reportRes
     * @param[in] reportRes report container to check in for devName
     * @return true - entry already in reportRes, false - entry absent
     */
    bool isDeviceCached(const std::string& devName,
                        const ReportResult& reportRes);

    /**
     * @brief Performs selftest on device extracted from event and returns
     * already evaluated test report corectness, not test operation status.
     *
     * @param[in] event
     * @return eventing::RcCode::succ when all testpoints passed, otherwise
     * eventing::RcCode::error (failed TP or failed test operation)
     */
    eventing::RcCode process([[maybe_unused]] event_info::EventNode& event) override
    {
        if (_dat.count(event.device) == 0)
        {
            std::cerr << "Error: device: " << event.device
                      << " is an invalid key!" << std::endl;
            return eventing::RcCode::error;
        }

        ReportResult rep;
        const dat_traverse::Device& dev = _dat.at(event.device);

        if (perform(dev, rep) != eventing::RcCode::succ)
        {
            return eventing::RcCode::error;
        }

        if (!evaluateTestReport(rep))
        {
            return eventing::RcCode::error;
        }

        return eventing::RcCode::succ;
    };

    /** @brief Performs selftest on given device.
     *
     * @note the report container can be reused: as a cache to omit non unique
     * device tests; to stack multiple but not associated by TP devices for
     * report generation;
     * @param[in]  dev        - device to perform test on
     * @param[out] reportRes  - container with written in TP's results @ref
     * <ReportResult>
     * @param[in]  layersToIgnore - for these passed layer names testpoints are
     * skipped and only empty layers are included in the reportRes; default none
     * @return eventing::RcCode meaning testing operation status, not test results
     */
    eventing::RcCode perform(const dat_traverse::Device& dev,
                        ReportResult& reportRes,
                        std::vector<std::string> layersToIgnore = {},
                        const bool& doEventDetermination = false);

    /** @brief Performs selftest on entire DAT.
     *
     * @param[out] reportRes  - container with written in TP's results @ref
     * <ReportResult>
     * @param[in]  layersToIgnore - for these passed layer names testpoints are
     * skipped and only empty layers are included in the reportRes; default none
     * @return eventing::RcCode meaning testing operation status, not test results
     */
    eventing::RcCode performEntireTree(ReportResult& reportRes,
                                  std::vector<std::string> layersToIgnore = {},
                                  const bool& doEventDetermination = false);

    /**
     * @brief Checks selftest result of particular device -> testpoints
     *
     * @param[in] deviceResult
     * @return true - all TP in *device* passed, false - any TP failed
     */
    static bool evaluateDevice(const DeviceResult& deviceResult);

    /**
     * @brief based on device result in case of failed test points evaluate
     *        severity
     * @param[in] deviceResult
     * @return returns "OK" if no failures found or depending on test point
     * configuration: "Warning" or "Critical"
     */
    static std::string getDeviceTestResult(const DeviceResult& deviceResult);

  private:
    inline bool isDeviceRegular(const dat_traverse::Device& dev)
    {
        return dev.getType() == dat_traverse::DeviceType::types::REGULAR;
    }

    /** @brief Internal DAT reference. **/
    const std::map<std::string, dat_traverse::Device>& _dat;
};

#ifdef PROFILING_SELFTEST_AND_RECOVERY_FLOW
#define PROFILING_SWITCH(x) x
/**
 * @brief intended use is to create object (which latches start time) and
 * let it end its scope to print profiling data. More timepoints can be
 * added in the same scope with labels
 */
class TsLatcher
{
  public:
    TsLatcher(std::string name, std::string startingLabel = "start",
              std::string exitingLabel = "exit") :
        instanceName(name),
        startLabel(startingLabel), exitLabel(exitingLabel)
    {
        addTimepoint(startLabel);
    }

    ~TsLatcher()
    {
        addTimepoint(exitLabel);
        printSummary();
    }

  public:
    void addTimepoint(std::string label)
    {
        timepoints.push_back(
            std::make_pair(label, std::chrono::high_resolution_clock::now()));
    }

    void printSummary()
    {
        for (unsigned int i = 0; i < (timepoints.size() - 1); i++)
        {
            auto t1 = timepoints[i].second;
            auto t2 = timepoints[i + 1].second;
            auto ms_int =
                std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
            std::stringstream ss;
            ss << instanceName << ": " << timepoints[i].first << " <-> "
               << timepoints[i + 1].first << " took " << ms_int.count() << "ms";
            log_err("%s\n", ss.str().c_str());
        }
    }

  private:
    std::vector<std::pair<std::string,
                          decltype(std::chrono::high_resolution_clock::now())>>
        timepoints;
    std::string instanceName;
    std::string startLabel;
    std::string exitLabel;
};
#else
#define PROFILING_SWITCH(x)
#endif // #if PROFILE_SELFTEST_AND_RECOVERY_FLOW

} // namespace selftest

namespace event_handler
{

/**
 * @brief estimates if a device is implemented (in dbus)
 * @note TODO: to refactor, uses duplicated function from message composer
 *
 * @param[in] device - device name to check presence
 *
 * @return true when device possibly exists and is implemented in dbus,
 * otherwise false
 */
bool checkDeviceDBus(const std::string& device);

/**
 * @brief A class for finding the potential root cause of problem when an event
 * is received, it tests device and it associated devices through device
 * association tree.
 *
 */
class RootCauseTracer : public EventHandler
{
  public:
    RootCauseTracer(const std::string& name,
            std::map<std::string, dat_traverse::Device>& dat) :
        EventHandler(name),
        _dat(dat)
    {}

    ~RootCauseTracer() = default;

  public:
    /**
     * @brief does selftest on device taken from event and associated devices;
     * finds root cause of failure and updates health and origin of condition
     * of event device. Writes back test report to event.
     *
     * @param[in out] event - shall carry problematic device name; gets written
     * in selftest report of problematic device + its associated devices
     *
     * @return eventing::RcCode::succ when performed root cause tracing, otherwise
     * eventing::RcCode::error (wrong device name in event, performing selftest
     * failed). Warning - does not mean a root cause was found, but op success.
     */
    eventing::RcCode process([[maybe_unused]] event_info::EventNode& event) override;

  private:
    /**
     * @brief updates device root cause of failure
     *
     * @param[out] dev  - device to update health and origin of condition
     * @param[in] rootCauseDeviceName - device name which failed selftest
     * @param[in] selftester - self test object
     */
    void updateRootCause(dat_traverse::Device& dev,
                         std::string& rootCauseDeviceName,
                         std::map<std::string, dat_traverse::Device>& dat);

    /**
     * @brief finds most nested device that caused the problem
     *
     * @param[in] triggeringDevice - device that root cause search started from
     * @param[in] report - report result of selftest performed on
     * @triggeringDevice
     * @param[out] rootCauseDevice - returns failed device name if found,
     * otherwise empty
     *
     * @return true when found root cause, otherwise false (whole report
     * correct)
     */
    bool findRootCauseGeneral(const std::string& triggeringDevice,
                              const selftest::ReportResult& report,
                              const event_info::EventNode& eventNode,
                              std::string& rootCauseDevice);

    bool findRootCause(const std::string& triggeringDevice,
                       const selftest::ReportResult& report,
                       const event_info::EventNode& eventNode,
                       std::string& rootCauseDevice);

    /** @brief Internal DAT reference. **/
    std::map<std::string, dat_traverse::Device>& _dat;
};

} // namespace event_handler
