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

#include "standalone_qualifier.hpp"

#include "check_accessor.hpp"
#include "log.hpp"

namespace eventing
{

bool standaloneQualifierPassed(event_info::EventNode& event)
{
    // Fail-open: no accessor, or no check criteria => no qualifier to apply.
    if (event.accessor.isEmpty() || !event.accessor.existsCheckKey())
    {
        return true;
    }

    // Scope the CheckAccessor to the specific device the caller targets
    // (e.g. "GPU_0") so range expansion in the accessor's arguments resolves
    // to that single device rather than looping the whole device_type range.
    //
    // The device_type pattern plus the instantiated index is used rather than
    // the concrete device name: expanding a range in the accessor requires an
    // index of matching dimension, and a bare device name carries none. With
    // an empty index, DataAccessor::read() leaves "GPU_SMA_[0-1] 128 reason"
    // unsubstituted and hands the literal to the command.
    data_accessor::CheckAccessor checkObj(event.getDataDeviceType());
    checkObj.check(event.accessor, event.accessor);
    return checkObj.passed();
}

} // namespace eventing
