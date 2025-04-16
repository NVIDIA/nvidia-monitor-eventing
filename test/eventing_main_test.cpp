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

// test/eventing_main_test.cpp

#include "event_info.hpp"
#include "eventing_main.hpp"
#include "log.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;
using json = nlohmann::json;

class EventingMainTest : public Test
{
  protected:
    void SetUp() override
    {
        // Reset global state before each test
        eventing::profile::eventMap.clear();
        eventing::profile::propertyFilterSet.clear();
        eventing::profile::eventTriggerView.clear();
        eventing::profile::eventAccessorView.clear();
        eventing::profile::eventRecoveryView.clear();

        // Set up default configuration
        eventing::configuration.standaloneMode = true;
        eventing::configuration.event = "test_events.json";
    }

    // Helper to create a mock event definition JSON
    json createMockEventJson(const std::string& errorId,
                             const std::string& devicePattern)
    {
        return json{
            {"event", "Test Event"},
            {"error_id", errorId},
            {"device_type", devicePattern},
            {"redfish",
             {{"message_id", "ResourceEvent.1.0.ResourceErrorsDetected"},
              {"origin_of_condition", "/redfish/v1/Chassis/Test"},
              {"message_args", {{"parameters", {}}, {"patterns", {}}}}}}};
    }
};

TEST_F(EventingMainTest, LoadEventDefinitionsSuccess)
{
    // Create temp event file
    std::string eventJson = R"({
        "events": [{
            "event": "Test Event",
            "error_id": "TEST_ERROR_1",
            "device_type": "SMA_[0-11]",
            "redfish": {
                "message_id": "ResourceEvent.1.0",
                "origin_of_condition": "/redfish/v1/Chassis/Test"
            }
        }]
    })";

    // Write to temp file
    std::ofstream eventFile("test_events.json");
    eventFile << eventJson;
    eventFile.close();

    // Test loading
    EXPECT_NO_THROW(event_info::loadFromFile(
        eventing::profile::eventMap, eventing::profile::propertyFilterSet,
        eventing::profile::eventTriggerView,
        eventing::profile::eventAccessorView,
        eventing::profile::eventRecoveryView, "test_events.json"));

    EXPECT_FALSE(eventing::profile::eventMap.empty());

    // Cleanup
    std::remove("test_events.json");
}

TEST_F(EventingMainTest, FindEventByErrorId)
{
    // Setup test event
    json eventJson = {{"events",
                       {createMockEventJson("TEST_ERROR_1", "SMA_[0-11]"),
                        createMockEventJson("TEST_ERROR_2", "GPU_[0-7]")}}};

    std::ofstream eventFile("test_events.json");
    eventFile << eventJson.dump();
    eventFile.close();

    // Load events
    event_info::loadFromFile(
        eventing::profile::eventMap, eventing::profile::propertyFilterSet,
        eventing::profile::eventTriggerView,
        eventing::profile::eventAccessorView,
        eventing::profile::eventRecoveryView, "test_events.json");

    // Test finding event
    eventing::configuration.errorId = "TEST_ERROR_1";
    eventing::configuration.deviceId = "SMA_0";

    auto it = std::find_if(
        eventing::profile::eventMap.begin(), eventing::profile::eventMap.end(),
        [&](const auto& pair) {
            return std::any_of(
                pair.second.begin(), pair.second.end(), [&](const auto& event) {
                    return event.errorId == eventing::configuration.errorId;
                });
        });

    EXPECT_NE(it, eventing::profile::eventMap.end());

    // Cleanup
    std::remove("test_events.json");
}

TEST_F(EventingMainTest, ErrorIdNotFound)
{
    // Setup test event
    json eventJson = {
        {"events", {createMockEventJson("TEST_ERROR_1", "SMA_[0-11]")}}};

    std::ofstream eventFile("test_events.json");
    eventFile << eventJson.dump();
    eventFile.close();

    // Load events
    event_info::loadFromFile(
        eventing::profile::eventMap, eventing::profile::propertyFilterSet,
        eventing::profile::eventTriggerView,
        eventing::profile::eventAccessorView,
        eventing::profile::eventRecoveryView, "test_events.json");

    // Test with non-existent error ID
    eventing::configuration.errorId = "NONEXISTENT_ERROR";
    eventing::configuration.deviceId = "SMA_0";

    auto it = std::find_if(
        eventing::profile::eventMap.begin(), eventing::profile::eventMap.end(),
        [&](const auto& pair) {
            return std::any_of(
                pair.second.begin(), pair.second.end(), [&](const auto& event) {
                    return event.errorId == eventing::configuration.errorId;
                });
        });

    EXPECT_EQ(it, eventing::profile::eventMap.end());

    // Cleanup
    std::remove("test_events.json");
}

