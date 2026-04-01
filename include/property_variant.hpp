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

#include <cstdint>
#include <map>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

/**
 * It is a common type used in openbmc DBus services
 */
using Association = std::tuple<std::string, std::string, std::string>;

/**
 *     This strange type just replaces std::monostate which should be a class
 *     and due that it is not supported by sdbusplus functions that requires
 *     std::variant as parameter
 *
 *     It is not expected to use a variable of this type
 **/
using InvalidMonoState =
    std::map<uint16_t, std::map<uint16_t, std::map<uint16_t, uint16_t>>>;

/**
 *  Variant type used for Dbus blocking 'get' and 'set' properties
 */
using PropertyVariant =
    std::variant<InvalidMonoState,
                 /*01*/ bool,
                 /*02*/ uint8_t,
                 /*03*/ int16_t,
                 /*04*/ uint16_t,
                 /*05*/ int32_t,
                 /*06*/ uint32_t,
                 /*07*/ int64_t,
                 /*08*/ uint64_t,
                 /*09*/ double,
                 /*10*/ std::string,
                 /*11*/ std::vector<std::string>,
                 /*12*/ std::vector<Association>,
                 /*13*/ std::vector<uint8_t>,
                 /*14*/ std::vector<int16_t>,
                 /*15*/ std::vector<uint16_t>,
                 /*16*/ std::vector<int32_t>,
                 /*17*/ std::vector<uint32_t>,
                 /*18*/ std::vector<int64_t>,
                 /*19*/ std::vector<uint64_t>,
                 /*20*/ std::vector<double>>;

/**
 * @brief returns true if the PropertyVariant has a valid value
 * @param variant
 * @return
 */
inline bool isValidVariant(const PropertyVariant& variant)
{
    return variant.index() != 0;
}

/**
 * @brief returns true if the PropertyVariant has an invalid value
 * @param variant
 * @return
 */
inline bool isInvalidVariant(const PropertyVariant& variant)
{
    return variant.index() == 0;
}
