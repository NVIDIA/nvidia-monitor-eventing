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

#pragma once
#include "json_schema.hpp"

#include <memory>

namespace mon_evt
{

constexpr auto SERVICE_BUSNAME = "xyz.openbmc_project.MON_EVT";
constexpr auto TOP_OBJPATH = "/xyz/openbmc_project/MON_EVT";
constexpr auto SERVICE_IFCNAME = "xyz.openbmc_project.MON_EVT";

} // namespace mon_evt

namespace eventing
{

std::shared_ptr<json_schema::JsonSchema> dataAccessorSchema();
std::shared_ptr<json_schema::JsonSchema> eventNodeJsonSchema();
std::shared_ptr<json_schema::JsonSchema> eventInfoJsonSchema();

std::shared_ptr<json_schema::JsonSchema> datSchema();

} // namespace eventing
