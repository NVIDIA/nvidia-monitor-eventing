/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES.
 * All rights reserved. SPDX-License-Identifier: Apache-2.0
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

#include "tests_common_defs.hpp"

#include <cassert>

#include "gmock/gmock.h"
using namespace testing;

TEST(ObjectMapper, getAllDevIdObjPaths)
{
    DummyObjectMapper om;
    EXPECT_THAT(
        om.getAllDevIdObjPaths("GPU_SXM_3"),
        UnorderedElementsAre(
            "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_3",
            "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_3/PCIeDevices/GPU_SXM_3",
            "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Endpoints/GPU_SXM_3",
            "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_2/Endpoints/GPU_SXM_3",
            "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_3"));
}

TEST(ObjectMapper, getPrimaryDevIdPaths)
{
    DummyObjectMapper om;
    EXPECT_THAT(
        om.getPrimaryDevIdPaths("GPU_SXM_3"),
        UnorderedElementsAre(
            "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_3"));
}

TEST(ObjectMapper, getPrimaryDevIdPaths1)
{
    DummyObjectMapper om;
    EXPECT_THAT(
        om.getPrimaryDevIdPaths("PCIeSwitch_0"),
        UnorderedElementsAre(
            "/xyz/openbmc_project/inventory/system/chassis/HGX_PCIeSwitch_0"));
}
