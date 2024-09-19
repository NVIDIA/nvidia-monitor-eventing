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

#include "json_proc.hpp"
#include "tests_common_defs.hpp"

#include <fstream>

#include <gtest/gtest.h>

namespace json_proc
{

// marcinw:TODO: what's really the issue with testing json equality?
// static void testEqual(const nlohmann::json& left, const nlohmann::json&
// right)
// {
//     nlohmann::ordered_json orderedLeft = left;
//     nlohmann::ordered_json orderedRight = right;
//     EXPECT_EQ(orderedLeft.dump(), orderedRight.dump());
// }

TEST(JsonProcTest, JsonPath_contains)
{
    auto js = event_GPU_VRFailure();
    EXPECT_TRUE(contains(js, JsonPath({})));
    EXPECT_TRUE(contains(js, JsonPath({"event"})));
    EXPECT_TRUE(contains(js, JsonPath({"telemetries", 0u})));
    EXPECT_FALSE(contains(js, JsonPath({"telemetries", 1u})));
    EXPECT_FALSE(contains(js, JsonPath({"event_trigger", "device_id"})));
}

TEST(JsonProcTest, JsonPath_nodeAt)
{
    auto js = event_GPU_VRFailure();
    EXPECT_EQ(nodeAt(js, JsonPath({})), js);
    EXPECT_EQ(nodeAt(js, JsonPath({"event"})), nlohmann::json("VR Failure"));
    EXPECT_EQ(nodeAt(js, JsonPath({"telemetries", 0u})),
              nlohmann::json(
                  {{"name", "GPU Power Good Abnormal change"},
                   {"type", "DeviceCoreAPI"},
                   {"property", "gpu.interrupt.powerGoodAbnormalChange"}}));
    EXPECT_ANY_THROW(nodeAt(js, JsonPath({"telemetries", 1u})));
    EXPECT_ANY_THROW(nodeAt(js, JsonPath({"event_trigger", "device_id"})));
}

TEST(JsonProcTest, JsonPattern_eval)
{
    nlohmann::json js{
        {"type", "DBUS"},
        {"object", "/.../Switches/NVSwitch_[0-3]/Ports/NVLink_[0-39]"},
        {"interface", "xyz.openbmc_project.Inventory.Item.Port"},
        {"property", "FlitCRCCount"},
        {"check", {{"not_equal", "0"}}}};
    JsonPattern jsp(js);
    EXPECT_EQ(jsp.eval(device_id::PatternIndex(1, 12)),
              nlohmann::json(
                  {{"type", "DBUS"},
                   {"object", "/.../Switches/NVSwitch_1/Ports/NVLink_12"},
                   {"interface", "xyz.openbmc_project.Inventory.Item.Port"},
                   {"property", "FlitCRCCount"},
                   {"check", {{"not_equal", "0"}}}}));
    EXPECT_EQ(jsp.eval(device_id::PatternIndex(3, 0)),
              nlohmann::json(
                  {{"type", "DBUS"},
                   {"object", "/.../Switches/NVSwitch_3/Ports/NVLink_0"},
                   {"interface", "xyz.openbmc_project.Inventory.Item.Port"},
                   {"property", "FlitCRCCount"},
                   {"check", {{"not_equal", "0"}}}}));
    EXPECT_ANY_THROW(jsp.eval(device_id::PatternIndex(3)));
    EXPECT_ANY_THROW(jsp.eval(device_id::PatternIndex()));
    EXPECT_ANY_THROW(jsp.eval(device_id::PatternIndex(4, 0)));
}

TEST(JsonProcTest, JsonPattern_eval1)
{
    nlohmann::json js{
        {"message_id", "ResourceEvent.1.0.ResourceErrorsDetected"},
        {"message_args",
         {{"patterns",
           nlohmann::json::array(
               {"NVSwitch_[0|0-3] NVLink_[1|0-17]", "Flit CRC Errors"})},
          {"parameters", nlohmann::json::array()}}}};
    JsonPattern jsp(js);
    EXPECT_EQ(jsp.eval(device_id::PatternIndex(1, 12)),
              nlohmann::json(
                  {{"message_id", "ResourceEvent.1.0.ResourceErrorsDetected"},
                   {"message_args",
                    {{"patterns", nlohmann::json::array({"NVSwitch_1 NVLink_12",
                                                         "Flit CRC Errors"})},
                     {"parameters", nlohmann::json::array()}}}}));
    EXPECT_EQ(jsp.eval(device_id::PatternIndex(0, 1)),
              nlohmann::json(
                  {{"message_id", "ResourceEvent.1.0.ResourceErrorsDetected"},
                   {"message_args",
                    {{"patterns", nlohmann::json::array({"NVSwitch_0 NVLink_1",
                                                         "Flit CRC Errors"})},
                     {"parameters", nlohmann::json::array()}}}}));
    EXPECT_ANY_THROW(jsp.eval(device_id::PatternIndex(3)));
    EXPECT_ANY_THROW(jsp.eval(device_id::PatternIndex()));
    EXPECT_ANY_THROW(jsp.eval(device_id::PatternIndex(4, 0)));
}

} // namespace json_proc
