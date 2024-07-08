/**
 * Copyright (c) 2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#include "data_accessor.hpp"

#include "event_info.hpp"
#include "log.hpp"

#include <boost/process.hpp>

#include <regex>
#include <string>
#include <vector>

namespace data_accessor
{

bool DataAccessor::contains(const DataAccessor& other) const
{
    bool ret = isValid(other._acc);
    std::stringstream ss;
    if (ret == true)
    {
        for (auto& [key, val] : other._acc.items())
        {
            if (_acc.count(key) == 0)
            {
                ss << "key=" << key << " not found in _acc";
                log_dbg("%s\n", ss.str().c_str());
                ret = false;
                break;
            }
            // this may have
            auto regex_string = _acc[key].get<std::string>();
            auto val_string = val.get<std::string>();
            const std::regex reg_value{regex_string};
            auto values_match = std::regex_match(val_string, reg_value);
            if (values_match == false)
            {
                ss.str(std::string()); // Clearing the stream first
                ss << "values do not match key=" << key << " value=" << val;
                log_dbg("%s\n", ss.str().c_str());
                ret = false;
                break;
            }
        }
    }
    ss.str(std::string()); // Clearing the stream first
    ss << "ret=" << ret;
    log_dbg("%s\n", ss.str().c_str());
    return ret;
}


InterfaceObjectsMap DataAccessor::getDbusInterfaceObjectsMap() const
{
    InterfaceObjectsMap ret;
    auto interface = getDbusInterface();
    if (interface.empty() == false)
    {
        std::vector<std::string> list;
        auto deviceList = util::expandDeviceRange(getDbusObjectPath());
        for (const auto& device : deviceList)
        {
            list.push_back(device.second);
        }
        ret[interface] = list;
    }
    return ret;
}

bool DataAccessor::readDbus(const device_id::PatternIndex* devIndex)
{
    log_elapsed();
    clearData();
    bool ret = isValidDbusAccessor();
    if (ret == true)
    {
        /*
         * readDbusProperty() returns empty variant for the following cases:
         *     1. bad return from getService() -> empty service
         *     2. object path with range specification
         * readDbusProperty() forwards an exception when receives it from Dbus
         */
        std::string objPath = _acc[objectKey].get<std::string>();
        if (util::existsRange(objPath) == true && devIndex != nullptr)
        {
            // apply the device into the "object" to replace the range
            objPath =
                util::introduceDeviceInObjectpath(objPath, *devIndex);
        }
        auto propVariant = dbus::readDbusProperty(
            objPath, _acc[interfaceKey], _acc[propertyKey]);
        // setDataValueFromVariant returns false in case variant is invalid
        ret = setDataValueFromVariant(propVariant);
    }
    return ret;
}

bool DataAccessor::runCommandLine(const device_id::PatternIndex* devIndex)
{
    log_elapsed();
    clearData();
    bool ret = isValidCmdlineAccessor();
    if (ret == true)
    {
        auto cmd = getExecutable();
        if (_acc.count(argumentsKey) != 0)
        {
            auto args = getArguments();
            if (devIndex != nullptr)
            {
                // if args does not have range, it does nothing
                args = util::introduceDeviceInObjectpath(args, *devIndex);
            }
            else // expand the range inside args
            {
                // TODO: "before A[0-3] after" => "before A0 A1 A2 A3 after"
                // args = expandRangeFromString(args);
            }
            cmd += ' ' + args;
        }
        std::stringstream ss;
        std::string result{""};
        uint64_t processExitCode = 0;
        try
        {
            std::string line{""};
            log_elapsed("running cmd: %s", cmd.c_str());
            boost::process::ipstream pipe_stream;
            boost::process::group g;
            boost::process::child process(cmd, g, boost::process::std_out > pipe_stream);
            int waits_remaining = (SUBPROCESS_RUNNING_TIMEOUT_MS / SUBPROCESS_RUNNING_POLL_MS);
            while (process.running() && waits_remaining > 0)
            {
                //std::cerr << "process still running, " << waits_remaining << " waits remaining" << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(SUBPROCESS_RUNNING_POLL_MS));
                waits_remaining--;
            }
            std::error_code ec;
            if (process.running())
            {
                log_err("process still running, going to terminate group\n");
                // terminate process and any subprocesses it launched as well
                g.terminate(ec);
                // prevent direct child becoming zombie (grandchildren are reparented
                // to init which reaps them)
                process.wait();
                if (ec && ec.value() != ESRCH)
                {
                    throw std::runtime_error("error terminating subprocess group: " + ec.message());
                }
                throw std::runtime_error("child process timed out and was terminated!");
            }
            // store main process's exit code now, before cleanup
            process.wait();
            // make sure children (if any) are cleaned up once the main process exits
            // since these are indirect children, they will be reparented to init
            // which reaps them.
            g.terminate(ec);
            if (ec && ec.value() != ESRCH)
            {
                throw std::runtime_error("error terminating subprocess group: " + ec.message());
            }
            processExitCode = static_cast<uint64_t>(process.exit_code());
            log_dbg("returnCode=%llu cmd='%s'\n", processExitCode, cmd.c_str());
            if (processExitCode != 0)
            {
                ret = false;
                std::string error("return code = ");
                error += std::to_string(processExitCode);
                ss.str(std::string()); // Clear the stream
                ss << "Error running the command \'" << cmd << "\' " << error;
                log_err("%s\n", ss.str().c_str());
                return ret;
            }
            while (pipe_stream && std::getline(pipe_stream, line) &&
                   line.empty() == false)
            {
                result += line;
            }
        }
        catch (const std::exception& error)
        {
            ret = false;
            ss.str(std::string()); // Clear the stream
            ss << "Error running the command \'" << cmd << "\' "
               << error.what();
            log_err("%s\n", ss.str().c_str());
        }
        // ok lets store the output
        if (ret == true)
        {
            _dataValue = PropertyValue(result);
        }
    }
    return ret;
}

