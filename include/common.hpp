/**
 * Copyright (c) 2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#pragma once

#include <nlohmann/json.hpp>

#include "eventing_config.h"
#include "log.hpp"

#include <type_traits>

namespace eventing
{

namespace profile
{
extern nlohmann::json deviceAssociation;
}

enum class RcCode : int
{
    succ,
    error,  // no block, allow next EventHandler
    timeout,
    block,  // stop processing next EventHanlder
};

/**
 * @brief Turn enum class into integer
 *
 * @tparam E
 * @param e
 * @return std::underlying_type<E>::type
 */
template <typename E>
constexpr auto to_integer(E e) -> typename std::underlying_type<E>::type
{
    return static_cast<typename std::underlying_type<E>::type>(e);
}

} // namespace eventing
