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

#include "device_id.hpp"
#include "event_info.hpp"
#include "event_instantiator.hpp"
#include "log.hpp"

#include <memory>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;
using namespace event_handler;
using namespace event_info;

// Test fixture for EventInstantiator
class EventInstantiatorTest : public Test
{
  protected:
    // Static initializer to set log level for all tests
    static void SetUpTestSuite()
    {
        log_set_level(5);
    }

    void SetUp() override
    {
        // Create a test event instantiator
        eventInstantiator =
            std::make_unique<EventInstantiator>("TestInstantiator");
    }

    void TearDown() override
    {
        eventInstantiator.reset();
    }

    // Helper method to create a basic event node with patterns
    EventNode createTestEventNode(const std::string& deviceType,
                                  const std::string& oocPattern = "",
                                  const std::string& messageArg0 = "",
                                  const std::string& logNamespace = "")
    {
        std::string realOocPattern =
            (oocPattern == "") ? deviceType : oocPattern;
        std::string realMessageArg0 =
            (messageArg0 == "") ? deviceType : messageArg0;
        std::string realLogNamespace =
            (logNamespace == "") ? deviceType : logNamespace;

        nlohmann::json eventJson;
        eventJson["event"] = "Test Event";
        eventJson["error_id"] = "TEST_ERROR_ID";
        eventJson["device_type"] = deviceType;
        eventJson["trigger_count"] = 0;
        eventJson["severity"] = "Critical";
        eventJson["resolution"] = "Test Resolution";
        eventJson["action"] = "Test Action";
        eventJson["accessor"] = nlohmann::json::object();
        eventJson["event_trigger"] = nlohmann::json::object();
        eventJson["event_counter_reset"] = nlohmann::json::object();
        eventJson["telemetries"] = nlohmann::json::array();
        eventJson["redfish"]["message_id"] =
            "ResourceEvent.1.0.ResourceErrorsDetected";
        eventJson["redfish"]["origin_of_condition"] = realOocPattern;

        // Initialize message_args with empty arrays
        eventJson["redfish"]["message_args"]["patterns"][0] = realMessageArg0;
        eventJson["log_namespace"] = realLogNamespace;

        EventNode eventNode;
        eventNode.loadFrom(eventJson);
        return eventNode;
    }

    std::unique_ptr<EventInstantiator> eventInstantiator;
};

// Test the constructor
TEST_F(EventInstantiatorTest, Constructor)
{
    ASSERT_NE(eventInstantiator, nullptr);
    EXPECT_EQ(eventInstantiator->getName(), "TestInstantiator");
}

// Test processing an event node with a simple device pattern
TEST_F(EventInstantiatorTest, ProcessSimpleDevicePattern)
{
    // Create a test event node with a simple pattern
    EventNode eventNode = createTestEventNode("SXM_SMA_[0-7]");
    eventNode.device = "SXM_SMA_0";

    // Process the event
    eventing::RcCode result = eventInstantiator->process(eventNode);

    // Check the result
    ASSERT_EQ(result, eventing::RcCode::succ)
        << "Failed to process event with simple device pattern";

    // Get the instantiated event
    const EventNode& instantiatedEvent =
        eventInstantiator->getInstantiatedEvent();

    // Verify the result
    EXPECT_EQ(instantiatedEvent.device, "SXM_SMA_0");
    EXPECT_TRUE(instantiatedEvent.isEventNodeEvaluated());
    EXPECT_EQ(instantiatedEvent.event, "Test Event");
    EXPECT_EQ(instantiatedEvent.errorId, "TEST_ERROR_ID");
}

