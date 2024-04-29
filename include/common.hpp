/**
 *
 */

#pragma once

#include "log.hpp"

#include <type_traits>

namespace eventing
{

enum class RcCode : int
{
    succ,
    error,
    timeout,
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