TEST_F(EventingMainTest, InvalidEventFile)
{
    eventing::configuration.event = "nonexistent.json";

    EXPECT_THROW(event_info::loadFromFile(eventing::profile::eventMap,
                                          eventing::profile::propertyFilterSet,
                                          eventing::profile::eventTriggerView,
                                          eventing::profile::eventAccessorView,
                                          eventing::profile::eventRecoveryView,
                                          eventing::configuration.event),
                 std::runtime_error);
}

// Helper class to mock the EventNode
class MockEventNode : public event_info::EventNode
{
  public:
    MockEventNode() : EventNode("MockEvent")
    {}

    // Add message args to simulate real events
    void setupMessageArgs(int numArgs)
    {
        for (int i = 0; i < numArgs; i++)
        {
            event_info::MessageArg msgArg;
            msgArg.pattern.pattern =
                "Test Message Arg " + std::to_string(i + 1);
            messageRegistry.messageArgs.push_back(msgArg);
        }
    }
};

// Test fixture for standalone mode tests
class StandaloneModeTest : public EventingMainTest
{
  protected:
    void SetUp() override
    {
        EventingMainTest::SetUp();

        // Set up standalone mode configuration
        eventing::configuration.standaloneMode = true;
        eventing::configuration.errorId = "TEST_ERROR_ID";
        eventing::configuration.deviceId = "GPU_0";
        eventing::configuration.msgArg2 = "";

        // Create test event file with proper complete event definition
        std::string eventJson = R"({
            "GPU": [{
                "event": "Test GPU Event",
                "error_id": "TEST_ERROR_ID",
                "device_type": "GPU_[0-7]",
                "sub_type": "",
                "severity": "Critical",
                "resolution": "Contact NVIDIA Support",
                "redfish": {
                    "message_id": "ResourceEvent.1.0.ResourceErrorsDetected",
                    "message_args": {
                        "patterns": ["GPU_{0}", "Custom Error Message"],
                        "parameters": [
                            {
                                "type": "DIRECT",
                                "field": "CurrentDeviceIndex"
                            }
                        ]
                    }
                },
                "telemetries": [],
                "trigger_count": 1,
                "event_trigger": {},
                "action": "test action",
                "event_counter_reset": {},
                "accessor": {
                    "type": "DBUS",
                    "interface": "xyz.openbmc_project.Inventory.Item",
                    "object": "/xyz/openbmc_project/inventory/system/chassis/GPU_{0}",
                    "property": "Present"
                },
                "value_as_count": false
            }]
        })";

        // Write to temp file
        std::ofstream eventFile("test_events.json");
        eventFile << eventJson;
        eventFile.close();
    }

    void TearDown() override
    {
        // Clean up
        std::remove("test_events.json");
        EventingMainTest::TearDown();
    }
};

// Test loading an event by error ID in standalone mode
TEST_F(StandaloneModeTest, LoadEventByErrorId)
{
    auto eventNode = event_info::EventNode::loadEventByErrorId(
        eventing::configuration.errorId, eventing::configuration.event);

    ASSERT_NE(eventNode, nullptr);
    EXPECT_EQ(eventNode->errorId, eventing::configuration.errorId);
    EXPECT_EQ(eventNode->event, "Test GPU Event");
}

// Test handling non-existent error ID in standalone mode
TEST_F(StandaloneModeTest, ErrorIdNotFound)
{
    eventing::configuration.errorId = "NONEXISTENT_ERROR";

    auto eventNode = event_info::EventNode::loadEventByErrorId(
        eventing::configuration.errorId, eventing::configuration.event);

    EXPECT_EQ(eventNode, nullptr);
}