using ::testing::HasSubstr;
// Test processing an event node with a more complex device pattern
TEST_F(EventInstantiatorTest, ProcessComplexDevicePattern)
{
    // Create a test event node with a complex pattern
    EventNode eventNode = createTestEventNode(
        "SXM_SMA_[0-7]", "GPU_SXM_[0|0-7:1-8]", "SXM_SMA_[0-7]", "Baseboard_0");
    eventNode.device = "SXM_SMA_0";

    // Process the event
    eventing::RcCode result = eventInstantiator->process(eventNode);

    // Check the result
    ASSERT_EQ(result, eventing::RcCode::succ)
        << "Failed to process event with complex device pattern";

    // Get the instantiated event
    EventNode& instantiatedEvent = eventInstantiator->getInstantiatedEvent();

    // Verify the result
    EXPECT_EQ(instantiatedEvent.device, "SXM_SMA_0");
    EXPECT_TRUE(instantiatedEvent.isEventNodeEvaluated());
    EXPECT_EQ(instantiatedEvent.originOfCondition, "GPU_SXM_1");
    EXPECT_THAT(instantiatedEvent.getStringMessageArgs(),
                HasSubstr("SXM_SMA_0"));
    EXPECT_EQ(instantiatedEvent.logNamespace, "Baseboard_0");
}

// Test trying to get instantiated event without processing
TEST_F(EventInstantiatorTest, GetInstantiatedEventWithoutProcessing)
{
    // Try to get the instantiated event without processing any event
    EXPECT_THROW(eventInstantiator->getInstantiatedEvent(), std::runtime_error);
}

// Test that the accessor's patterned fields are baked with the device index.
TEST_F(EventInstantiatorTest, AccessorIsInstantiated)
{
    EventNode eventNode = createTestEventNode("GPU_SXM_[1-8]");
    nlohmann::json acc = {
        {"type", "DBUS"},
        {"object", "/xyz/openbmc_project/inventory/GPU_SXM_[1-8]"},
        {"interface", "xyz.openbmc_project.Inventory.Item"},
        {"property", "Present"}};
    eventNode.accessor = acc;
    eventNode.device = "GPU_SXM_3";

    ASSERT_EQ(eventInstantiator->process(eventNode), eventing::RcCode::succ);

    const EventNode& out = eventInstantiator->getInstantiatedEvent();
    EXPECT_EQ(out.accessor["object"],
              "/xyz/openbmc_project/inventory/GPU_SXM_3");
    EXPECT_EQ(out.accessor["property"], "Present");
}

// CMDLINE accessor: 'arguments' range must be baked.
TEST_F(EventInstantiatorTest, AccessorCmdlineArgumentsInstantiated)
{
    EventNode eventNode = createTestEventNode("GPU_[0-7]");
    nlohmann::json acc = {{"type", "CMDLINE"},
                          {"executable", "gpio_parser"},
                          {"arguments", "BRD_PG-I GPU_[0-7]"}};
    eventNode.accessor = acc;
    eventNode.device = "GPU_5";

    ASSERT_EQ(eventInstantiator->process(eventNode), eventing::RcCode::succ);

    const EventNode& out = eventInstantiator->getInstantiatedEvent();
    EXPECT_EQ(out.accessor["arguments"], "BRD_PG-I GPU_5");
}

// No-range accessors must pass through unchanged.
TEST_F(EventInstantiatorTest, AccessorWithoutRangeIsUnchanged)
{
    EventNode eventNode = createTestEventNode("GPU_[0-7]");
    nlohmann::json acc = {{"type", "DBUS"},
                          {"object", "/xyz/openbmc_project/state/host0"},
                          {"interface", "xyz.openbmc_project.State.Host"},
                          {"property", "CurrentHostState"}};
    eventNode.accessor = acc;
    eventNode.device = "GPU_2";

    ASSERT_EQ(eventInstantiator->process(eventNode), eventing::RcCode::succ);

    const EventNode& out = eventInstantiator->getInstantiatedEvent();
    EXPECT_EQ(out.accessor["object"], "/xyz/openbmc_project/state/host0");
}

