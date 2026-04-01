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

#include "event_instantiator.hpp"

#include "json_proc.hpp"
#include "log.hpp"

namespace event_handler
{

EventInstantiator::EventInstantiator(const std::string& name) :
    EventHandler(name), event_instance(nullptr)
{
    log_dbg("Creating EventInstantiator: %s.\n", name.c_str());
}

eventing::RcCode EventInstantiator::process(event_info::EventNode& event_node)
{
    // Check if the node is already instantiated
    if (event_instance)
    {
        log_dbg("Reusing existing event instance for %s.\n",
                event_node.event.c_str());
        return eventing::RcCode::succ;
    }

    try
    {
        // Check if device is set
        if (event_node.device.empty())
        {
            log_err("Device name is not set in the event node.\n");
            return eventing::RcCode::error;
        }

        log_dbg("Processing event node for device %s.\n",
                event_node.device.c_str());

        // Create a fully instantiated event node
        auto instantiated_event =
            instantiateEvent(event_node, event_node.device);

        // Ensure message args are processed even if messageArgsJsonPattern is
        // null This is important for standalone mode where we might have
        // manually set message args like with --msg-arg2
        if (instantiated_event.messageRegistry.messageArgsJsonPattern
                .is_null() &&
            !instantiated_event.messageRegistry.messageArgs.empty())
        {
            log_dbg(
                "Using existing message args without pattern evaluation.\n");
            // The message args were already set manually, keep them
        }
        else if (!instantiated_event.messageRegistry.messageArgsJsonPattern
                      .is_null())
        {
            log_dbg("Evaluating message args from pattern.\n");
            // Normal processing with patterns was already done in
            // instantiateEvent
        }
        else
        {
            log_dbg("No message args or patterns found.\n");
        }

        event_instance =
            std::make_unique<event_info::EventNode>(instantiated_event);
        log_dbg("Successfully processed event: %s\n", event_node.event.c_str());
        return eventing::RcCode::succ;
    }
    catch (const std::exception& e)
    {
        log_err("Error processing event node: %s\n", e.what());
        event_instance.reset();
        return eventing::RcCode::error;
    }
}

event_info::EventNode& EventInstantiator::getInstantiatedEvent() const
{
    if (!event_instance)
    {
        log_err("Attempted to get instantiated event when none exists!\n");
        throw std::runtime_error("No instantiated event available");
    }

    log_dbg("Returning instantiated event: %s, device: %s.\n",
            event_instance->event.c_str(), event_instance->device.c_str());

    return *event_instance;
}

event_info::EventNode EventInstantiator::instantiateEvent(
    const event_info::EventNode& event_node, const std::string& device_id)
{
    log_dbg(
        "Instantiating event node for device %s, event: %s, error_id: %s.\n",
        device_id.c_str(), event_node.event.c_str(),
        event_node.errorId.c_str());

    // Create a copy of the original event node
    event_info::EventNode instantiated_node = event_node;

    // Get device type pattern
    std::string device_type = event_node.getStringifiedDeviceType();
    log_dbg("Using device type pattern: %s.\n", device_type.c_str());

    device_id::DeviceIdPattern device_pattern(device_type);

    // Find matching device index
    std::vector<device_id::PatternIndex> indices =
        device_pattern.match(device_id);

    if (indices.empty())
    {
        log_err("Device ID %s doesn't match the pattern %s in the event %s!\n",
                device_id.c_str(), device_type.c_str(),
                event_node.event.c_str());
        throw std::runtime_error(
            "Device ID doesn't match the pattern in the event");
    }

    log_dbg("Found matching device index for device %s in pattern %s.\n",
            device_id.c_str(), device_type.c_str());

    // Set specific device ID first (needed for evaluation)
    instantiated_node.device = device_id;

    // Handle origin of condition - extract the pattern before setting the
    // device index
    std::string ooc_pattern;
    if (instantiated_node.originOfCondition.has_value())
    {
        ooc_pattern = *instantiated_node.originOfCondition;
        log_dbg("Saved Origin of Condition pattern: %s.\n",
                ooc_pattern.c_str());
    }

    // Set the device index
    device_id::PatternIndex device_index = indices[0];
    instantiated_node.setDeviceIndexTuple(device_index);
    log_dbg("Set device index tuple for device %s.\n", device_id.c_str());

    // Handle origin of condition after device index is set
    if (!ooc_pattern.empty())
    {
        log_dbg("Processing Origin of Condition pattern: %s.\n",
                ooc_pattern.c_str());
        device_id::DeviceIdPattern ooc_id_pattern(ooc_pattern);

        // Evaluate OOC pattern with device index
        if (ooc_id_pattern.dim() > 0)
        {
            std::string instantiated_ooc = ooc_id_pattern.eval(device_index);
            log_dbg("Instantiated Origin of Condition: %s -> %s.\n",
                    ooc_pattern.c_str(), instantiated_ooc.c_str());

            instantiated_node.setOriginOfCondition(instantiated_ooc);
        }
        else
        {
            log_dbg(
                "Origin of Condition pattern has no dimensions, using as-is: %s.\n",
                ooc_pattern.c_str());
            // Keep the original OOC pattern
            instantiated_node.setOriginOfCondition(ooc_pattern);
        }
    }
    else
    {
        log_dbg(
            "Event does not have Origin of Condition or it's not available yet.\n");
    }

    // Handle message args with patterns
    if (!instantiated_node.messageRegistry.messageArgsJsonPattern.is_null())
    {
        log_dbg("Processing message args patterns.\n");
        try
        {
            json_proc::JsonPattern pattern(
                instantiated_node.messageRegistry.messageArgsJsonPattern);
            nlohmann::json evaluated = pattern.eval(device_index);
            instantiated_node.messageRegistry.messageArgsJsonPattern =
                evaluated;
            log_dbg("Successfully instantiated message args patterns.\n");
        }
        catch (const std::exception& e)
        {
            log_err("Error instantiating message args patterns: %s!\n",
                    e.what());
            throw;
        }
    }
    else
    {
        log_dbg("No message args patterns to process.\n");
    }

    log_dbg("Successfully created instantiated event for device %s.\n",
            device_id.c_str());
    return instantiated_node;
}

} // namespace event_handler
