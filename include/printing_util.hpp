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

#include <map>
#include <sstream>
#include <tuple>
#include <vector>

/**
 * @brief Tools for printing commonly used objects
 *
 * - std::vector
 * - std::map
 * - std::tuple
 *
 * The 'operator<<' will only work if it's also overloaded for the types of
 * containing elements.
 *
 * All operators print object in a single line, without newline at the end
 * (assuming the 'operator<<' for element type doesn't print any newline
 * either).
 *
 * Operators defined in global scope, because they mostly operate
 * on objects from 'std' namespace which should not be
 * extended. If the compiler complains about not being able to
 * find the operator use
 *
 * @code
 * {
 *     using ::operator<<;
 *     std::cout << someVector;
 *     // ...
 * }
 * @endcode
 *
 */

template <typename A, typename B>
std::ostream& operator<<(std::ostream& os, const std::tuple<A, B>& tup)
{
    os << "(" << std::get<0>(tup) << ", " << std::get<1>(tup) << ")";
    return os;
}

template <typename A, typename B, typename C>
std::ostream& operator<<(std::ostream& os, const std::tuple<A, B, C>& tup)
{
    os << "(" << std::get<0>(tup) << ", " << std::get<1>(tup) << ", "
       << std::get<2>(tup) << ")";
    return os;
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& vec)
{
    os << "vector{";
    if (vec.size() > 0)
    {
        os << " ";
        for (auto i = 0u; i < vec.size(); ++i)
        {
            if (i > 0u)
            {
                os << ", ";
            }
            os << "[" << i << "]: " << vec.at(i);
        }
        os << " ";
    }
    else // ! vec.size() > 0
    {
        os << " ";
    }
    os << "}";
    return os;
}

template <typename K, typename V>
std::ostream& operator<<(std::ostream& os, const std::map<K, V>& m)
{
    os << "map{";
    if (m.size() > 0)
    {
        os << " ";
        bool firstElemPrinted = false;
        for (const auto& [key, value] : m)
        {
            if (firstElemPrinted)
            {
                os << ", ";
            }
            os << "[" << key << "]: " << value;
            firstElemPrinted = true;
        }
        os << " ";
    }
    else
    {
        os << " ";
    }
    os << "}";
    return os;
}

template <typename T>
std::string to_string(const T& value)
{
    std::ostringstream ss;
    ss << value;
    return ss.str();
}
