/**
 * Copyright (c) 2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#include "check_accessor.hpp"

namespace data_accessor
{

bool CheckAccessor::buildSingleAssertedDeviceName(const DataAccessor& dataAcc,
                                                  const std::string& realDevice,
                                                  const int deviceId)
{
    if (deviceId == util::InvalidDeviceId)
    {
        return buildSingleAssertedDeviceName(dataAcc, realDevice,
                                             _devIdData.index);
    }
    return buildSingleAssertedDeviceName(dataAcc, realDevice,
                                         device_id::PatternIndex(deviceId));
}

std::string CheckAccessor::getCurrentDevice()
{
    std::string currentDevice{""};
    if (_devIdData.pattern.dim() > 0)
    {
        currentDevice = util::determineDeviceName(
            _devIdData.pattern, _devIdData.index);
    }
    if (currentDevice.empty() && false == _triggerAssertedDevice.empty())
    {
        currentDevice = _triggerAssertedDevice;
    }
    return currentDevice;
}

bool CheckAccessor::buildSingleAssertedDeviceName(
    const DataAccessor& dataAcc, const std::string& realDevice,
    const device_id::PatternIndex& patternIndex)
{
    bool ret = false;
    auto deviceName = realDevice;
    if (deviceName.empty() && util::existsRange(_devIdData.pattern) == false)
    {
        deviceName =
            util::determineDeviceName(_devIdData.pattern, patternIndex);
    }
    if (false == deviceName.empty())
    {
        _assertedDevices.push_back(
            AssertedDevice(_trigger, dataAcc, deviceName, patternIndex));
        ret = true;
    }
    return ret;
}

bool CheckAccessor::loopDevices(const DeviceIndexesList& deviceIndexes,
                                const DataAccessor& jsonAcc,
                                DataAccessor& dataAcc)
{
    bool ret = false;
    for (auto& index : deviceIndexes)
    {
        auto deviceName = this->_devIdData.pattern.eval(index);
        dataAcc.read(deviceName, &index);
        if (subCheck(jsonAcc, dataAcc, deviceName, index[0]))
        {
            ret = true; // it at least one passes the entire check also passes
        }
    }
    return ret;
}

bool CheckAccessor::check(const DataAccessor& jsonAcc, // template Accessor
                          const DataAccessor& dataAcc) // trigger  Accessor
{
    auto tempAccData = dataAcc;
     _lastStatus = NotPassed;
    // both are DBUS, as dataAcc is always DBUS,
    // calls util::determineDeviceIndex only if device_type also has range
    if (jsonAcc.isTypeDbus())
    {
        if (util::existsRange(_devIdData.pattern))
        {
            // get the real device index (event multiple devices)
            auto templateAccessorObj = jsonAcc.getDbusObjectPath();
            auto triggerAccessorObj = dataAcc.getDbusObjectPath();
            if (!templateAccessorObj.empty() && !triggerAccessorObj.empty())
            {
                device_id::DeviceIdPattern jsonObjPattern(templateAccessorObj);
                _devIdData.index =
                 util::determineDeviceIndex(jsonObjPattern, triggerAccessorObj);
            }
        }
    }
    else if (jsonAcc.isTypeCmdline())
    {
        device_id::DeviceIdPattern jsonArgPattern(jsonAcc.getArguments());
        device_id::DeviceIdPattern dataArgPattern(dataAcc.getArguments());
        if (util::existsRange(jsonArgPattern) &&
            false == util::existsRange(dataArgPattern))
        {
            _devIdData.index = util::determineDeviceIndex(
                              jsonArgPattern, dataAcc.getArguments());
            if (dataAcc.hasData())
            {
                auto device = util::determineDeviceName(_devIdData.pattern,
                                                        _devIdData.index);
                return
                    subCheck(jsonAcc, tempAccData, device, _devIdData.index[0]);
            }
        }
    }
    else if (dataAcc.hasData() && jsonAcc.isTypeDeviceCoreApi())
    {
        auto device = dataAcc.getDevice();
        if (!device.empty())
        {
            auto deviceId = util::getDeviceId(device);
            return subCheck(jsonAcc, tempAccData, device, deviceId);
        }
        else
        {
            std::stringstream ss;
            dataAcc.print(ss);
            log_err("Device unknown for the Accessor: %s\n", ss.str().c_str());
        }
    }
    return privCheck(jsonAcc, tempAccData);
}

bool CheckAccessor::check(const DataAccessor& jsonTrig,
                          const DataAccessor& jsonAcc,
                          const DataAccessor& dataAcc)
{
    auto eventType = this->_devIdData.pattern.pattern();
    CheckAccessor triggerCheck(eventType);
    triggerCheck.check(jsonTrig, dataAcc);
    if (triggerCheck.passed())
    {
        CheckAccessor accessorCheck(eventType);
        auto accessorJsonAcc = jsonAcc;
        // accessorJsonAcc contains check criteria and will also contain data
        if (accessorJsonAcc == dataAcc)
        {
            // avoids calling Accessor::read()
            accessorJsonAcc.setDataValue(jsonAcc.getDataValue());
        }
        // else Accessor::read() will called for accessorJsonAcc
        accessorCheck.check(accessorJsonAcc, triggerCheck);
        // save current check() result from accessorCheck
        *this = accessorCheck;
    }
    else
    {
        // save current check() result from triggerCheck
        *this = triggerCheck;
    }
    return passed();
}

bool CheckAccessor::privCheck(const DataAccessor& jsonAcc,
                              DataAccessor& dataAcc)
{
    bool ret = true; // defaults to true if accessor["check"] does not exist
    // set the status here
    _lastStatus = NotPassed;
    if (_trigger.isEmpty())
    {
        _trigger = dataAcc;
    }
    std::string deviceToRead = this->getCurrentDevice();

    if (jsonAcc.existsCheckKey() == true)
    {
        ret = false;
        /*
         * Note:
         *   As read(device) is for a single device, there are cases where
         *   it is necessary to loop all devices calling read() and subCheck()
         *   These cases are:
         *   1. CMDLINE i.g., "arguments": "AP0_BOOTCOMPLETE_TIMEOUT GPU[0-7]",
         *   2. DeviceCoreAPI having range in event 'device_type', not having
         *      “device_id” field or having “device_id” equal “range”
         */
        std::vector<device_id::PatternIndex> devRange{};
        if (true == deviceToRead.empty())
        {
            if (jsonAcc.isTypeCmdline() &&
                    util::existsRange(jsonAcc.getArguments()))
            {
                devRange = _devIdData.pattern.domainVec();
            }
            else if (jsonAcc.isTypeDeviceCoreApi() && jsonAcc.isDeviceIdRange())
            {
                // working so far with 'equal' and 'bitmask'
                devRange = _devIdData.pattern.domainVec();
            }
        }
        if (devRange.size() > 0)
        {
            DataAccessor tempDataAcc = jsonAcc; // tempDataAcc will have data in
            ret = loopDevices(devRange, jsonAcc, tempDataAcc);
        }
        else // not necessary to loop all devices, using original deviceToRead
        {
            if (dataAcc.isTypeDbus() == false || dataAcc.hasData() == false)
            {
                dataAcc.read(deviceToRead, &_devIdData.index);
            }
            ret = subCheck(jsonAcc, dataAcc, deviceToRead);
        }
    }
    else
    {
        // there is not "check" in the dataAcc, then needs to get devices
        buildSingleAssertedDeviceName(dataAcc, deviceToRead, _devIdData.index);
    }
    if (ret == true)
    {
        _lastStatus = Passed;
    }

    // log stuff
    unsigned long size = _assertedDevices.size();
    log_dbg("asserted vector size=%lu\n", size);
    for (auto& deviceData : _assertedDevices)
    {
        log_dbg("device=%s data=%s\n", deviceData.device.c_str(),
                deviceData.accessor.getDataValue().getString().c_str());
    }
    return ret;
}

