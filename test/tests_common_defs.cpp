/**
 * Copyright (c) 2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#include "tests_common_defs.hpp"

using namespace nlohmann;

std::vector<std::string> DummyObjectMapper::getSubTreePathsImpl(
    [[maybe_unused]] sdbusplus::bus::bus& bus,
    [[maybe_unused]] const std::string& subtree, [[maybe_unused]] int depth,
    [[maybe_unused]] const std::vector<std::string>& interfaces)
{
    assert(depth == 0);
    assert(subtree == "/");
    std::vector<std::string> result;
    result.push_back("/");
    result.push_back("/xyz");
    result.push_back("/xyz/openbmc_project");
    result.push_back("/xyz/openbmc_project/GpuMgr");
    result.push_back("/xyz/openbmc_project/inventory");
    result.push_back("/xyz/openbmc_project/inventory/platformmetrics");
    result.push_back("/xyz/openbmc_project/inventory/system");
    result.push_back("/xyz/openbmc_project/inventory/system/chassis");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_Chassis_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_Chassis_0/Assembly0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_Chassis_0/Assembly1");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_Chassis_0/Assembly2");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_Chassis_0/PCIeSlots0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_Chassis_0/PCIeSlots1");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_Chassis_0/PCIeSlots2");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_Chassis_0/PCIeSlots3");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_Chassis_0/PCIeSlots4");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_Chassis_0/PCIeSlots5");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_Chassis_0/PCIeSlots6");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_Chassis_0/PCIeSlots7");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_Chassis_0/PCIeSlots8");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_Chassis_0/PCIeSlots9");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_FPGA_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_1");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_1/Assembly0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_1/Assembly1");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_1/PCIeDevices");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_1/PCIeDevices/GPU_SXM_1");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_2");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_2/Assembly0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_2/Assembly1");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_2/PCIeDevices");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_2/PCIeDevices/GPU_SXM_2");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_3");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_3/Assembly0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_3/Assembly1");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_3/PCIeDevices");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_3/PCIeDevices/GPU_SXM_3");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_4");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_4/Assembly0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_4/Assembly1");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_4/PCIeDevices");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_4/PCIeDevices/GPU_SXM_4");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_5");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_5/Assembly0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_5/Assembly1");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_5/PCIeDevices");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_5/PCIeDevices/GPU_SXM_5");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_6");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_6/Assembly0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_6/Assembly1");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_6/PCIeDevices");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_6/PCIeDevices/GPU_SXM_6");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_7");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_7/Assembly0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_7/Assembly1");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_7/PCIeDevices");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_7/PCIeDevices/GPU_SXM_7");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_8");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_8/Assembly0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_8/Assembly1");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_8/PCIeDevices");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_8/PCIeDevices/GPU_SXM_8");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_NVSwitch_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_NVSwitch_0/PCIeDevices");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_NVSwitch_0/PCIeDevices/NVSwitch_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_NVSwitch_1");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_NVSwitch_1/PCIeDevices");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_NVSwitch_1/PCIeDevices/NVSwitch_1");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_NVSwitch_2");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_NVSwitch_2/PCIeDevices");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_NVSwitch_2/PCIeDevices/NVSwitch_2");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_NVSwitch_3");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_NVSwitch_3/PCIeDevices");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_NVSwitch_3/PCIeDevices/NVSwitch_3");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_PCIeRetimer_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_PCIeRetimer_1");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_PCIeRetimer_2");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_PCIeRetimer_3");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_PCIeRetimer_4");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_PCIeRetimer_5");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_PCIeRetimer_6");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_PCIeRetimer_7");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_PCIeSwitch_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_PCIeSwitch_0/PCIeDevices");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/chassis/HGX_PCIeSwitch_0/PCIeDevices/PCIeSwitch_0");
    result.push_back("/xyz/openbmc_project/inventory/system/fabrics");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Endpoints");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Endpoints/GPU_SXM_1");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Endpoints/GPU_SXM_2");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Endpoints/GPU_SXM_3");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Endpoints/GPU_SXM_4");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Endpoints/GPU_SXM_5");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Endpoints/GPU_SXM_6");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Endpoints/GPU_SXM_7");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Endpoints/GPU_SXM_8");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_0/Ports");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_0/Ports/NVLink_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_0/Ports/NVLink_1");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_0/Ports/NVLink_10");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_0/Ports/NVLink_11");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_0/Ports/NVLink_12");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_0/Ports/NVLink_13");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_0/Ports/NVLink_14");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_0/Ports/NVLink_15");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_0/Ports/NVLink_16");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_0/Ports/NVLink_17");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_0/Ports/NVLink_18");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_0/Ports/NVLink_19");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_0/Ports/NVLink_2");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_0/Ports/NVLink_20");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_0/Ports/NVLink_21");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_0/Ports/NVLink_22");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_0/Ports/NVLink_23");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_0/Ports/NVLink_24");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_0/Ports/NVLink_25");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_0/Ports/NVLink_26");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_0/Ports/NVLink_27");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_0/Ports/NVLink_28");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_0/Ports/NVLink_29");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_0/Ports/NVLink_3");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_0/Ports/NVLink_30");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_0/Ports/NVLink_31");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_0/Ports/NVLink_4");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_0/Ports/NVLink_5");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_0/Ports/NVLink_6");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_0/Ports/NVLink_7");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_0/Ports/NVLink_8");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_0/Ports/NVLink_9");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_1");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_1/Ports");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_1/Ports/NVLink_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_1/Ports/NVLink_1");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_1/Ports/NVLink_10");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_1/Ports/NVLink_11");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_1/Ports/NVLink_12");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_1/Ports/NVLink_13");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_1/Ports/NVLink_14");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_1/Ports/NVLink_15");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_1/Ports/NVLink_16");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_1/Ports/NVLink_17");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_1/Ports/NVLink_18");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_1/Ports/NVLink_19");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_1/Ports/NVLink_2");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_1/Ports/NVLink_20");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_1/Ports/NVLink_21");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_1/Ports/NVLink_22");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_1/Ports/NVLink_23");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_1/Ports/NVLink_24");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_1/Ports/NVLink_25");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_1/Ports/NVLink_26");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_1/Ports/NVLink_27");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_1/Ports/NVLink_28");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_1/Ports/NVLink_29");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_1/Ports/NVLink_3");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_1/Ports/NVLink_30");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_1/Ports/NVLink_31");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_1/Ports/NVLink_32");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_1/Ports/NVLink_33");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_1/Ports/NVLink_34");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_1/Ports/NVLink_35");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_1/Ports/NVLink_36");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_1/Ports/NVLink_37");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_1/Ports/NVLink_38");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_1/Ports/NVLink_39");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_1/Ports/NVLink_4");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_1/Ports/NVLink_5");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_1/Ports/NVLink_6");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_1/Ports/NVLink_7");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_1/Ports/NVLink_8");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_1/Ports/NVLink_9");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_2");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_2/Ports");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_2/Ports/NVLink_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_2/Ports/NVLink_1");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_2/Ports/NVLink_10");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_2/Ports/NVLink_11");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_2/Ports/NVLink_12");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_2/Ports/NVLink_13");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_2/Ports/NVLink_14");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_2/Ports/NVLink_15");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_2/Ports/NVLink_16");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_2/Ports/NVLink_17");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_2/Ports/NVLink_18");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_2/Ports/NVLink_19");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_2/Ports/NVLink_2");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_2/Ports/NVLink_20");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_2/Ports/NVLink_21");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_2/Ports/NVLink_22");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_2/Ports/NVLink_23");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_2/Ports/NVLink_24");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_2/Ports/NVLink_25");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_2/Ports/NVLink_26");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_2/Ports/NVLink_27");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_2/Ports/NVLink_28");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_2/Ports/NVLink_29");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_2/Ports/NVLink_3");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_2/Ports/NVLink_30");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_2/Ports/NVLink_31");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_2/Ports/NVLink_32");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_2/Ports/NVLink_33");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_2/Ports/NVLink_34");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_2/Ports/NVLink_35");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_2/Ports/NVLink_36");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_2/Ports/NVLink_37");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_2/Ports/NVLink_38");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_2/Ports/NVLink_39");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_2/Ports/NVLink_4");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_2/Ports/NVLink_5");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_2/Ports/NVLink_6");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_2/Ports/NVLink_7");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_2/Ports/NVLink_8");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_2/Ports/NVLink_9");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_3");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_3/Ports");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_3/Ports/NVLink_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_3/Ports/NVLink_1");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_3/Ports/NVLink_10");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_3/Ports/NVLink_11");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_3/Ports/NVLink_12");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_3/Ports/NVLink_13");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_3/Ports/NVLink_14");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_3/Ports/NVLink_15");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_3/Ports/NVLink_16");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_3/Ports/NVLink_17");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_3/Ports/NVLink_18");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_3/Ports/NVLink_19");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_3/Ports/NVLink_2");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_3/Ports/NVLink_20");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_3/Ports/NVLink_21");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_3/Ports/NVLink_22");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_3/Ports/NVLink_23");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_3/Ports/NVLink_24");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_3/Ports/NVLink_25");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_3/Ports/NVLink_26");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_3/Ports/NVLink_27");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_3/Ports/NVLink_28");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_3/Ports/NVLink_29");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_3/Ports/NVLink_3");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_3/Ports/NVLink_30");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_3/Ports/NVLink_31");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_3/Ports/NVLink_4");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_3/Ports/NVLink_5");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_3/Ports/NVLink_6");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_3/Ports/NVLink_7");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_3/Ports/NVLink_8");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_3/Ports/NVLink_9");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/zones");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/zones/0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_0/Endpoints");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_0/Endpoints/GPU_SXM_1");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_0/Switches");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_0/Switches/PCIeRetimer_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_0/Switches/PCIeRetimer_0/Ports");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_0/Switches/PCIeRetimer_0/Ports/DOWN_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_0/Switches/PCIeRetimer_0/Ports/UP_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_0/zones");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_0/zones/0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_1");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_1/Endpoints");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_1/Endpoints/GPU_SXM_2");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_1/Switches");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_1/Switches/PCIeRetimer_1");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_1/Switches/PCIeRetimer_1/Ports");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_1/Switches/PCIeRetimer_1/Ports/DOWN_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_1/Switches/PCIeRetimer_1/Ports/UP_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_1/zones");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_1/zones/0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_2");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_2/Endpoints");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_2/Endpoints/GPU_SXM_3");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_2/Switches");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_2/Switches/PCIeRetimer_2");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_2/Switches/PCIeRetimer_2/Ports");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_2/Switches/PCIeRetimer_2/Ports/DOWN_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_2/Switches/PCIeRetimer_2/Ports/UP_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_2/zones");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_2/zones/0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_3");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_3/Endpoints");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_3/Endpoints/GPU_SXM_4");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_3/Switches");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_3/Switches/PCIeRetimer_3");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_3/Switches/PCIeRetimer_3/Ports");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_3/Switches/PCIeRetimer_3/Ports/DOWN_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_3/Switches/PCIeRetimer_3/Ports/UP_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_3/zones");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_3/zones/0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_4");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_4/Endpoints");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_4/Endpoints/GPU_SXM_5");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_4/Switches");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_4/Switches/PCIeRetimer_4");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_4/Switches/PCIeRetimer_4/Ports");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_4/Switches/PCIeRetimer_4/Ports/DOWN_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_4/Switches/PCIeRetimer_4/Ports/UP_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_4/zones");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_4/zones/0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_5");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_5/Endpoints");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_5/Endpoints/GPU_SXM_6");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_5/Switches");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_5/Switches/PCIeRetimer_5");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_5/Switches/PCIeRetimer_5/Ports");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_5/Switches/PCIeRetimer_5/Ports/DOWN_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_5/Switches/PCIeRetimer_5/Ports/UP_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_5/zones");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_5/zones/0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_6");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_6/Endpoints");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_6/Endpoints/GPU_SXM_7");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_6/Switches");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_6/Switches/PCIeRetimer_6");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_6/Switches/PCIeRetimer_6/Ports");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_6/Switches/PCIeRetimer_6/Ports/DOWN_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_6/Switches/PCIeRetimer_6/Ports/UP_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_6/zones");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_6/zones/0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_7");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_7/Endpoints");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_7/Endpoints/GPU_SXM_8");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_7/Switches");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_7/Switches/PCIeRetimer_7");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_7/Switches/PCIeRetimer_7/Ports");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_7/Switches/PCIeRetimer_7/Ports/DOWN_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_7/Switches/PCIeRetimer_7/Ports/UP_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_7/zones");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_7/zones/0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeSwitchTopology_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeSwitchTopology_0/Endpoints");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeSwitchTopology_0/Endpoints/NVSwitch_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeSwitchTopology_0/Endpoints/NVSwitch_1");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeSwitchTopology_0/Endpoints/NVSwitch_2");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeSwitchTopology_0/Endpoints/NVSwitch_3");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeSwitchTopology_0/Switches");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeSwitchTopology_0/Switches/PCIeSwitch_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeSwitchTopology_0/Switches/PCIeSwitch_0/Ports");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeSwitchTopology_0/Switches/PCIeSwitch_0/Ports/DOWN_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeSwitchTopology_0/Switches/PCIeSwitch_0/Ports/DOWN_1");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeSwitchTopology_0/Switches/PCIeSwitch_0/Ports/DOWN_2");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeSwitchTopology_0/Switches/PCIeSwitch_0/Ports/DOWN_3");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeSwitchTopology_0/Switches/PCIeSwitch_0/Ports/UP_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeSwitchTopology_0/zones");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeSwitchTopology_0/zones/0");
    result.push_back("/xyz/openbmc_project/inventory/system/managers");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/managers/HGX_FabricManager_0");
    result.push_back("/xyz/openbmc_project/inventory/system/memory");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/memory/GPU_SXM_1_DRAM_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/memory/GPU_SXM_2_DRAM_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/memory/GPU_SXM_3_DRAM_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/memory/GPU_SXM_4_DRAM_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/memory/GPU_SXM_5_DRAM_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/memory/GPU_SXM_6_DRAM_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/memory/GPU_SXM_7_DRAM_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/memory/GPU_SXM_8_DRAM_0");
    result.push_back("/xyz/openbmc_project/inventory/system/processors");
    result.push_back("/xyz/openbmc_project/inventory/system/processors/FPGA_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/FPGA_0/Ports");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/FPGA_0/Ports/PCIeToHMC_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/FPGA_0/Ports/PCIeToHost_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_1");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_1/Ports");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_1/Ports/NVLink_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_1/Ports/NVLink_1");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_1/Ports/NVLink_10");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_1/Ports/NVLink_11");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_1/Ports/NVLink_12");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_1/Ports/NVLink_13");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_1/Ports/NVLink_14");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_1/Ports/NVLink_15");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_1/Ports/NVLink_16");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_1/Ports/NVLink_17");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_1/Ports/NVLink_2");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_1/Ports/NVLink_3");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_1/Ports/NVLink_4");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_1/Ports/NVLink_5");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_1/Ports/NVLink_6");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_1/Ports/NVLink_7");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_1/Ports/NVLink_8");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_1/Ports/NVLink_9");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_2");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_2/Ports");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_2/Ports/NVLink_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_2/Ports/NVLink_1");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_2/Ports/NVLink_10");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_2/Ports/NVLink_11");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_2/Ports/NVLink_12");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_2/Ports/NVLink_13");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_2/Ports/NVLink_14");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_2/Ports/NVLink_15");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_2/Ports/NVLink_16");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_2/Ports/NVLink_17");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_2/Ports/NVLink_2");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_2/Ports/NVLink_3");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_2/Ports/NVLink_4");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_2/Ports/NVLink_5");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_2/Ports/NVLink_6");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_2/Ports/NVLink_7");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_2/Ports/NVLink_8");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_2/Ports/NVLink_9");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_3");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_3/Ports");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_3/Ports/NVLink_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_3/Ports/NVLink_1");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_3/Ports/NVLink_10");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_3/Ports/NVLink_11");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_3/Ports/NVLink_12");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_3/Ports/NVLink_13");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_3/Ports/NVLink_14");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_3/Ports/NVLink_15");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_3/Ports/NVLink_16");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_3/Ports/NVLink_17");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_3/Ports/NVLink_2");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_3/Ports/NVLink_3");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_3/Ports/NVLink_4");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_3/Ports/NVLink_5");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_3/Ports/NVLink_6");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_3/Ports/NVLink_7");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_3/Ports/NVLink_8");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_3/Ports/NVLink_9");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_4");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_4/Ports");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_4/Ports/NVLink_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_4/Ports/NVLink_1");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_4/Ports/NVLink_10");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_4/Ports/NVLink_11");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_4/Ports/NVLink_12");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_4/Ports/NVLink_13");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_4/Ports/NVLink_14");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_4/Ports/NVLink_15");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_4/Ports/NVLink_16");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_4/Ports/NVLink_17");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_4/Ports/NVLink_2");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_4/Ports/NVLink_3");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_4/Ports/NVLink_4");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_4/Ports/NVLink_5");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_4/Ports/NVLink_6");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_4/Ports/NVLink_7");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_4/Ports/NVLink_8");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_4/Ports/NVLink_9");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_5");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_5/Ports");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_5/Ports/NVLink_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_5/Ports/NVLink_1");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_5/Ports/NVLink_10");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_5/Ports/NVLink_11");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_5/Ports/NVLink_12");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_5/Ports/NVLink_13");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_5/Ports/NVLink_14");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_5/Ports/NVLink_15");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_5/Ports/NVLink_16");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_5/Ports/NVLink_17");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_5/Ports/NVLink_2");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_5/Ports/NVLink_3");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_5/Ports/NVLink_4");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_5/Ports/NVLink_5");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_5/Ports/NVLink_6");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_5/Ports/NVLink_7");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_5/Ports/NVLink_8");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_5/Ports/NVLink_9");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_6");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_6/Ports");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_6/Ports/NVLink_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_6/Ports/NVLink_1");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_6/Ports/NVLink_10");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_6/Ports/NVLink_11");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_6/Ports/NVLink_12");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_6/Ports/NVLink_13");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_6/Ports/NVLink_14");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_6/Ports/NVLink_15");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_6/Ports/NVLink_16");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_6/Ports/NVLink_17");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_6/Ports/NVLink_2");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_6/Ports/NVLink_3");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_6/Ports/NVLink_4");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_6/Ports/NVLink_5");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_6/Ports/NVLink_6");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_6/Ports/NVLink_7");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_6/Ports/NVLink_8");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_6/Ports/NVLink_9");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_7");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_7/Ports");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_7/Ports/NVLink_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_7/Ports/NVLink_1");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_7/Ports/NVLink_10");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_7/Ports/NVLink_11");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_7/Ports/NVLink_12");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_7/Ports/NVLink_13");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_7/Ports/NVLink_14");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_7/Ports/NVLink_15");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_7/Ports/NVLink_16");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_7/Ports/NVLink_17");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_7/Ports/NVLink_2");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_7/Ports/NVLink_3");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_7/Ports/NVLink_4");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_7/Ports/NVLink_5");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_7/Ports/NVLink_6");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_7/Ports/NVLink_7");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_7/Ports/NVLink_8");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_7/Ports/NVLink_9");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_8");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_8/Ports");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_8/Ports/NVLink_0");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_8/Ports/NVLink_1");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_8/Ports/NVLink_10");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_8/Ports/NVLink_11");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_8/Ports/NVLink_12");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_8/Ports/NVLink_13");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_8/Ports/NVLink_14");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_8/Ports/NVLink_15");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_8/Ports/NVLink_16");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_8/Ports/NVLink_17");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_8/Ports/NVLink_2");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_8/Ports/NVLink_3");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_8/Ports/NVLink_4");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_8/Ports/NVLink_5");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_8/Ports/NVLink_6");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_8/Ports/NVLink_7");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_8/Ports/NVLink_8");
    result.push_back(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_8/Ports/NVLink_9");
    result.push_back("/xyz/openbmc_project/inventory/system/system");
    result.push_back("/xyz/openbmc_project/inventory/system/system/system0");
    result.push_back("/xyz/openbmc_project/inventory_software");
    result.push_back(
        "/xyz/openbmc_project/inventory_software/HGX_Driver_GPU_SXM_1");
    result.push_back(
        "/xyz/openbmc_project/inventory_software/HGX_Driver_GPU_SXM_2");
    result.push_back(
        "/xyz/openbmc_project/inventory_software/HGX_Driver_GPU_SXM_3");
    result.push_back(
        "/xyz/openbmc_project/inventory_software/HGX_Driver_GPU_SXM_4");
    result.push_back(
        "/xyz/openbmc_project/inventory_software/HGX_Driver_GPU_SXM_5");
    result.push_back(
        "/xyz/openbmc_project/inventory_software/HGX_Driver_GPU_SXM_6");
    result.push_back(
        "/xyz/openbmc_project/inventory_software/HGX_Driver_GPU_SXM_7");
    result.push_back(
        "/xyz/openbmc_project/inventory_software/HGX_Driver_GPU_SXM_8");
    result.push_back(
        "/xyz/openbmc_project/inventory_software/HGX_Driver_NVSwitch_0");
    result.push_back(
        "/xyz/openbmc_project/inventory_software/HGX_Driver_NVSwitch_1");
    result.push_back(
        "/xyz/openbmc_project/inventory_software/HGX_Driver_NVSwitch_2");
    result.push_back(
        "/xyz/openbmc_project/inventory_software/HGX_Driver_NVSwitch_3");
    result.push_back("/xyz/openbmc_project/sensors");
    result.push_back("/xyz/openbmc_project/sensors/altitude");
    result.push_back(
        "/xyz/openbmc_project/sensors/altitude/HGX_Chassis_0_AltitudePressure_0");
    result.push_back("/xyz/openbmc_project/sensors/energy");
    result.push_back(
        "/xyz/openbmc_project/sensors/energy/HGX_GPU_SXM_1_Energy_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/energy/HGX_GPU_SXM_2_Energy_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/energy/HGX_GPU_SXM_3_Energy_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/energy/HGX_GPU_SXM_4_Energy_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/energy/HGX_GPU_SXM_5_Energy_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/energy/HGX_GPU_SXM_6_Energy_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/energy/HGX_GPU_SXM_7_Energy_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/energy/HGX_GPU_SXM_8_Energy_0");
    result.push_back("/xyz/openbmc_project/sensors/power");
    result.push_back(
        "/xyz/openbmc_project/sensors/power/HGX_Chassis_0_HSC_0_Power_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/power/HGX_Chassis_0_HSC_1_Power_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/power/HGX_Chassis_0_HSC_2_Power_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/power/HGX_Chassis_0_HSC_3_Power_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/power/HGX_Chassis_0_HSC_4_Power_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/power/HGX_Chassis_0_HSC_5_Power_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/power/HGX_Chassis_0_HSC_6_Power_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/power/HGX_Chassis_0_HSC_7_Power_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/power/HGX_Chassis_0_HSC_8_Power_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/power/HGX_Chassis_0_HSC_9_Power_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/power/HGX_Chassis_0_StandbyHSC_0_Power_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/power/HGX_Chassis_0_TotalGPU_Power_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/power/HGX_Chassis_0_TotalHSC_Power_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/power/HGX_GPU_SXM_1_DRAM_0_Power_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/power/HGX_GPU_SXM_1_Power_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/power/HGX_GPU_SXM_2_DRAM_0_Power_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/power/HGX_GPU_SXM_2_Power_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/power/HGX_GPU_SXM_3_DRAM_0_Power_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/power/HGX_GPU_SXM_3_Power_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/power/HGX_GPU_SXM_4_DRAM_0_Power_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/power/HGX_GPU_SXM_4_Power_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/power/HGX_GPU_SXM_5_DRAM_0_Power_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/power/HGX_GPU_SXM_5_Power_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/power/HGX_GPU_SXM_6_DRAM_0_Power_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/power/HGX_GPU_SXM_6_Power_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/power/HGX_GPU_SXM_7_DRAM_0_Power_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/power/HGX_GPU_SXM_7_Power_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/power/HGX_GPU_SXM_8_DRAM_0_Power_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/power/HGX_GPU_SXM_8_Power_0");
    result.push_back("/xyz/openbmc_project/sensors/temperature");
    result.push_back(
        "/xyz/openbmc_project/sensors/temperature/HGX_Chassis_0_HSC_0_Temp_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/temperature/HGX_Chassis_0_HSC_1_Temp_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/temperature/HGX_Chassis_0_HSC_2_Temp_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/temperature/HGX_Chassis_0_HSC_3_Temp_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/temperature/HGX_Chassis_0_HSC_4_Temp_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/temperature/HGX_Chassis_0_HSC_5_Temp_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/temperature/HGX_Chassis_0_HSC_6_Temp_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/temperature/HGX_Chassis_0_HSC_7_Temp_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/temperature/HGX_Chassis_0_HSC_8_Temp_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/temperature/HGX_Chassis_0_HSC_9_Temp_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/temperature/HGX_Chassis_0_Inlet_0_Temp_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/temperature/HGX_Chassis_0_Inlet_1_Temp_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/temperature/HGX_Chassis_0_PCB_0_Temp_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/temperature/HGX_Chassis_0_PCB_1_Temp_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/temperature/HGX_Chassis_0_PCB_2_Temp_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/temperature/HGX_Chassis_0_PCB_3_Temp_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/temperature/HGX_Chassis_0_StandbyHSC_0_Temp_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/temperature/HGX_FPGA_0_TEMP_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/temperature/HGX_GPU_SXM_1_DRAM_0_Temp_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/temperature/HGX_GPU_SXM_1_TEMP_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/temperature/HGX_GPU_SXM_2_DRAM_0_Temp_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/temperature/HGX_GPU_SXM_2_TEMP_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/temperature/HGX_GPU_SXM_3_DRAM_0_Temp_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/temperature/HGX_GPU_SXM_3_TEMP_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/temperature/HGX_GPU_SXM_4_DRAM_0_Temp_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/temperature/HGX_GPU_SXM_4_TEMP_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/temperature/HGX_GPU_SXM_5_DRAM_0_Temp_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/temperature/HGX_GPU_SXM_5_TEMP_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/temperature/HGX_GPU_SXM_6_DRAM_0_Temp_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/temperature/HGX_GPU_SXM_6_TEMP_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/temperature/HGX_GPU_SXM_7_DRAM_0_Temp_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/temperature/HGX_GPU_SXM_7_TEMP_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/temperature/HGX_GPU_SXM_8_DRAM_0_Temp_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/temperature/HGX_GPU_SXM_8_TEMP_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/temperature/HGX_NVSwitch_0_TEMP_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/temperature/HGX_NVSwitch_1_TEMP_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/temperature/HGX_NVSwitch_2_TEMP_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/temperature/HGX_NVSwitch_3_TEMP_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/temperature/HGX_PCIeRetimer_0_TEMP_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/temperature/HGX_PCIeRetimer_1_TEMP_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/temperature/HGX_PCIeRetimer_2_TEMP_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/temperature/HGX_PCIeRetimer_3_TEMP_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/temperature/HGX_PCIeRetimer_4_TEMP_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/temperature/HGX_PCIeRetimer_5_TEMP_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/temperature/HGX_PCIeRetimer_6_TEMP_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/temperature/HGX_PCIeRetimer_7_TEMP_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/temperature/HGX_PCIeSwitch_0_TEMP_0");
    result.push_back("/xyz/openbmc_project/sensors/voltage");
    result.push_back(
        "/xyz/openbmc_project/sensors/voltage/HGX_GPU_SXM_1_Voltage_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/voltage/HGX_GPU_SXM_2_Voltage_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/voltage/HGX_GPU_SXM_3_Voltage_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/voltage/HGX_GPU_SXM_4_Voltage_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/voltage/HGX_GPU_SXM_5_Voltage_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/voltage/HGX_GPU_SXM_6_Voltage_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/voltage/HGX_GPU_SXM_7_Voltage_0");
    result.push_back(
        "/xyz/openbmc_project/sensors/voltage/HGX_GPU_SXM_8_Voltage_0");
    result.push_back("/xyz/openbmc_project/software");
    result.push_back("/xyz/openbmc_project/software/HGX_FW_FPGA_0");
    result.push_back("/xyz/openbmc_project/software/HGX_FW_GPU_SXM_1");
    result.push_back("/xyz/openbmc_project/software/HGX_FW_GPU_SXM_2");
    result.push_back("/xyz/openbmc_project/software/HGX_FW_GPU_SXM_3");
    result.push_back("/xyz/openbmc_project/software/HGX_FW_GPU_SXM_4");
    result.push_back("/xyz/openbmc_project/software/HGX_FW_GPU_SXM_5");
    result.push_back("/xyz/openbmc_project/software/HGX_FW_GPU_SXM_6");
    result.push_back("/xyz/openbmc_project/software/HGX_FW_GPU_SXM_7");
    result.push_back("/xyz/openbmc_project/software/HGX_FW_GPU_SXM_8");
    result.push_back("/xyz/openbmc_project/software/HGX_FW_NVSwitch_0");
    result.push_back("/xyz/openbmc_project/software/HGX_FW_NVSwitch_1");
    result.push_back("/xyz/openbmc_project/software/HGX_FW_NVSwitch_2");
    result.push_back("/xyz/openbmc_project/software/HGX_FW_NVSwitch_3");
    result.push_back("/xyz/openbmc_project/software/HGX_FW_PCIeRetimer_0");
    result.push_back("/xyz/openbmc_project/software/HGX_FW_PCIeRetimer_1");
    result.push_back("/xyz/openbmc_project/software/HGX_FW_PCIeRetimer_2");
    result.push_back("/xyz/openbmc_project/software/HGX_FW_PCIeRetimer_3");
    result.push_back("/xyz/openbmc_project/software/HGX_FW_PCIeRetimer_4");
    result.push_back("/xyz/openbmc_project/software/HGX_FW_PCIeRetimer_5");
    result.push_back("/xyz/openbmc_project/software/HGX_FW_PCIeRetimer_6");
    result.push_back("/xyz/openbmc_project/software/HGX_FW_PCIeRetimer_7");
    result.push_back("/xyz/openbmc_project/software/HGX_FW_PCIeSwitch_0");
    result.push_back("/xyz/openbmc_project/software/HGX_InfoROM_GPU_SXM_1");
    result.push_back("/xyz/openbmc_project/software/HGX_InfoROM_GPU_SXM_2");
    result.push_back("/xyz/openbmc_project/software/HGX_InfoROM_GPU_SXM_3");
    result.push_back("/xyz/openbmc_project/software/HGX_InfoROM_GPU_SXM_4");
    result.push_back("/xyz/openbmc_project/software/HGX_InfoROM_GPU_SXM_5");
    result.push_back("/xyz/openbmc_project/software/HGX_InfoROM_GPU_SXM_6");
    result.push_back("/xyz/openbmc_project/software/HGX_InfoROM_GPU_SXM_7");
    result.push_back("/xyz/openbmc_project/software/HGX_InfoROM_GPU_SXM_8");
    result.push_back("/xyz/openbmc_project/software/HGX_InfoROM_NVSwitch_0");
    result.push_back("/xyz/openbmc_project/software/HGX_InfoROM_NVSwitch_1");
    result.push_back("/xyz/openbmc_project/software/HGX_InfoROM_NVSwitch_2");
    result.push_back("/xyz/openbmc_project/software/HGX_InfoROM_NVSwitch_3");
    return result;
}

// Autentic event definition (openbmc, commit
// 36b4bec02bb27c6d5146d4776be8f922e6102194)
json event_GPU_VRFailure()
{
    return json{
        {"event", "VR Failure"},
        {"error_id", "GPU_VR_FAILURE-Error"},
        {"device_type", "GPU_SXM_[1-8]"},
        {"category", {"power_rail"}},
        {"event_trigger",
         {{"type", "DBUS"},
          {"object", "/xyz/openbmc_project/GpioStatusHandler"},
          {"interface", "xyz.openbmc_project.GpioStatus"},
          {"property", "I2C3_ALERT"},
          {"check", {{"equal", "false"}}}}},
        {"accessor",
         {{"type", "DeviceCoreAPI"},
          {"device_id", "range"},
          {"property", "gpu.interrupt.powerGoodAbnormalChange"},
          {"check", {{"equal", "1"}}}}},
        {"severity", "Critical"},
        {"resolution", "Power cycle the BaseBoard. If the problem persists, "
                       "isolate the server for RMA evaluation."},
        {"trigger_count", 0},
        {"event_counter_reset", {{"type", ""}, {"metadata", ""}}},
        {"redfish",
         {{"message_id", "ResourceEvent.1.0.ResourceErrorsDetected"},
          {"message_args",
           {{"patterns", {"{GPUId} PowerGood", "Abnormal Power Change"}},
            {"parameters",
             {{{"type", "DIRECT"}, {"field", "CurrentDeviceName"}}}}}}}},
        {"telemetries",
         {{{"name", "GPU Power Good Abnormal change"},
           {"type", "DeviceCoreAPI"},
           {"property", "gpu.interrupt.powerGoodAbnormalChange"}}}},
        {"action", ""},
        {"value_as_count", false}};
}

// Autentic event definition (openbmc, commit
// 36b4bec02bb27c6d5146d4776be8f922e6102194)
json event_GPU_SpiFlashError()
{
    return json{
        {"event", "SPI flash error"},
        {"error_id", "GPU_SPI_FLASH_ERROR-Error"},
        {"device_type", "GPU_SXM_[1-8]"},
        {"event_trigger",
         {{"type", "DBUS"},
          {"object",
           "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_[1-8]"},
          {"interface",
           "xyz.openbmc_project.Inventory.Item.Cpu.OperatingConfig"},
          {"property", "MinSpeed"}}},
        {"accessor",
         {{"type", "CMDLINE"},
          {"executable", "mctp-vdm-util-wrapper"},
          {"arguments", "AP0_SPI_READ_FAILURE GPU_SXM_[1-8]"},
          {"check", {{"equal", "1"}}}}},
        {"severity", "Critical"},
        {"resolution",
         "Cordon HGX server from the cluster for RMA evaluation."},
        {"trigger_count", 0},
        {"event_counter_reset", {{"type", ""}, {"metadata", ""}}},
        {"redfish",
         {{"message_id", "ResourceEvent.1.0.ResourceErrorsDetected"},
          {"message_args",
           {{"patterns", {"{GPUId} Firmware", "SPI Flash Error"}},
            {"parameters",
             {{{"type", "DIRECT"}, {"field", "CurrentDeviceName"}}}}}}}},
        {"telemetries", {}},
        {"action", ""},
        {"value_as_count", false}};
}

// Authentic sub-DAT (for associations defined by test layers) of the
// 'GPU_SXM_1' (openbmc, commit 24d1661e7b47fc43b36a9785abe3ab264bbd2de6)
json testLayersSubDat_GPU_SXM_1()
{
    json js;
    js["ERoT_GPU_SXM_1"]["association"] = {"StandbyHSC_0"};
    js["ERoT_GPU_SXM_1"]["power_rail"] = {
        {{"name", "StandbyHSC_0"},
         {"accessor", {{"type", "DEVICE"}, {"device_name", "StandbyHSC_0"}}},
         {"expected_value", "PASS"}}};
    js["ERoT_GPU_SXM_1"]["pin_status"] = {
        {{"name", "Fatal Error Status"},
         {"accessor",
          {{"type", "CMDLINE"},
           {"executable", "fpga_regtbl_wrapper"},
           {"arguments", "GPU1_EROT_FATAL_ERROR_N GPU_SXM_1"}}},
         {"expected_value", "1"}},
        {{"name", "Recovery Status"},
         {"accessor",
          {{"type", "CMDLINE"},
           {"executable", "fpga_regtbl_wrapper"},
           {"arguments", "FPGA_SXM0_EROT_RECOV_L GPU_SXM_1"}}},
         {"expected_value", "1"}}};
    js["ERoT_GPU_SXM_1"]["erot_control"] = json::array();
    js["ERoT_GPU_SXM_1"]["interface_status"] = json::array();
    js["ERoT_GPU_SXM_1"]["protocol_status"] = json::array();
    js["ERoT_GPU_SXM_1"]["firmware_status"] = {
        {{"name", "Authentication Status"},
         {"accessor",
          {{"type", "CMDLINE"},
           {"executable", "mctp-vdm-util-wrapper"},
           {"arguments", "EC_TAG0_AUTH_ERROR GPU_SXM_1"}}},
         {"expected_value", "0"}},
        {{"name", "Firmware"},
         {"accessor",
          {{"type", "DBUS"},
           {"object", "/xyz/openbmc_project/software/HGX_FW_ERoT_GPU_SXM_1"},
           {"interface", "xyz.openbmc_project.Software.Version"},
           {"property", "Version"}}},
         {"expected_value", ""}}};
    js["ERoT_GPU_SXM_1"]["data_dump"] = {
        {{"name", "Boot status"},
         {"accessor",
          {{"type", "CMDLINE"},
           {"executable", "mctp-vdm-util-wrapper"},
           {"arguments", "query_boot_status GPU_SXM_1"}}},
         {"expected_value", ""}},
        {{"name", "ERoT selftest log"},
         {"accessor",
          {{"type", "CMDLINE"},
           {"executable", "mctp-vdm-util-wrapper"},
           {"arguments", "\"selftest 08 00 00 00\" GPU_SXM_1"}}},
         {"expected_value", ""}}};
    js["PCIeRetimer_0"]["association"] = {"HSC_8"};
    js["PCIeRetimer_0"]["power_rail"] = {
        {{"name", "HSC_8"},
         {"accessor", {{"type", "DEVICE"}, {"device_name", "HSC_8"}}},
         {"expected_value", "PASS"}}};
    js["PCIeRetimer_0"]["pin_status"] = {
        {{"name", "PGOOD Status"},
         {"accessor",
          {{"type", "DBUS"},
           {"object",
            "/xyz/openbmc_project/inventory/system/chassis/HGX_PCIeRetimer_0"},
           {"interface", "xyz.openbmc_project.State.Chassis"},
           {"property", "CurrentPowerState"}}},
         {"expected_value", "xyz.openbmc_project.State.Chassis.PowerState.On"}},
        {{"name", "0V9 Power Enablement"},
         {"accessor",
          {{"type", "CMDLINE"},
           {"executable", "fpga_regtbl_wrapper"},
           {"arguments", "FPGA_RET_0123_0V9_PWR_EN PCIeRetimer_0"}}},
         {"expected_value", "1"}},
        {{"name", "0V9 PGOOD Status"},
         {"accessor",
          {{"type", "CMDLINE"},
           {"executable", "fpga_regtbl_wrapper"},
           {"arguments", "RET0_0123_0V9_PG PCIeRetimer_0"}}},
         {"expected_value", "1"}},
        {{"name", "0V9 PGOOD Interrupt"},
         {"accessor",
          {{"type", "DeviceCoreAPI"},
           {"property", "pcieretimer.0V9.abnormalPowerChange"}}},
         {"expected_value", "0"}},
        {{"name", "1V8LDO Power Enablement"},
         {"accessor",
          {{"type", "CMDLINE"},
           {"executable", "fpga_regtbl_wrapper"},
           {"arguments", "FPGA_RET_0123_1V8LDO_PWR_EN PCIeRetimer_0"}}},
         {"expected_value", "1"}},
        {{"name", "1V8LDO PGOOD Status"},
         {"accessor",
          {{"type", "CMDLINE"},
           {"executable", "fpga_regtbl_wrapper"},
           {"arguments", "RET_0123_1V8LDO_PG PCIeRetimer_0"}}},
         {"expected_value", "1"}},
        {{"name", "1V8LDO PGOOD Interrupt"},
         {"accessor",
          {{"type", "CMDLINE"},
           {"executable", "fpga_regtbl_wrapper"},
           {"arguments", "RET_0123_1V8LDO_INT_PG PCIeRetimer_0"}}},
         {"expected_value", "0"}},
        {{"name", "1V8VDD Power Enablement"},
         {"accessor",
          {{"type", "CMDLINE"},
           {"executable", "fpga_regtbl_wrapper"},
           {"arguments", "FPGA_RET_0123_1V8VDD_PWR_EN PCIeRetimer_0"}}},
         {"expected_value", "1"}},
        {{"name", "1V8VDD PGOOD Status"},
         {"accessor",
          {{"type", "CMDLINE"},
           {"executable", "fpga_regtbl_wrapper"},
           {"arguments", "RET_0123_1V8VDD_PG PCIeRetimer_0"}}},
         {"expected_value", "1"}},
        {{"name", "1V8VDD PGOOD Interrupt"},
         {"accessor",
          {{"type", "DeviceCoreAPI"},
           {"property", "pcieretimer.1V8VDD.abnormalPowerChange"}}},
         {"expected_value", "0"}}};
    js["PCIeRetimer_0"]["erot_control"] = json::array();
    js["PCIeRetimer_0"]["interface_status"] = {
        {{"name", "Clock Enablement"},
         {"accessor",
          {{"type", "DeviceCoreAPI"},
           {"property", "pcieretimer.pcie.clockOutputEnableState"}}},
         {"expected_value", "1"}},
        {{"name", "PCIe AER Status non-fatal error count"},
         {"accessor",
          {{"type", "CMDLINE"},
           {"executable", "pcie_wrapper"},
           {"arguments", "pcie_non_fatal_error_count PCIeRetimer_0"}}},
         {"expected_value", "0"}},
        {{"name", "PCIe AER Status fatal error count"},
         {"accessor",
          {{"type", "CMDLINE"},
           {"executable", "pcie_wrapper"},
           {"arguments", "pcie_fatal_error_count PCIeRetimer_0"}}},
         {"expected_value", "0"}}};
    js["PCIeRetimer_0"]["protocol_status"] = json::array();
    js["PCIeRetimer_0"]["firmware_status"] = {
        {{"name", "Firmware"},
         {"accessor",
          {{"type", "DBUS"},
           {"object", "/xyz/openbmc_project/software/HGX_FW_PCIeRetimer_0"},
           {"interface", "xyz.openbmc_project.Software.Version"},
           {"property", "Version"}}},
         {"expected_value", ""}},
        {{"name", "WriteProtect Status"},
         {"accessor",
          {{"type", "CMDLINE"},
           {"executable", "nvlink-wp-status-wrapper"},
           {"arguments", "pcieretimer_eeprom_wp_status PCIeRetimer_0"}}},
         {"expected_value", ""}}};
    js["PCIeRetimer_0"]["data_dump"] = json::array();
    js["StandbyHSC_0"]["association"] = json::array();
    js["StandbyHSC_0"]["power_rail"] = json::array();
    js["StandbyHSC_0"]["pin_status"] = {
        {{"name", "PGOOD Status"},
         {"accessor",
          {{"type", "DeviceCoreAPI"},
           {"property", "hsc.powerSupply.enablement"}}},
         {"expected_value", "1"}},
        {{"name", "PGOOD Interrupt"},
         {"accessor",
          {{"type", "DeviceCoreAPI"},
           {"property", "hsc.power.abnormalPowerChange"}}},
         {"expected_value", "0"}},
        {{"name", "Alert Status"},
         {"accessor",
          {{"type", "CMDLINE"},
           {"executable", "fpga_regtbl_wrapper"},
           {"arguments", "HSC_STBY_ALERT_N StandbyHSC_0"}}},
         {"expected_value", "1"}},
        {{"name", "Alert Interrupt"},
         {"accessor",
          {{"type", "CMDLINE"},
           {"executable", "fpga_regtbl_wrapper"},
           {"arguments", "HSC_STBY_ALERT_INT_N StandbyHSC_0"}}},
         {"expected_value", "0"}}};
    js["StandbyHSC_0"]["erot_control"] = json::array();
    js["StandbyHSC_0"]["interface_status"] = json::array();
    js["StandbyHSC_0"]["protocol_status"] = json::array();
    js["StandbyHSC_0"]["firmware_status"] = json::array();
    js["StandbyHSC_0"]["data_dump"] = json::array();
    js["HSC_0"]["association"] = json::array();
    js["HSC_0"]["power_rail"] = json::array();
    js["HSC_0"]["pin_status"] = {
        {{"name", "Power Enablement (left)"},
         {"accessor",
          {{"type", "CMDLINE"},
           {"executable", "fpga_regtbl_wrapper"},
           {"arguments", "FPGA_HSC1_LEFT_EN HSC_0"}}},
         {"expected_value", "1"}},
        {{"name", "PGOOD Status"},
         {"accessor",
          {{"type", "DeviceCoreAPI"},
           {"property", "hsc.powerSupply.enablement"}}},
         {"expected_value", "1"}},
        {{"name", "PGOOD Interrupt"},
         {"accessor",
          {{"type", "DeviceCoreAPI"},
           {"property", "hsc.power.abnormalPowerChange"}}},
         {"expected_value", "0"}},
        {{"name", "Alert Status"},
         {"accessor",
          {{"type", "CMDLINE"},
           {"executable", "fpga_regtbl_wrapper"},
           {"arguments", "HSC1_ALERT_N HSC_0"}}},
         {"expected_value", "1"}},
        {{"name", "Alert Interrupt"},
         {"accessor",
          {{"type", "CMDLINE"},
           {"executable", "fpga_regtbl_wrapper"},
           {"arguments", "HSC1_ALERT_INT_N HSC_0"}}},
         {"expected_value", "0"}}};
    js["HSC_0"]["erot_control"] = json::array();
    js["HSC_0"]["interface_status"] = json::array();
    js["HSC_0"]["protocol_status"] = json::array();
    js["HSC_0"]["firmware_status"] = json::array();
    js["HSC_0"]["data_dump"] = json::array();
    js["HSC_8"]["association"] = json::array();
    js["HSC_8"]["power_rail"] = json::array();
    js["HSC_8"]["pin_status"] = {
        {{"name", "Power Enablement (left)"},
         {"accessor",
          {{"type", "CMDLINE"},
           {"executable", "fpga_regtbl_wrapper"},
           {"arguments", "FPGA_HSC9_LEFT_EN HSC_8"}}},
         {"expected_value", "1"}},
        {{"name", "PGOOD Status"},
         {"accessor",
          {{"type", "DeviceCoreAPI"},
           {"property", "hsc.powerSupply.enablement"}}},
         {"expected_value", "1"}},
        {{"name", "PGOOD Interrupt"},
         {"accessor",
          {{"type", "DeviceCoreAPI"},
           {"property", "hsc.power.abnormalPowerChange"}}},
         {"expected_value", "0"}},
        {{"name", "Alert Status"},
         {"accessor",
          {{"type", "CMDLINE"},
           {"executable", "fpga_regtbl_wrapper"},
           {"arguments", "HSC9_ALERT_N HSC_8"}}},
         {"expected_value", "1"}},
        {{"name", "Alert Interrupt"},
         {"accessor",
          {{"type", "CMDLINE"},
           {"executable", "fpga_regtbl_wrapper"},
           {"arguments", "HSC9_ALERT_INT_N HSC_8"}}},
         {"expected_value", "0"}}};
    js["HSC_8"]["erot_control"] = json::array();
    js["HSC_8"]["interface_status"] = json::array();
    js["HSC_8"]["protocol_status"] = json::array();
    js["HSC_8"]["firmware_status"] = json::array();
    js["HSC_8"]["data_dump"] = json::array();
    js["GPU_SXM_1"]["association"] = {"ERoT_GPU_SXM_1", "PCIeRetimer_0",
                                      "HSC_0"};
    js["GPU_SXM_1"]["power_rail"] = {
        {{"name", "HSC_0"},
         {"accessor", {{"type", "DEVICE"}, {"device_name", "HSC_0"}}},
         {"expected_value", "PASS"}}};
    js["GPU_SXM_1"]["pin_status"] = {
        {{"name", "Power Enablement"},
         {"accessor",
          {{"type", "CMDLINE"},
           {"executable", "fpga_regtbl_wrapper"},
           {"arguments", "FPGA_SXM0_PWR_EN GPU_SXM_1"}}},
         {"expected_value", "1"}},
        {{"name", "PGOOD Status From GPU"},
         {"accessor",
          {{"type", "DBUS"},
           {"object",
            "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_1"},
           {"interface", "xyz.openbmc_project.State.Chassis"},
           {"property", "CurrentPowerState"}}},
         {"expected_value", "xyz.openbmc_project.State.Chassis.PowerState.On"}},
        {{"name", "PGOOD Status"},
         {"accessor",
          {{"type", "CMDLINE"},
           {"executable", "fpga_regtbl_wrapper"},
           {"arguments", "GPU1_PWRGD GPU_SXM_1"}}},
         {"expected_value", "1"}},
        {{"name", "PGOOD Interrupt"},
         {"accessor",
          {{"type", "DeviceCoreAPI"},
           {"property", "gpu.interrupt.powerGoodAbnormalChange"}}},
         {"expected_value", "0"}}};
    js["GPU_SXM_1"]["erot_control"] = {
        {{"name", "ERoT_GPU_SXM_1"},
         {"accessor", {{"type", "DEVICE"}, {"device_name", "ERoT_GPU_SXM_1"}}},
         {"expected_value", "PASS"}}};
    js["GPU_SXM_1"]["interface_status"] = {
        {{"name", "I2CDirect Status"},
         {"accessor",
          {{"type", "CMDLINE"},
           {"executable", "i2c_wrapper"},
           {"arguments", "i2c_access GPU_SXM_1"}}},
         {"expected_value", "link-up"}},
        {{"name", "SPI Flash"},
         {"accessor",
          {{"type", "CMDLINE"},
           {"executable", "mctp-vdm-util-wrapper"},
           {"arguments", "AP0_SPI_READ_FAILURE GPU_SXM_1"}}},
         {"expected_value", "0"}},
        {{"name", "Clock Enablement"},
         {"accessor",
          {{"type", "DeviceCoreAPI"},
           {"property", "gpu.pcie.clockOutputEnableState"}}},
         {"expected_value", "1"}},
        {{"name", "PCIe AER Status - non-fatal error count"},
         {"accessor",
          {{"type", "CMDLINE"},
           {"executable", "pcie_wrapper"},
           {"arguments", "pcie_non_fatal_error_count GPU_SXM_1"}}},
         {"expected_value", "0"}},
        {{"name", "PCIe AER Status - fatal error count"},
         {"accessor",
          {{"type", "CMDLINE"},
           {"executable", "pcie_wrapper"},
           {"arguments", "pcie_fatal_error_count GPU_SXM_1"}}},
         {"expected_value", "0"}},
        {{"name", "PCIeRetimer_0"},
         {"accessor", {{"type", "DEVICE"}, {"device_name", "PCIeRetimer_0"}}},
         {"expected_value", "PASS"}},
        {{"name", "NVLink Clock Enablement"},
         {"accessor",
          {{"type", "DeviceCoreAPI"},
           {"property", "gpu.nvlink.clockOutputEnableState"}}},
         {"expected_value", "1"}}};
    js["GPU_SXM_1"]["protocol_status"] = {
        {{"name", "I2CDirect Status"},
         {"accessor",
          {{"type", "CMDLINE"},
           {"executable", "i2c_wrapper"},
           {"arguments", "i2c_access GPU_SXM_1"}}},
         {"expected_value", "link-up"}}};
    js["GPU_SXM_1"]["firmware_status"] = {
        {{"name", "Authentication Status"},
         {"accessor",
          {{"type", "CMDLINE"},
           {"executable", "mctp-vdm-util-wrapper"},
           {"arguments", "active_auth_status GPU_SXM_1"}}},
         {"expected_value", "1"}},
        {{"name", "Boot Completion Status"},
         {"accessor",
          {{"type", "CMDLINE"},
           {"executable", "mctp-vdm-util-wrapper"},
           {"arguments", "AP0_BOOTCOMPLETE_TIMEOUT GPU_SXM_1"}}},
         {"expected_value", "0"}},
        {{"name", "Heartbeat Status"},
         {"accessor",
          {{"type", "CMDLINE"},
           {"executable", "mctp-vdm-util-wrapper"},
           {"arguments", "AP0_HEARTBEAT_TIMEOUT GPU_SXM_1"}}},
         {"expected_value", "0"}},
        {{"name", "Firmware"},
         {"accessor",
          {{"type", "DBUS"},
           {"object", "/xyz/openbmc_project/software/HGX_FW_GPU_SXM_1"},
           {"interface", "xyz.openbmc_project.Software.Version"},
           {"property", "Version"}}},
         {"expected_value", ""}},
        {{"name", "InfoROM"},
         {"accessor",
          {{"type", "DBUS"},
           {"object", "/xyz/openbmc_project/software/HGX_InfoROM_GPU_SXM_1"},
           {"interface", "xyz.openbmc_project.Software.Version"},
           {"property", "Version"}}},
         {"expected_value", ""}},
        {{"name", "WriteProtect Status"},
         {"accessor",
          {{"type", "CMDLINE"},
           {"executable", "nvlink-wp-status-wrapper"},
           {"arguments", "gpu_spiflash_wp_status GPU_SXM_1"}}},
         {"expected_value", ""}}};
    js["GPU_SXM_1"]["data_dump"] = {
        {{"name", "Boot status"},
         {"accessor",
          {{"type", "CMDLINE"},
           {"executable", "mctp-vdm-util-wrapper"},
           {"arguments", "query_boot_status GPU_SXM_1"}}},
         {"expected_value", ""}},
        {{"name", "XID event"},
         {"accessor",
          {{"type", "CMDLINE"},
           {"executable", "echo"},
           {"arguments",
            "To check captured XIDs, Please check URI /redfish/v1/Chassis/HGX_GPU_SXM_1/LogServices/XID/Entries"}}},
         {"expected_value", ""}}};
    return js;
}
