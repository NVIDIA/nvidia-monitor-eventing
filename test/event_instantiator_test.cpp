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