bool CheckAccessor::check(const DataAccessor& jsonAcc,
                          const CheckAccessor& triggerCheck)
{
    _trigger = triggerCheck._trigger;
    // this accessor will contains data if that passes
    auto accessorData = jsonAcc;
    // using trigger Check assertedDevice
    if (triggerCheck.hasAssertedDevices())
    {
        auto& firstAssertedDevice = triggerCheck._assertedDevices.at(0);
        _triggerAssertedDevice = firstAssertedDevice.device;
        _devIdData = triggerCheck._devIdData;
    }
    privCheck(accessorData, accessorData);

    // prevent the cases when:
    //   1.  event.trigger exists and event.accessor does NOT exist
    //   2.  could NOT build assertedDevice in this Check but triggerCheck could
    //          example for (2) is "DRAM Contained ECC Error"
    if (passed() && hasAssertedDevices() == false)
    {
        _assertedDevices = triggerCheck._assertedDevices;
        if (accessorData.hasData() == true)
        {
            for (auto& asserteDevice : _assertedDevices)
            {
                // trigger, device and deviceId are already there, sets accessor
                asserteDevice.accessor = accessorData;
            }
        }
    }
    return passed();
}

bool CheckAccessor::subCheck(const DataAccessor& jsonAcc, DataAccessor& dataAcc,
                             const std::string& dev2Read, const int deviceId)
{
    if (dataAcc.hasData() == false)
    {
        return false; // without data nothing to do
    }
    bool ret = false;
    auto checkMap = jsonAcc[checkKey].get<CheckDefinitionMap>();
    /**
     * special logic for bitmap
     * --------------------------
     *    bitmap vaulue is checked for every device from 'event.device_type'
     *    exemple in json file:
     *         "check": { "bitmap": "1" }
     */
    if (jsonAcc.existsCheckBitmap() == true)
    {
        PropertyValue bitmapValue;
        auto jsonBitmapValue = jsonAcc[checkKey][bitmapKey].get<std::string>();
        bitmapValue = PropertyValue(jsonBitmapValue);
        if (bitmapValue.isValidInteger() == true)
        {
            auto devRange = _devIdData.pattern.domainVec();
            // now walk thuru devices using a zero based index to shift bits
            int zero_index_bit_shift = 0;
            for (auto& index : devRange)
            {
                PropertyVariant bitmask(bitmapValue.getInteger() << zero_index_bit_shift++);
                if (dataAcc.getDataValue().check(checkMap, bitmask) == true)
                {
                    ret = true;
                    auto deviceName = this->_devIdData.pattern.eval(index);
                    buildSingleAssertedDeviceName(dataAcc, deviceName, index);
                }
            }
        }
    }
    else if (dataAcc.getDataValue().check(checkMap, PropertyVariant()))
    {
        ret = true;
        buildSingleAssertedDeviceName(dataAcc, dev2Read, deviceId);
    }
    if (ret == true)
    {
        _lastStatus = Passed;
    }
    return ret;
}

} // namespace data_accessor