// Test device ID pattern matching
TEST_F(StandaloneModeTest, DevicePatternMatch)
{
    auto eventNode = event_info::EventNode::loadEventByErrorId(
        eventing::configuration.errorId, eventing::configuration.event);

    ASSERT_NE(eventNode, nullptr);

    // Valid device ID within pattern range
    EXPECT_TRUE(eventNode->isDeviceTypeMatch("GPU_0"));
    EXPECT_TRUE(eventNode->isDeviceTypeMatch("GPU_7"));

    // Invalid device IDs outside range
    EXPECT_FALSE(eventNode->isDeviceTypeMatch("GPU_8"));
    EXPECT_FALSE(eventNode->isDeviceTypeMatch("CPU_0"));
}

// Test the message_arg2 setting with enough message args
TEST_F(StandaloneModeTest, SetMessageArg2Success)
{
    MockEventNode eventNode;
    eventNode.setupMessageArgs(2); // Create 2 message args

    const std::string customMsg = "Custom Error Message";
    EXPECT_TRUE(eventNode.setMessageArg2(customMsg));

    ASSERT_GE(eventNode.messageRegistry.messageArgs.size(), 2);
    EXPECT_EQ(eventNode.messageRegistry.messageArgs[1].pattern.pattern,
              customMsg);
}

// Test the message_arg2 setting with insufficient message args
TEST_F(StandaloneModeTest, SetMessageArg2InsufficientArgs)
{
    MockEventNode eventNode;
    eventNode.setupMessageArgs(1); // Create only 1 message arg

    const std::string customMsg = "Custom Error Message";
    EXPECT_FALSE(eventNode.setMessageArg2(customMsg));

    EXPECT_EQ(eventNode.messageRegistry.messageArgs.size(),
              1); // Should not add any args
}

// Test the message_arg2 setting with no message args
TEST_F(StandaloneModeTest, SetMessageArg2NoArgs)
{
    MockEventNode eventNode;
    // No message args setup

    const std::string customMsg = "Custom Error Message";
    EXPECT_FALSE(eventNode.setMessageArg2(customMsg));

    EXPECT_EQ(eventNode.messageRegistry.messageArgs.size(),
              0); // Should not add any args
}

// Integration test for full standalone mode with message_arg2
TEST_F(StandaloneModeTest, StandaloneModeWithMessageArg2)
{
    // Setup
    eventing::configuration.msgArg2 = "Custom Message From Test";

    // Load event
    auto eventNode = event_info::EventNode::loadEventByErrorId(
        eventing::configuration.errorId, eventing::configuration.event);

    ASSERT_NE(eventNode, nullptr);

    // Set device and apply message_arg2
    eventNode->device = eventing::configuration.deviceId;
    EXPECT_TRUE(eventNode->setMessageArg2(eventing::configuration.msgArg2));

    // Verify
    ASSERT_GE(eventNode->messageRegistry.messageArgs.size(), 2);
    EXPECT_EQ(eventNode->messageRegistry.messageArgs[1].pattern.pattern,
              eventing::configuration.msgArg2);
}

// Test handling of device tuple indices with message_arg2
TEST_F(StandaloneModeTest, DeviceIndexTupleWithMessageArg2)
{
    // Setup
    eventing::configuration.msgArg2 = "Custom Message From Test";

    // Load event
    auto eventNode = event_info::EventNode::loadEventByErrorId(
        eventing::configuration.errorId, eventing::configuration.event);

    ASSERT_NE(eventNode, nullptr);

    // Set device
    eventNode->device = eventing::configuration.deviceId;

    // Set device index tuple
    device_id::DeviceIdPattern pattern(eventNode->getStringifiedDeviceType());
    auto indices = pattern.match(eventing::configuration.deviceId);
    ASSERT_FALSE(indices.empty());
    eventNode->setDeviceIndexTuple(indices[0]);

    // Apply message_arg2
    EXPECT_TRUE(eventNode->setMessageArg2(eventing::configuration.msgArg2));

    // Check message args after evaluating with device index
    std::string messageArgs = eventNode->getStringMessageArgs();
    EXPECT_NE(messageArgs.find("GPU_0"),
              std::string::npos); // First arg should contain device
    EXPECT_NE(messageArgs.find(eventing::configuration.msgArg2),
              std::string::npos); // Second arg is our custom message
}

// Add more tests as necessary...
