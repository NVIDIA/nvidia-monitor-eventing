/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES.
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

#include "device_id.hpp"
#include "event_info.hpp"
#include "log.hpp"
#include "standalone_qualifier.hpp"

#include <nlohmann/json.hpp>

#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;
using event_info::EventNode;
using json = nlohmann::json;

class StandaloneQualifierTest : public Test
{
  protected:
    static void SetUpTestSuite()
    {
        log_set_level(5);
    }

    // Build a minimally-populated EventNode JSON. The caller supplies the
    // accessor block so each test exercises a different qualifier shape.
    static json makeEventJson(const json& accessor)
    {
        json j;
        j["event"] = "Test Event";
        j["error_id"] = "TEST_QUALIFIER_ERROR";
        j["error_type"] = "GPIO-ALERT";
        j["device_type"] = "GPU_[0-3]";
        j["sub_type"] = "";
        j["severity"] = "Critical";
        j["resolution"] = "Test Resolution";
        j["action"] = "";
        j["trigger_count"] = 0;
        j["telemetries"] = json::array();
        j["event_trigger"] = json::object();
        j["event_counter_reset"] = json::object();
        j["accessor"] = accessor;
        j["redfish"]["message_id"] = "ResourceEvent.1.0.ResourceErrorsDetected";
        j["redfish"]["origin_of_condition"] = "/redfish/v1/Chassis/HGX_GPU_0";
        j["redfish"]["message_args"]["patterns"] = json::array({"GPU_[0-3]"});
        j["redfish"]["message_args"]["parameters"] = json::array();
        return j;
    }

    // Mirrors EventInstantiator: set the concrete device *and* the matched
    // index, because range expansion in the accessor needs the index.
    static EventNode makeEventNode(const json& accessor,
                                   const std::string& device = "GPU_0")
    {
        EventNode node;
        node.loadFrom(makeEventJson(accessor));
        node.device = device;

        device_id::DeviceIdPattern pattern(node.getStringifiedDeviceType());
        auto indices = pattern.match(device);
        if (!indices.empty())
        {
            node.setDeviceIndexTuple(indices[0]);
        }
        return node;
    }
};

// Empty accessor (no qualifier criteria) -> fail-open: event must emit.
TEST_F(StandaloneQualifierTest, EmptyAccessorPasses)
{
    EventNode node = makeEventNode(json::object());
    EXPECT_TRUE(eventing::standaloneQualifierPassed(node));
}

// Accessor present but no "check" key -> fail-open: nothing to evaluate.
TEST_F(StandaloneQualifierTest, AccessorWithoutCheckPasses)
{
    json acc = {
        {"type", "CMDLINE"}, {"executable", "/bin/echo"}, {"arguments", "1"}};
    EventNode node = makeEventNode(acc);
    EXPECT_TRUE(eventing::standaloneQualifierPassed(node));
}

// CMDLINE qualifier whose check.equal matches the command output -> pass.
TEST_F(StandaloneQualifierTest, CmdlineCheckEqualMatchesPasses)
{
    json acc = {{"type", "CMDLINE"},
                {"executable", "/bin/echo"},
                {"arguments", "1"},
                {"check", {{"equal", "1"}}}};
    EventNode node = makeEventNode(acc);
    EXPECT_TRUE(eventing::standaloneQualifierPassed(node));
}

// CMDLINE qualifier whose check.equal does NOT match -> suppress.
TEST_F(StandaloneQualifierTest, CmdlineCheckEqualMismatchSuppresses)
{
    json acc = {{"type", "CMDLINE"},
                {"executable", "/bin/echo"},
                {"arguments", "1"},
                {"check", {{"equal", "0"}}}};
    EventNode node = makeEventNode(acc);
    EXPECT_FALSE(eventing::standaloneQualifierPassed(node));
}

// A range in the accessor arguments must resolve to the targeted device.
// Without an index of matching dimension the literal "GPU_[0-3]" reaches the
// command and every check comparing against a concrete name silently fails.
TEST_F(StandaloneQualifierTest, RangedArgumentsResolveToTargetDevice)
{
    json acc = {{"type", "CMDLINE"},
                {"executable", "/bin/echo"},
                {"arguments", "GPU_[0-3]"},
                {"check", {{"equal", "GPU_2"}}}};
    EventNode node = makeEventNode(acc, "GPU_2");
    EXPECT_TRUE(eventing::standaloneQualifierPassed(node));
}

// The literal pattern must never be what the command receives.
TEST_F(StandaloneQualifierTest, RangedArgumentsAreNotPassedLiterally)
{
    json acc = {{"type", "CMDLINE"},
                {"executable", "/bin/echo"},
                {"arguments", "GPU_[0-3]"},
                {"check", {{"equal", "GPU_[0-3]"}}}};
    EventNode node = makeEventNode(acc, "GPU_1");
    EXPECT_FALSE(eventing::standaloneQualifierPassed(node));
}

// Each device of the range resolves to its own value, not a fixed index 0.
TEST_F(StandaloneQualifierTest, RangedArgumentsResolvePerDevice)
{
    json acc = {{"type", "CMDLINE"},
                {"executable", "/bin/echo"},
                {"arguments", "GPU_[0-3]"},
                {"check", {{"equal", "GPU_0"}}}};
    EventNode first = makeEventNode(acc, "GPU_0");
    EXPECT_TRUE(eventing::standaloneQualifierPassed(first));

    EventNode other = makeEventNode(acc, "GPU_3");
    EXPECT_FALSE(eventing::standaloneQualifierPassed(other));
}

// Bracket mapping (many devices -> one board), as used by the GPU thermal
// events: GPU_[0-1] live on board 0, GPU_[2-3] on board 1.
TEST_F(StandaloneQualifierTest, BracketMappingResolvesToBoard)
{
    json acc = {{"type", "CMDLINE"},
                {"executable", "/bin/echo"},
                {"arguments", "BRD[0-1:0,2-3:1]_RUN_POWER_PG"},
                {"check", {{"equal", "BRD1_RUN_POWER_PG"}}}};
    EventNode node = makeEventNode(acc, "GPU_2");
    EXPECT_TRUE(eventing::standaloneQualifierPassed(node));

    EventNode board0 = makeEventNode(acc, "GPU_0");
    EXPECT_FALSE(eventing::standaloneQualifierPassed(board0));
}

// A multi-token argument list keeps its non-pattern tokens intact, matching
// the "<device> <selection> <op>" shape used by the HSC/HSCC alert qualifier.
TEST_F(StandaloneQualifierTest, RangeResolvesWithinMultiTokenArguments)
{
    json acc = {{"type", "CMDLINE"},
                {"executable", "/bin/echo"},
                {"arguments", "GPU_SMA_[0-3] 128 reason"},
                {"check", {{"equal", "GPU_SMA_2 128 reason"}}}};
    EventNode node = makeEventNode(acc, "GPU_2");
    EXPECT_TRUE(eventing::standaloneQualifierPassed(node));
}

// Fail-open is unchanged: an accessor with no check still emits.
TEST_F(StandaloneQualifierTest, RangedAccessorWithoutCheckStillPasses)
{
    json acc = {{"type", "CMDLINE"},
                {"executable", "/bin/echo"},
                {"arguments", "GPU_[0-3]"}};
    EventNode node = makeEventNode(acc, "GPU_2");
    EXPECT_TRUE(eventing::standaloneQualifierPassed(node));
}
