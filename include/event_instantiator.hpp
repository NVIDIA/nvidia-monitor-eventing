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

#include "device_id.hpp"
#include "event_handler.hpp"
#include "event_info.hpp"

namespace event_handler
{

/**
 * @brief Instantiates events with specific device information
 *
 * The EventInstantiator class converts event entries with device patterns (like
 * "[n-m]") into specific event instances for a particular device.
 */
class EventInstantiator : public EventHandler
{
  public:
    /**
     * @brief Construct a new Event Instantiator
     *
     * @param name Name of the event handler
     */
    EventInstantiator(const std::string& name = "EventInstantiator");

    /**
     * @brief Destroy the Event Instantiator
     */
    virtual ~EventInstantiator() = default;

    /**
     * @brief Process an event node with patterns and create specific instance
     *
     * @param event_node Original event node with patterns
     * @return eventing::RcCode Return code indicating success or failure
     */
    virtual eventing::RcCode process(
        event_info::EventNode& event_node) override;

    /**
     * @brief Get the instantiated event
     *
     * @return event_info::EventNode& Reference to instantiated event
     */
    event_info::EventNode& getInstantiatedEvent() const;

  private:
    /**
     * @brief Create a specific event instance from a pattern-based event node
     *
     * @param event_node Event node with pattern
     * @param device_id Specific device ID
     * @return event_info::EventNode New event node with specific device info
     */
    event_info::EventNode instantiateEvent(
        const event_info::EventNode& event_node, const std::string& device_id);

    /**
     * @brief Instantiated event with specific device information
     */
    std::unique_ptr<event_info::EventNode> event_instance;
};

} // namespace event_handler