// Real entry from meta-vr-nvl-hmc event_info.json line 14 — "GPU GPU NVLink
// Training Error". Exercises full instantiation of accessor + trigger +
// recovery_accessor + telemetries[] with the 2-dim DBUS object range
// GPU_[0-3]/NVLink_[0-17].
TEST_F(EventInstantiatorTest, AccessorRealNvLinkTrainingError)
{
    EventNode eventNode = createTestEventNode("GPU_[0-3]/NVLink_[0-17]");

    nlohmann::json acc = {
        {"type", "DBUS"},
        {"object",
         "/xyz/openbmc_project/inventory/system/accelerator/GPU_[0-3]/Ports/NVLink_[0-17]"},
        {"interface", "xyz.openbmc_project.Metrics.PortMetricsOem3"},
        {"property", "TrainingError"},
        {"check", {{"not_equal", "0"}}}};
    nlohmann::json recovery = {
        {"type", "DBUS"},
        {"object",
         "/xyz/openbmc_project/inventory/system/accelerator/GPU_[0-3]/Ports/NVLink_[0-17]"},
        {"interface", "xyz.openbmc_project.Metrics.PortMetricsOem3"},
        {"property", "TrainingError"},
        {"check", {{"equal", "0"}}}};
    nlohmann::json telem = {
        {"name", "NVLink Runtime Error Count"},
        {"type", "DBUS"},
        {"object",
         "/xyz/openbmc_project/inventory/system/accelerator/GPU_[0-3]/Ports/NVLink_[0-17]"},
        {"interface", "xyz.openbmc_project.Metrics.PortMetricsOem3"},
        {"property", "RuntimeError"}};

    eventNode.accessor = acc;
    // event_trigger is empty in the real entry, so trigger == accessor.
    eventNode.trigger = acc;
    eventNode.recovery_accessor = recovery;
    eventNode.telemetries.clear();
    eventNode.telemetries.emplace_back(telem);
    eventNode.device = "GPU_2/NVLink_15";

    ASSERT_EQ(eventInstantiator->process(eventNode), eventing::RcCode::succ);

    const EventNode& out = eventInstantiator->getInstantiatedEvent();
    const std::string expectedObj =
        "/xyz/openbmc_project/inventory/system/accelerator/GPU_2/Ports/NVLink_15";

    EXPECT_EQ(out.accessor["object"], expectedObj);
    EXPECT_EQ(out.accessor["interface"],
              "xyz.openbmc_project.Metrics.PortMetricsOem3");
    EXPECT_EQ(out.accessor["property"], "TrainingError");

    EXPECT_EQ(out.trigger["object"], expectedObj);
    EXPECT_EQ(out.recovery_accessor["object"], expectedObj);

    ASSERT_EQ(out.telemetries.size(), 1u);
    EXPECT_EQ(out.telemetries[0]["object"], expectedObj);
    EXPECT_EQ(out.telemetries[0]["property"], "RuntimeError");

    // redfish.message_args.parameters is [] in this real entry — verify the
    // empty-vector path is a no-op (no crash).
    EXPECT_TRUE(out.messageRegistry.messageArgs.empty() ||
                out.messageRegistry.messageArgs[0].parameters.empty());
}

// Real entry from meta-vr-nvl-hmc event_info.json line 1475 — "GPU GPU
// Overtemp Indicator". CMDLINE accessor; trigger falls back to accessor (empty
// event_trigger); recovery and telemetries are absent and must stay empty.
TEST_F(EventInstantiatorTest, AccessorRealGpuOvertempCmdline)
{
    EventNode eventNode = createTestEventNode("GPU_[0-3]");
    nlohmann::json acc = {{"type", "CMDLINE"},
                          {"executable", "gpio_parser"},
                          {"arguments", "BRD[0-1:0,2-3:1]_RUN_POWER_PG-I"},
                          {"check", {{"equal", "1"}}}};

    eventNode.accessor = acc;
    eventNode.trigger = acc; // event_trigger empty => trigger == accessor
    eventNode.device = "GPU_2";

    ASSERT_EQ(eventInstantiator->process(eventNode), eventing::RcCode::succ);

    const EventNode& out = eventInstantiator->getInstantiatedEvent();
    EXPECT_EQ(out.accessor["arguments"], "BRD1_RUN_POWER_PG-I");
    EXPECT_EQ(out.accessor["executable"], "gpio_parser");

    EXPECT_EQ(out.trigger["arguments"], "BRD1_RUN_POWER_PG-I");

    // recovery_accessor and telemetries are absent in this real entry; the
    // instantiator must handle them as no-ops.
    EXPECT_TRUE(out.recovery_accessor.isEmpty());
    EXPECT_TRUE(out.telemetries.empty());
}
