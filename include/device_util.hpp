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

#include "device_id.hpp"

#include <iostream>
#include <map>
#include <regex>
#include <string>
#include <tuple>
#include <vector>

namespace util
{

using DeviceIdMap = std::map<int, std::string>;

bool existsRange(const device_id::DeviceIdPattern& patternObj);
bool existsRange(const std::string& str);

/**
 * @brief   Expands range in a string and returns a list of expanded strings
 *
 * Calls example:
 * @example
 *   ("[0-5]");          '0'  '1'  '2'  '3'  '4'  '5'
 *   ("name[1-4]");      'name1'  'name2'  'name3'  'name4'
 *   (begin[0-2]end");   'begin0end'  'begin1end'  'begin2end'
 *   ("unique");         'unique'
 *   ("[2-3]end");       '2end'  '3end'
 *
 * @return DeviceIdMap which contains both deviceId and deviceName
 */
DeviceIdMap expandDeviceRange(const std::string& deviceRegx);
DeviceIdMap expandDeviceRange(const device_id::DeviceIdPattern& patternObj);

device_id::PatternIndex
determineDeviceIndex(const device_id::DeviceIdPattern& objPathPattern,
                     const std::string& objPath);

/**
 * @brief determine device name from DBus object path.
 *
 * @param objPath
 * @param devType
 * @return std::string
 */
std::string determineDeviceName(const std::string& objPath,
                                const std::string& devType);


/**
 * @brief Returns a device_id::PatternIndex that will
 *        contain the indexes from the patterns in @c objPathPattern which
 *        matches @c objPath
 *
 * @param objPathPattern an object-path that may have patterns(usually Accessor)
 * @param objPath  a real object-path from Dbus (i,e., got from DBus message)
 *
 * Example:
 * @code
   TEST(DetermineDeviceName, Pattern)
   {
      auto obP = "/xyz/openbmc_project/processors/GPU_SXM_[1-8]/more";
      auto ob = "/xyz/openbmc_project/processors/GPU_SXM_4/more";
      auto devName = determineDeviceName(obP, ob, "GPU_SXM_[1-8]");
      EXPECT_EQ(devName, "GPU_SXM_4");
   }

   TEST(DetermineDeviceName, NoPattern)
   {
      auto objPattern = "/xyz/openbmc_project/GpioStatusHandler";
      auto obj = "/xyz/openbmc_project/GpioStatusHandler";
      auto devName = determineDeviceName(objPattern, obj, "GPU_SXM_[1-8]");
      EXPECT_EQ(devName.empty(), true);
   }
 * @endcode
 */
std::string determineDeviceName(const std::string& objPathPattern,
                                const std::string& objPath,
                                const std::string& devType);

/**
 * @brief This is an overloaded method that is used by the previous version
 * @param objPathPattern
 * @param objPath
 * @param deviceTypePattern
 * Example:
 * @code
   TEST(DetermineDeviceName, DeviceIdPattern)
   {
      auto objPattern = "/xyz/openbmc_project/processors/GPU_SXM_[1-8]/more";
      auto obj = "/xyz/openbmc_project/processors/GPU_SXM_4/more";
      device_id::DeviceIdPattern deviceObjPattern(objPattern);
      device_id::DeviceIdPattern deviceTypePattern("GPU_SXM_[1-8]");
      auto devName = determineDeviceName(deviceObjPattern, obj, deviceTypePattern);
      EXPECT_EQ(devName, "GPU_SXM_4");
   }
   @endcode
 *
 */
std::string determineDeviceName(const device_id::DeviceIdPattern& objPathPattern,
                    const std::string& objPath,
                    const device_id::DeviceIdPattern& deviceTypePattern);

/**
 * @brief Returns the first device from a device_type pattern that can be
 *        single "GPU_[1-8]" or multi devices "GPU_[1-8"]/OTHER_[0-3]"
 *
 * @param deviceTypePattern the device_type pattern
 * @param index
 * @return the first device name according to the index
 */
std::string determineDeviceName(
        const device_id::DeviceIdPattern& deviceTypePattern,
        const device_id::PatternIndex& index);

/**
 * @brief Returns the first element from a pattern having separator such as '/'
 * @li  "GPU_[1-8]/OTHER_[0-3]", returns "GPU_[1-8]"
 * @li  "GPU_3/OTHER_2", returns ""GPU_3"
 * @param devicesPatterns a pattern separated by a separator defined indevice_id
 */
std::string getFirstDeviceTypePattern(const std::string& devicesPatterns);

} // namespace util
