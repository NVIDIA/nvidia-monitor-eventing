/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include "device_util.hpp"

#include "log.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>

namespace util
{

bool existsRange(const device_id::DeviceIdPattern& patternObj)
{
    return patternObj.dim() > 0;
}

bool existsRange(const std::string& str)
{
    device_id::DeviceIdPattern devicePattern(str);
    return existsRange(devicePattern);
}

DeviceIdMap expandDeviceRange(const device_id::DeviceIdPattern& patternObj)
{
    DeviceIdMap deviceIdMap;
    if (false == patternObj.pattern().empty())
    {
        auto domain = patternObj.domainVec();
        unsigned initial_key = 0;
        if (domain.size() > 0 && domain.front().dim() > 0)
        {
            initial_key = domain.front()[0];
        }
        for (auto instance : patternObj.valuesVec())
        {
            if (instance.size() > 0) // empty value not supported.
            {
                deviceIdMap[initial_key++] = instance;
            }
        }
    }
    return deviceIdMap;
}

DeviceIdMap expandDeviceRange(const std::string& deviceRegx)
{
    device_id::DeviceIdPattern devicePattern(deviceRegx);
    return expandDeviceRange(devicePattern);
}

device_id::PatternIndex
    determineDeviceIndex(const device_id::DeviceIdPattern& objPathPattern,
                         const std::string& objPath)
{
    auto indexes = objPathPattern.match(objPath);
    if (indexes.size() > 0)
    {
        return indexes[0];
    }
    return device_id::PatternIndex();
}

std::string
    determineDeviceName(const device_id::DeviceIdPattern& objPathPattern,
                        const std::string& objPath,
                        const device_id::DeviceIdPattern& deviceTypePattern)
{
    std::string deviceName{""};
    auto index = determineDeviceIndex(objPathPattern, objPath);
    if (index.dim() > 0)
    {
        // the same index from devicePattern is used in device type
        deviceName = determineDeviceName(deviceTypePattern, index);
    }
    return deviceName;
}

std::string determineDeviceName(const std::string& objPattern,
                                const std::string& objPath,
                                const std::string& devType)
{
    std::string deviceName{""};
    if (false == objPath.empty() && false == objPattern.empty())
    {
        device_id::DeviceIdPattern objectPathPattern(objPattern);
        device_id::DeviceIdPattern deviceTypePattern(devType);
        deviceName =
            determineDeviceName(objectPathPattern, objPath, deviceTypePattern);
    }
    logs_dbg("objPattern:'%s' objPath:'%s' devType:'%s' Devname:'%s'\n.",
             objPattern.c_str(), objPath.c_str(), devType.c_str(),
             deviceName.c_str());
    return deviceName;
}

std::string getFirstDeviceTypePattern(const std::string& devicesPatterns)
{
    std::vector<std::string> devices;
    boost::split(devices, devicesPatterns, boost::is_any_of("/"));
    return devices.front();
}

std::string
    determineDeviceName(const device_id::DeviceIdPattern& deviceTypePattern,
                        const device_id::PatternIndex& index)
{
    std::string name{""};
    if (false == deviceTypePattern.pattern().empty())
    {
        logs_dbg("deviceTypePattern.dim()=%u index.dim()=%u\n",
                 deviceTypePattern.dim(), index.dim());
        if (deviceTypePattern.dim() == index.dim())
        {
            name = getFirstDeviceTypePattern(deviceTypePattern.eval(index));
        }
    }
    return name;
}

} // namespace util
