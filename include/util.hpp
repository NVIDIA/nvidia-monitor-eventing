/*
 Copyright (c) 2021, NVIDIA CORPORATION.  All rights reserved.

 NVIDIA CORPORATION and its licensors retain all intellectual property
 and proprietary rights in and to this software, related documentation
 and any modifications thereto.  Any use, reproduction, disclosure or
 distribution of this software and related documentation without an express
 license agreement from NVIDIA CORPORATION is strictly prohibited.
*
*/

#pragma once
#include <iostream>
#include <map>
#include <regex>
#include <string>
#include <tuple>
#include <vector>

namespace util
{

constexpr int InvalidDeviceId = -1;

constexpr auto RangeRepeaterIndicator = "()";

using DeviceIdMap = std::map<int, std::string>;

using StringPosition = size_t;
using SizeString = size_t;
using RangeInformation = std::tuple<SizeString, StringPosition, std::string>;

/**
 * @brief it parses a string, and if it has a range returns its information
 * @param str
 *
 * @example
 *    "0123 GPU[0-3]" SizeString=8, StringPosition=5 string=GPU[0-3]
 *    "01GPU[0-3] end" SizeString=10, StringPosition=0 string=GPU[0-3]
 *    "0 GPU[0-7]-ERoT end" SizeString=13 stringPosition=2 string=GPU[0-7]-ERoT
 * @return
 */
RangeInformation getRangeInformation(const std::string& str);

void printThreadId(const char* funcName);

/**
 * @brief  performs std::regex_search(str, rgx)
 *
 * @param str  common string
 *
 * @param rgx  regular expression
 *
 * @return the part of the str which matches or an empty string (no matches)
 */
std::string matchedRegx(const std::string& str, const std::string& rgx);

bool existsRegx(const std::string& str, const std::string& rgx);

bool existsRange(const std::string& str);

std::tuple<std::vector<int>, std::string>
    getMinMaxRange(const std::string& rgx);

std::string removeRange(const std::string& str);

/**
 * @brief checks if name is not empty and it is not regular expression
 * @param name   an object path such as /xyz/path/deviceName
 * @return  the last part of a valid object path
 */
std::string getDeviceName(const std::string& name);

/**
 *  @brief  returns the device Id of a device
 *
 *  @param   [optional] range
 *
 *  @example getDeviceId("GPU5") should return 5
 *           getDeviceId("GPU6-ERoT") should return 6
 *           getDeviceId("GPU0") should return 0
 *           getDeviceId("PCIeSwitch") should return 0
 *           getDeviceId("GPU6-ERoT", "GPU[0-7]-ERoT") should return 6
 *           getDeviceId("GPU9", "GPU[0-7]") should return -1
 *
 *  @return the ID when found otherwise (-1 if range is specified or 0 if not)
 */
int getDeviceId(const std::string& deviceName,
                const std::string& range = std::string{""});

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

/** Allows multiple range replacement, uses deviceType to adjust the deviceId
 *
 *  @example:
 *  replaceRangeByMatchedValue("FPGA_SXM[0-7]_EROT_RECOV_L GPU_SXM_[1-8]",
 *                         "GPU_SXM_4", "GPU_SXM_[1-8]");
 *
 *  returns:  "FPGA_SXM3_EROT_RECOV_L GPU_SXM_4" // deviceId 4 is adjust to 3
 *
 *  @return  the string replaced or the 'pattern' if there is no range
 *
 */
std::string
    replaceRangeByMatchedValue(const std::string& regxValue,
                               const std::string& matchedValue,
                               const std::string& deviceType = std::string{""});

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
 * @brief Print the vector @c vec to the output stream @c os (e.g. std::cout,
 * std::cerr, std::stringstream) with every line prefixed with @c indent.
 *
 * The format is like:
 *
 * ,----
 * |   [
 * |   0:
 * |     pattern:  {GPUId} SRAM
 * |     parameters:
 * |       [
 * |       0:
 * |         {"type":"CurrentDeviceName"}
 * |       ]
 * |   1:
 * |     pattern:  Uncorrectable ECC Error
 * |     parameters:
 * |       [
 * |       ]
 * |   ]
 * `----
 *
 * with @c indent being two spaces, and the object representation defined by @c
 * MessageArg::print, which itself calls this function recursively for the @c
 * parameters vector-type field.
 *
 * For the use with logging framework use the following construct:
 *
 * @code
 *   std::stringstream ss;
 *   print(vec, ss, "");
 *   log_dbg("%s", ss.str().c_str());
 * @endcode
 */
template <class CharT, typename T>
void print(const std::vector<T>& vec, std::basic_ostream<CharT>& os,
           std::string indent)
{
    os << indent << "[";
    if (vec.size() > 0)
    {
        os << std::endl;
        for (auto i = 0u; i < vec.size(); ++i)
        {
            os << indent << i << ":" << std::endl;
            vec.at(i).print(os, indent + "\t");
        }
    }
    else // ! vec.size() > 0
    {
        os << std::endl;
    }
    os << indent << "]" << std::endl;
}

/**
 * @brief Replace any occurrence of "()" in a string by a previous range
 * specification, It reverts what @sa expandDeviceRange() made
 *
 * @example
 *   Having a range specification such as: "xyz_[-1-5]/another_()"
 *      returns "xyz_[-1-5]/another_[1-5]"
 */
std::string revertRangeRepeated(const std::string& str,
                                size_t pos = std::string::npos);

/**
 * @brief  Having a string with one or more range specifications returns a
 *         string suitable for std::regex_search()
 *
 * @example
 *   revertRangeRepeated("test[1-5]") returns "test[0-9]+"
 */
std::string makeRangeForRegexSearch(const std::string& rangeStr);

/**
 * @brief Splits a device_type string definition such as "GPU_SXM_[1-8]_DRAM_0"
 *         preserving range specificatin and isolated digits, also makes ranges
 *         suitable for std::regex_search()
 * @param  deviceType the device_type string
 * @param  devTypePieces an empty array where to store to split into
 * @example
 *          splitDeviceTypeForRegxSearch("GPU", "SMX_[1-8]+", "DRAM_0") returns
 *          [ "GPU", "SMX_[1-8]+", "DRAM_0" ]
 */
void splitDeviceTypeForRegxSearch(const std::string& deviceType,
                                  std::vector<std::string>& devTypePieces);

/**
 * @brief determine the AssertedDeviceName based on device_type
 *
 * Here are some examples as Unit Test:
 * @code
 *
 *      TEST(Device, AssertedDeviceName)
 *      {
 *         auto result = determineAssertedDeviceName("HGX_GPU_SXM_2",
 *                                                   "GPU_SXM_[1-8]_DRAM_0");
 *         EXPECT_EQ(result, "GPU_SXM_2_DRAM_0");
 *
 *         result = determineAssertedDeviceName("GPU_SXM_8",
 *                                             "GPU_SXM_[1-8]");
 *         EXPECT_EQ(result, "GPU_SXM_8");
 *
 *         result = determineAssertedDeviceName("PCIeSwitch0",
 *                                             "PCIeSwitch0");
 *         EXPECT_EQ(result, "PCIeSwitch0");
 *      }
 * @endcode
 */
std::string determineAssertedDeviceName(const std::string& realDevice,
                                        const std::string& deviceType);

/**
 * @return good deviceId if the deviceName is mapped or
 *         InvalidDeviceId in case the deviceName is not a mapped one
 *
 * @sa getDeviceId()
 */
int getMappedDeviceId(const std::string& deviceName);

/** @brief Compares  two strings using regular expression
 * @param regstr    string that may contain regular expression
 * @param str       normal string
 *
 * @note
 *      It considers strings used in event_info.json such as
 *             "HGX_GPU_SXM_[1-8]/PCIeDevices/GPU_SXM_()"
 *
 * @return true if both field strings match, otherwise false
 */
bool matchRegexString(const std::string& regstr, const std::string& str);

/**
 * @brief Creates a regular expression to match range in the 'pattern' parameter
 *
 *
 * @param pattern any string such as "GPU_1"
 *
 * @example
 *     createRegexDigitsRange("GPU_1")
 *        returns std::regex("(GPU_\\[[0-9]*-*[0-9]*\\])")
 *
 * @return std::regex
 *
 * @sa introduceDeviceInObjectpath()
 */
std::regex createRegexDigitsRange(const std::string& pattern);

/**
 * @brief Replaces occurrencies of 'device' with range in  objPath by device
 *
 * @param objPath having range specification such as:
 *
 *     "/xyz/inventory/chassis/HGX_GPU_SXM_[1-8]"
 *  or
 *     "/xyz/inventory/chassis/HGX_GPU_SXM_[1-8]/PCIeDevices/GPU_SXM_()"
 *
 * @param device device name such as "GPU_SMX_1"
 *
 * @return an Object path without range if 'device' matches with 'objPath'
 *         otherwise returns 'objPath'
 */
std::string introduceDeviceInObjectpath(const std::string& objPath,
                                        const std::string& device);

} // namespace util
