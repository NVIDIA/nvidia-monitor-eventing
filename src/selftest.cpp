
/*
 *
 */

#include "selftest.hpp"

#include <chrono>
#include <iostream>

namespace selftest
{

static constexpr auto reportResultPass = "Pass";
static constexpr auto reportResultFail = "Fail";

bool Selftest::evaluateTestReport(const ReportResult& reportRes)
{
    for (auto& dev : reportRes)
    {
        for (auto& layer : dev.second.layer)
        {
            for (auto& tp : layer.second)
            {
                if (!tp.result)
                {
                    return false;
                }
            }
        }
    }
    return true;
};

bool Selftest::isDeviceCached(const std::string& devName,
                              const ReportResult& reportRes)
{
    if (reportRes.find(devName) == reportRes.end())
    {
        return false; /* not cached */
    }

    return true; /* already cached */
}

aml::RcCode Selftest::perform(const dat_traverse::Device& dev,
                              ReportResult& reportRes)
{
    if (isDeviceCached(dev.name, reportRes))
    {
        return aml::RcCode::succ; /* early exit, non unique device test */
    }

    auto fillTpRes = [](selftest::TestPointResult& tp,
                        const std::string& expVal, const std::string& readVal,
                        const std::string& name) {
        tp.valRead = readVal;
        tp.valExpected = expVal;
        tp.result = tp.valRead == tp.valExpected;
        tp.targetName = name;
    };

    auto& availableLayers = dev.test;
    DeviceResult tmpDeviceReport;
    /* Important, preinsert new device test to detect recursed device. */
    reportRes[dev.name] = tmpDeviceReport;

    for (auto& tl : availableLayers)
    {
        auto& tmpLayerReport = tmpDeviceReport.layer[tl.first];

        for (auto& tp : tl.second.testPoints)
        {
            auto& testPoint = tp.second;
            auto acc = testPoint.accessor;
            TestPointResult tmpTestPointResult;

            if (acc.isValidDeviceAccessor())
            {
                const std::string& devName = acc.read();
                auto& devNested = this->_dat.at(devName);
                aml::RcCode stRes = aml::RcCode::succ;
                if (!isDeviceCached(devName, reportRes))
                {
                    stRes = this->perform(devNested, reportRes);
                }
                fillTpRes(tmpTestPointResult, testPoint.expectedValue,
                          (stRes == aml::RcCode::succ) ? testPoint.expectedValue
                                                       : reportResultFail,
                          devName);
            }
            else
            {
                auto accRead = acc.read();
                fillTpRes(tmpTestPointResult, testPoint.expectedValue, accRead,
                          tp.first);
            }
            tmpLayerReport.insert(tmpLayerReport.begin(), tmpTestPointResult);
        }
    }
    reportRes[dev.name] = tmpDeviceReport;
    return aml::RcCode::succ;
}

aml::RcCode Selftest::performEntireTree(ReportResult& reportRes)
{
    for (auto& dev : _dat)
    {
        if (perform(dev.second, reportRes) != aml::RcCode::succ)
        {
            return aml::RcCode::error;
        }
    }

    return aml::RcCode::succ;
}

Selftest::Selftest(const std::string& name,
                   const std::map<std::string, dat_traverse::Device>& dat) :
    event_handler::EventHandler(name),
    _dat(dat){};

/* ========================= report ========================= */

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
    stream << std::put_time(tm, "%Y-%m-%d %H:%M:%SZ");
    return stream.str();
}

void Report::generateReport(ReportResult& reportRes)
{
    const std::map<std::string, std::string> layerToKeyLut = {
        {"power_rail", "power-rail-status"},
        {"erot_control", "erot-control-status"},
        {"pin_status", "pin-status"},
        {"interface_status", "interface-status"},
        {"firmware_status", "firmware-status"},
        {"protocol_status", "protocol-status"}};

    auto tpTotal = 0;
    auto tpFailed = 0;
    auto currentTimestamp = getTimestampString();

    this->_report["header"]["name"] = "Self test report";
    this->_report["header"]["version"] = "1.0";
    this->_report["header"]["timestamp"] = currentTimestamp;
    this->_report["tests"] = nlohmann::json::array();

    for (auto& dev : reportRes)
    {
        nlohmann::json jdev;
        jdev["device-name"] = dev.first;
        jdev["firmware-version"] = "<todo>";
        jdev["timestamp"] = currentTimestamp;

        for (auto& layer : dev.second.layer)
        {
            const auto layerKey = layerToKeyLut.at(layer.first);
            auto layerPassOrFail = true;
            jdev[layerKey]["test-points"] = nlohmann::json::array();

            for (auto& tp : layer.second)
            {
                tpTotal++;

                if (!tp.result)
                {
                    tpFailed++;
                    layerPassOrFail = false;
                }

                jdev[layerKey]["test-points"] +=
                    {{"name", tp.targetName},
                     {"value", tp.valRead},
                     {"value-expected", tp.valExpected},
                     {"result",
                      tp.result ? reportResultPass : reportResultFail}};
            }
            jdev[layerKey]["result"] =
                layerPassOrFail ? reportResultPass : reportResultFail;
        }

        this->_report["tests"] += jdev;
    }

    this->_report["header"]["summary"]["test-case-total"] = tpTotal;
    this->_report["header"]["summary"]["test-case-failed"] = tpFailed;
}

const nlohmann::json& Report::getReport(void)
{
    return this->_report;
}

/* ========================= free function todo ========================= */

aml::RcCode DoSelftest([[maybe_unused]] const dat_traverse::Device& dev,
                       [[maybe_unused]] const std::string& report)
{
    return aml::RcCode::error;
}

} // namespace selftest
