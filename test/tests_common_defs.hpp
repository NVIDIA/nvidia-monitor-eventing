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

#include "dbus_accessor.hpp"

#include <nlohmann/json.hpp>

class DummyObjectMapper : public dbus::ObjectMapper<DummyObjectMapper>
{

  public:
    std::vector<std::string>
        getSubTreePathsImpl(sdbusplus::bus::bus& bus,
                            const std::string& subtree, int depth,
                            const std::vector<std::string>& interfaces);
};

nlohmann::json event_GPU_VRFailure();
nlohmann::json event_GPU_SpiFlashError();
nlohmann::json testLayersSubDat_GPU_SXM_1();