bool DataAccessor::setDataValueFromVariant(const PropertyVariant& propVariant)
{
    clearData();
    bool ret = isValidVariant(propVariant);
    if (ret == true)
    {
        _dataValue = PropertyValue(propVariant);
    }
    return ret;
}

bool DataAccessor::readDeviceCoreApi(const std::string& device)
{
    log_elapsed();
    clearData();
    bool ret = isValidDeviceCoreApiAccessor();
    if (ret == true)
    {
        auto deviceId = util::getMappedDeviceId(device);
        if (deviceId == util::InvalidDeviceId)
        {
            // not all devices are mapped, ten use common getDeviceId()
            deviceId = util::getDeviceId(device);
        }
        log_elapsed("deviceId=%d api='%s'", deviceId, device.c_str());
        auto tuple = dbus::deviceGetCoreAPI(deviceId, _acc[propertyKey]);
        auto rc = std::get<int>(tuple);
        if (rc != 0)
        {
            ret = false;
        }
        else
        {
            if (existsCheckLookup() == true && isDeviceIdRange() == false)
            {
                _dataValue = PropertyValue(std::get<std::string>(tuple),
                                           std::get<uint64_t>(tuple));
            }
            else
            {
                _dataValue = PropertyValue(std::get<uint64_t>(tuple));
            }
        }
    }
    return ret;
}

/**
 *   OtherAcc may be the same accessor, or have at least
 *          same "object" not considering the range specification
 *
 * @example
 *   Same "object", same "interface" but different "property"
 *
 * this                                otherAcc
   ------                              ----------
   {                                   {
    "type": "DBUS",                      "type": "DBUS",
    "object": "/temp/GPU_SXM_[1-8]",     "object": "/temp/GPU_SXM_3",
    "interface": "xyz.Sensor",           "interface": "xyz.Sensor",
    "property": "CriticalHigh"           "property": "CriticalLow"
   }                                   }
 */
std::string DataAccessor::readUsingMainAccessor(const DataAccessor& otherAcc)
{
    std::string ret{""};
    // (1) case
    //  OtherAcc is the same accessor as *this, just copy otherAcc data
    if (otherAcc.hasData() && *this == otherAcc)
    {
        _dataValue = otherAcc._dataValue;
        ret =  _dataValue.getString();
    }
    // (2) case
    // otherAcc data does not matter, trying to get otherAcc "object" value
    // example above: same "object", same "interface" but different "property"
    else if (this->isTypeDbus() && otherAcc.isTypeDbus())
    {
        auto objPath = this->getDbusObjectPath();
        auto otherObjPath = otherAcc.getDbusObjectPath();
        if (util::existsRange(objPath) && !util::existsRange(otherObjPath) &&
                util::matchRegexString(objPath, otherObjPath))
        {
            // build another Accessor using otherAcc object path without range
            auto jsonData = _acc;
            jsonData[data_accessor::objectKey] =  otherObjPath;
            DataAccessor tmpAcc(jsonData);
            ret = tmpAcc.read();
            _dataValue = tmpAcc._dataValue; // copy tmpAcc data
        }
    }
    std::stringstream ss;
    ss << "ret: '" << ret << "'"
       <<  "\n\t_acc: " << _acc
       << "\n\totherAcc: " << otherAcc;
    log_dbg("%s\n", ss.str().c_str());
    return ret;
}

std::vector<DataAccessor> DataAccessor::expand() const
{
    std::vector<DataAccessor> accessorList;
    // expansionType assumes: 1 for DBUS, 2 for CMDLINE, 0 for none of them
    auto expansionType = isTypeDbus() ? 1 : 0;
    if (isTypeCmdline())
    {
        expansionType = 2;        
    }
    if (expansionType > 0)
    {
        nlohmann::json json = this->_acc;
        std::string jKey =  expansionType == 1 ? data_accessor::objectKey :
                                                 data_accessor::argumentsKey;
        std::string jValue = expansionType == 1 ? this->getDbusObjectPath() :
                                                  this->getArguments();
        device_id::DeviceIdPattern pattern(jValue);
        for (auto& domainItemIndex : pattern.domainVec())
        {
            json[jKey] = pattern.eval(domainItemIndex);           
            accessorList.push_back(DataAccessor(json, this->getDataValue()));         
        }
    }
    return accessorList;
}

std::string DataAccessor::read(const event_info::EventNode& event)
{
    if (isTypeDeviceCoreApi() || isTypeCmdline() || isTypeDbus())
    {
        auto result = readUsingMainAccessor(event.accessor);
        if (result.empty())
        {
            result = readUsingMainAccessor(event.trigger);
        }
        if (false == result.empty())
        {
            return result;
        }
        util::DeviceIdData devIdData = event.getDataDeviceType();
        // common DataAccessor::read() if attempts above failed
        return read(event.device, &devIdData.index);
    }
    else
    if (isTypeDevice() || isTypeTest() || isTypeConst() || isTypeDeviceName())
    {
        // DataAccessor::read() decides when to use device parameter
        return read(event.device);
    }
    else
    {
        throw std::runtime_error(
            "Unrecognised data accessor type. "
            "If a new accessor type was added a change in '" __FILE__
            ":getStringMessageArg' is needed");
    }
}

} // namespace data_accessor
