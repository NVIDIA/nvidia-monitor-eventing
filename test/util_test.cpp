/**
 * Copyright (c) 2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#include "nlohmann/json.hpp"
#include "util.hpp"

#include <gtest/gtest.h>

using namespace util;

TEST(Util, GetDeviceId)
{
    EXPECT_EQ(getDeviceId("GPU5"), 5);
    EXPECT_EQ(getDeviceId("GPU6-ERoT"), 6);
    EXPECT_EQ(getDeviceId("GPU0"), 0);
    EXPECT_EQ(getDeviceId("PCIeSwitch"), 0); // using defaults
    EXPECT_EQ(getDeviceId(""), 0);           // using defaults
    EXPECT_EQ(getDeviceId("Retimer3"), 3);
    EXPECT_EQ(getDeviceId("NVSwitch1-ERoT"), 1);
    EXPECT_EQ(getDeviceId("GPU6-ERoT", "GPU[0-7]-ERoT"), 6);
    EXPECT_EQ(getDeviceId("GPU9", "GPU[0-7]"), -1);
}

TEST(Util, RemoveRange)
{
    EXPECT_EQ(removeRange("LS101"), std::string{"LS10"});
    EXPECT_EQ(removeRange("GPU5"), std::string{"GPU"});
    EXPECT_EQ(removeRange("GPU[0-3]"), std::string{"GPU"});
    EXPECT_EQ(removeRange("GPU6-ERoT"), std::string{"GPU-ERoT"});
    EXPECT_EQ(removeRange("GPU[1-6]-ERoT"), std::string{"GPU-ERoT"});
    EXPECT_EQ(removeRange("PCIeSwitch"), std::string{"PCIeSwitch"});
}

TEST(UtilExpandDeviceRange, OnlyRange)
{
    auto devMap = expandDeviceRange("[0-1]");
    EXPECT_EQ(devMap.size(), 2);
    EXPECT_EQ(devMap.count(0), 1);
    EXPECT_EQ(devMap.count(1), 1);
    EXPECT_EQ(devMap.at(0), "0");
    EXPECT_EQ(devMap.at(1), "1");
}

TEST(UtilExpandDeviceRange, RangeAtEnd)
{
    auto devMap = expandDeviceRange("begin[0-1]");
    EXPECT_EQ(devMap.size(), 2);
    EXPECT_EQ(devMap.count(0), 1);
    EXPECT_EQ(devMap.count(1), 1);
    EXPECT_EQ(devMap.at(0), "begin0");
    EXPECT_EQ(devMap.at(1), "begin1");
}

TEST(UtilExpandDeviceRange, DeviceType)
{
   auto list =  expandDeviceRange("GPU_SXM_[1-8]");
   EXPECT_EQ(list.size(), 8);
   EXPECT_EQ(list.count(1), 1);
   EXPECT_EQ(list.count(8), 1);
   EXPECT_EQ(list.at(1), "GPU_SXM_1");
   EXPECT_EQ(list.at(8), "GPU_SXM_8");

   auto dobuleList = expandDeviceRange("NVSwitch_[0-3]/NVLink_[0-39]");
   EXPECT_EQ(dobuleList.size(), 160);
   EXPECT_EQ(dobuleList.at(0), "NVSwitch_0/NVLink_0");
   EXPECT_EQ(dobuleList.at(40), "NVSwitch_1/NVLink_0");
   EXPECT_EQ(dobuleList.at(80), "NVSwitch_2/NVLink_0");
   EXPECT_EQ(dobuleList.at(120), "NVSwitch_3/NVLink_0");
   EXPECT_EQ(dobuleList.at(159), "NVSwitch_3/NVLink_39");
}

TEST(UtilExpandDeviceRange, LS10)
{
    auto devMap = expandDeviceRange("LS10[0-3]");
    EXPECT_EQ(devMap.size(), 4);
    EXPECT_EQ(devMap.count(0), 1);
    EXPECT_EQ(devMap.count(1), 1);
    EXPECT_EQ(devMap.count(2), 1);
    EXPECT_EQ(devMap.at(0), "LS100");
    EXPECT_EQ(devMap.at(1), "LS101");
    EXPECT_EQ(devMap.at(2), "LS102");
    EXPECT_EQ(devMap.at(3), "LS103");
}

TEST(UtilExpandDeviceRange, RangeAtBeginning)
{
    auto devMap = expandDeviceRange("[0-1]end");
    EXPECT_EQ(devMap.size(), 2);
    EXPECT_EQ(devMap.count(0), 1);
    EXPECT_EQ(devMap.count(1), 1);
    EXPECT_EQ(devMap.at(0), "0end");
    EXPECT_EQ(devMap.at(1), "1end");
}

TEST(UtilExpandDeviceRange, DoubleRange)
{
    auto devMap = expandDeviceRange("GPU_SXM_[1-8]_DRAM_[1-8]");
    EXPECT_EQ(devMap.size(), 64);

    EXPECT_EQ(devMap.count(1), 1);
    EXPECT_EQ(devMap.count(32), 1);
    EXPECT_EQ(devMap.count(64), 1);

    EXPECT_EQ(devMap.at(1),  "GPU_SXM_1_DRAM_1");
    EXPECT_EQ(devMap.at(32), "GPU_SXM_4_DRAM_8");
    EXPECT_EQ(devMap.at(64), "GPU_SXM_8_DRAM_8");


    devMap = expandDeviceRange("GPU_SXM_[0-3]_DRAM_[0-3]");
    EXPECT_EQ(devMap.size(), 16);

    EXPECT_EQ(devMap.count(0), 1);
    EXPECT_EQ(devMap.count(7), 1);
    EXPECT_EQ(devMap.count(15), 1);

    EXPECT_EQ(devMap.at(0), "GPU_SXM_0_DRAM_0");
    EXPECT_EQ(devMap.at(7), "GPU_SXM_1_DRAM_3");
    EXPECT_EQ(devMap.at(15), "GPU_SXM_3_DRAM_3");

}

TEST(UtilRemoveRangeFollowed, OneOccurrence)
{
   auto str = revertRangeRepeated("/xyz/first_[0-3]/second_()/third_()");
   EXPECT_EQ(str, "/xyz/first_[0-3]/second_[0-3]/third_[0-3]");

#if 0 // TODO there is another TODO on revertRangeFollowed() function
   str = revertRangeFollowed("1_[0-3]/2_()/3_()/[0-1]/_()");
   EXPECT_EQ(str, "1_[0-3]/2_[0-3]/3_[0-3]/[0-1]/_[0-1]");
#endif
}

TEST(UtilExpandDeviceRange, DoubleRangeSingleExpansion)
{
    auto devMap = expandDeviceRange("/xyz/HGX_NVSwitch_[0-3]/NVSwitch_[0|0-3]");

    EXPECT_EQ(devMap.size(), 4);
    EXPECT_EQ(devMap.count(0), 1);
    EXPECT_EQ(devMap.count(1), 1);
    EXPECT_EQ(devMap.count(2), 1);
    EXPECT_EQ(devMap.count(3), 1);
    EXPECT_EQ(devMap.at(0), "/xyz/HGX_NVSwitch_0/NVSwitch_0");
    EXPECT_EQ(devMap.at(1), "/xyz/HGX_NVSwitch_1/NVSwitch_1");
    EXPECT_EQ(devMap.at(2), "/xyz/HGX_NVSwitch_2/NVSwitch_2");
    EXPECT_EQ(devMap.at(3), "/xyz/HGX_NVSwitch_3/NVSwitch_3");

    devMap = expandDeviceRange("/xyz/first_[0-3]/second_[0|0-3]/third_[0|0-3]");
    EXPECT_EQ(devMap.size(), 4);
    EXPECT_EQ(devMap.count(0), 1);
    EXPECT_EQ(devMap.count(1), 1);
    EXPECT_EQ(devMap.count(2), 1);
    EXPECT_EQ(devMap.count(3), 1);
    EXPECT_EQ(devMap.at(0), "/xyz/first_0/second_0/third_0");
    EXPECT_EQ(devMap.at(1), "/xyz/first_1/second_1/third_1");
    EXPECT_EQ(devMap.at(2), "/xyz/first_2/second_2/third_2");
    EXPECT_EQ(devMap.at(3), "/xyz/first_3/second_3/third_3");

    devMap = expandDeviceRange("/xyz/first_[0-3]/second_[0|0-3]/third_[1|0-3]");
    EXPECT_EQ(devMap.size(), 16);
    EXPECT_EQ(devMap.count(0), 1);
    EXPECT_EQ(devMap.count(1), 1);
    EXPECT_EQ(devMap.count(2), 1);
    EXPECT_EQ(devMap.count(3), 1);
    EXPECT_EQ(devMap.at(0), "/xyz/first_0/second_0/third_0");
    EXPECT_EQ(devMap.at(1), "/xyz/first_0/second_0/third_1");
    EXPECT_EQ(devMap.at(2), "/xyz/first_0/second_0/third_2");
    EXPECT_EQ(devMap.at(15), "/xyz/first_3/second_3/third_3");
}

TEST(UtilExpandDeviceRange, RangeMiddle)
{
    auto devMap = expandDeviceRange("begin[0-1]end");
    EXPECT_EQ(devMap.size(), 2);
    EXPECT_EQ(devMap.count(0), 1);
    EXPECT_EQ(devMap.count(1), 1);
    EXPECT_EQ(devMap.at(0), "begin0end");
    EXPECT_EQ(devMap.at(1), "begin1end");
}

TEST(UtilExpandDeviceRange, NoRange)
{
    auto devMap = expandDeviceRange("test");
    EXPECT_EQ(devMap.size(), 1);
    EXPECT_EQ(devMap.count(0), 1);
    EXPECT_EQ(devMap.at(0), "test");
}

TEST(UtilExpandDeviceRange, Empty)
{
    auto devMap = expandDeviceRange("");
    EXPECT_EQ(devMap.size(), 0);
}

TEST(UtilExpandDeviceRange, TwoDigits)
{
    auto devMap = expandDeviceRange("[1-20]");
    EXPECT_EQ(devMap.size(), 20);
}

TEST(Util, GetRangeInformation)
{
    auto info = getRangeInformation("");
    EXPECT_EQ(std::get<0>(info), 0);

    info = getRangeInformation("none");
    EXPECT_EQ(std::get<0>(info), 0);

    info = getRangeInformation("[0-2]");
    EXPECT_EQ(std::get<0>(info), 5); // size
    EXPECT_EQ(std::get<1>(info), 0); // position
    EXPECT_EQ(std::get<2>(info), "[0-2]");

    info = getRangeInformation("0123 GPU[0-3]");
    EXPECT_EQ(std::get<0>(info), 8); // size
    EXPECT_EQ(std::get<1>(info), 5); // position
    EXPECT_EQ(std::get<2>(info), "GPU[0-3]");

    info = getRangeInformation("0 GPU[0-7]-ERoT end");
    EXPECT_EQ(std::get<0>(info), 13); // size
    EXPECT_EQ(std::get<1>(info), 2);  // position
    EXPECT_EQ(std::get<2>(info), "GPU[0-7]-ERoT");

    info = getRangeInformation("01GPU[0-3] end");
    EXPECT_EQ(std::get<0>(info), 10); // size
    EXPECT_EQ(std::get<1>(info), 0);  // position
    EXPECT_EQ(std::get<2>(info), "01GPU[0-3]");
}

TEST(IntroduceDeviceInObjectpath, TrainingError)
{
    std::string obj{"nvlink-training-error-wrapper TRAINING GPU_SXM_[0|1-8] NVLink_[1|0-17]"};
    device_id::DeviceIdPattern  objPattern(obj);
    EXPECT_EQ(objPattern.dim(), 2);

    device_id::DeviceIdPattern devType("GPU_SXM_[0|1-8]/NVLink_[1|0-17]");
    EXPECT_EQ(devType.dim(), 2);

    device_id::PatternIndex index(3,15);
    auto result = util::introduceDeviceInObjectpath(obj, index);
    EXPECT_EQ(result, "nvlink-training-error-wrapper TRAINING GPU_SXM_3 NVLink_15");
}

TEST(IntroduceDeviceInObjectpath, PatternIndex)
{
    std::string obj{"FPGA_SXM[0|1-8:0-7]_EROT_RECOV_L GPU_SXM_[0|1-8]"};
    device_id::DeviceIdPattern  objPattern(obj);
    EXPECT_EQ(objPattern.dim(), 1);

    std::string pcsw{"PCIeSwitch_0"};
    device_id::DeviceIdPattern noRange(pcsw);
    EXPECT_EQ(noRange.eval(device_id::PatternIndex()), pcsw);
    EXPECT_EQ(noRange.eval(device_id::PatternIndex(0)), pcsw);
}

TEST(IntroduceDeviceInObjectpath, DoubleRangeDoubleRangeViewWithDeviceData)
{
    std::string obj{"FPGA_SXM[0|1-8:0-7]_EROT_RECOV_L GPU_SXM_[0|1-8]"};
    device_id::DeviceIdPattern  objPattern(obj);

    device_id::PatternIndex index(6);
    auto ret = introduceDeviceInObjectpath(obj, index);
    EXPECT_EQ(ret, "FPGA_SXM5_EROT_RECOV_L GPU_SXM_6");
}

TEST(IntroduceDeviceInObjectpath, SingleRangeWithDeviceData)
{
    device_id::PatternIndex index(3);
    std::string obj= "/xyz/HGX_GPU_SXM_[1-8]/PCIeDevices";
    auto result = util::introduceDeviceInObjectpath(obj, index);
    EXPECT_EQ(result, "/xyz/HGX_GPU_SXM_3/PCIeDevices");
}

TEST(IntroduceDeviceInObjectpath, DoubleDeviceWithDeviceData)
{
    std::string obj= "/processors/GPU_SXM_[1-8]/Ports/NVLink_[0-17]";
    device_id::PatternIndex index(3,15);
    auto result = util::introduceDeviceInObjectpath(obj, index);
    EXPECT_EQ(result, "/processors/GPU_SXM_3/Ports/NVLink_15");
}

TEST(IntroduceDeviceInObjectpath, NoRangeWithDeviceIdData)
{
    std::string obj= "/processors/GPU_SXM_1/Ports/NVLink_7";
    device_id::PatternIndex index(3,15);
    auto result = util::introduceDeviceInObjectpath(obj, index);
    EXPECT_EQ(result, obj);
}

TEST(ExistsRange,  EmptyString)
{
    EXPECT_NE(existsRange(""), true);
}

TEST(ExistsRange,  NoRange)
{
    EXPECT_NE(existsRange("no-range"), true);
}

TEST(ExistsRange,  Range)
{
    auto str = "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_[1-8]";
    EXPECT_EQ(existsRange(str), true);
}

TEST(DetermineDeviceName, GpioStatusHandler)
{
    auto objPath = "/xyz/openbmc_project/GpioStatusHandler";
    auto devType = "PCIeSwitch_0";
    auto name = determineDeviceName(objPath, devType, devType);
    EXPECT_EQ(name.empty(), true);
}

TEST(DetermineDeviceName, MiddleName)
{
    auto pattern = "/xyz/openbmc_project/HGX_GPU_SXM_[1-8]/PCIeDevices";
    auto objPath = "/xyz/openbmc_project/HGX_GPU_SXM_5/PCIeDevices";
    auto devType = "GPU_SXM_[1-8]";
    auto name = determineDeviceName(pattern, objPath, devType);
    EXPECT_EQ(name, "GPU_SXM_5");
}

TEST(DetermineDeviceName, NoPattern)
{
    auto objPattern = "/xyz/openbmc_project/GpioStatusHandler";
    auto obj = "/xyz/openbmc_project/GpioStatusHandler";
    auto devName = determineDeviceName(objPattern, obj, "GPU_SXM_[1-8]");
    EXPECT_EQ(devName.empty(), true);
}

TEST(DetermineDeviceName, Pattern)
{
    auto objPattern = "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_[1-8]";
    auto obj = "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_8";
    auto devName = determineDeviceName(objPattern, obj, "GPU_SXM_[1-8]");
    EXPECT_EQ(devName, "GPU_SXM_8");
}


TEST(DetermineDeviceName, DeviceIdPattern)
{
    auto objPattern = "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_[1-8]";
    auto obj = "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_3";
    device_id::DeviceIdPattern deviceObjPattern(objPattern);
    device_id::DeviceIdPattern deviceTypePattern("GPU_SXM_[1-8]");
    auto devName = determineDeviceName(deviceObjPattern, obj, deviceTypePattern);
    EXPECT_EQ(devName, "GPU_SXM_3");
}

TEST(DetermineDeviceName, DeviceIdDoublePattern)
{
    auto objPattern = "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_[0-3]/Ports/NVLink_[0-39]";
    auto obj = "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_1/Ports/NVLink_21";
    device_id::DeviceIdPattern deviceObjPattern(objPattern);
    device_id::DeviceIdPattern deviceTypePattern("NVSwitch_[0-3]/NVLink_[0-39]");
    auto devName = determineDeviceName(deviceObjPattern, obj, deviceTypePattern);
    EXPECT_EQ(devName, "NVSwitch_1");
}

TEST(UtilMatchRegexString, NoPatternMatches)
{
   bool match = matchRegexString("YES", "YES");
   EXPECT_EQ(match, true);
}

TEST(UtilMatchRegexString, NoPatternDoesNotMatch)
{
   bool match = matchRegexString("YES", "No");
   EXPECT_NE(match, true);
}

TEST(UtilMatchRegexString, DoublePatternMatches)
{
    auto objPattern = "/xyz/openbmc_project/inventory/system/fabrics/"
              "HGX_NVLinkFabric_0/Switches/NVSwitch_[0-3]/Ports/NVLink_[0-39]";
    auto obj = "/xyz/openbmc_project/inventory/system/fabrics/"
               "HGX_NVLinkFabric_0/Switches/NVSwitch_1/Ports/NVLink_21";

    bool match = matchRegexString(objPattern, obj);
    EXPECT_EQ(match, true);
}

TEST(UtilMatchRegexString, DoublePatternDoesNotMatch)
{
    auto objPattern = "/xyz/openbmc_project/inventory/system/fabrics/"
              "HGX_NVLinkFabric_0/Switches/NVSwitch_[0-3]/Ports/NVLink_[0-39]";
    auto obj = "/xyz/openbmc_project/inventory/system/fabrics/"
               "HGX_NVLinkFabric_0/Switches/NVSwitch_1/";

    bool match = matchRegexString(objPattern, obj);
    EXPECT_NE(match, true);
}

using namespace util::file_util;

TEST(writeJson2File, WriteFileOK)
{
    auto file = "/tmp/GPU_0";
    nlohmann::json j = {
        "Status", {
            {"Health", "OK"},
            {"HealthRollup", "OK"},
            {"Conditions", {""}},
        },
    };

    int rc = writeJson2File(file, j);
    EXPECT_EQ(rc, 0);
}
