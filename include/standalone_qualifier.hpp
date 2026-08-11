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

#pragma once

#include "event_info.hpp"

namespace eventing
{

/**
 * @brief Evaluate the event's accessor as a qualifier in standalone mode.
 *
 * Standalone mode (monitor-eventingd -S) is invoked by external triggers
 * (e.g. phosphor-multi-gpio-monitor starting a *-alert.service when a fault
 * GPIO transitions). The trigger itself proves the fault was observed; the
 * `accessor` field in event_info.json is consulted here as a *qualifier* to
 * decide whether the report should be propagated -- for example, "only report
 * GPU IROT-fatal if the GPU module's power-good signal is asserted." This
 * suppresses spurious reports caused by transient GPIO transitions during
 * early-boot or power-cycle windows.
 *
 * Fail-open behavior: if the event has no accessor, or its accessor has no
 * `check` criteria, this function returns true so the event still emits.
 *
 * @param event  Fully-instantiated event node (device + deviceIndexTuple set).
 * @return true if the qualifier passes (or is absent), false to suppress.
 */
bool standaloneQualifierPassed(event_info::EventNode& event);

} // namespace eventing
