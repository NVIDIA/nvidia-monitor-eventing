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
