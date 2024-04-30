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

#include <string>
namespace object
{

/**
 * @brief Base class for all
 *
 */
class Object
{
  public:
    Object(const std::string& name = "") : _name(name)
    {}
    virtual ~Object()
    {}

  public:
    std::string getName(void)
    {
        return _name;
    }

    void setName(const std::string& name)
    {
        _name = name;
    }

  private:
    /**
     * @brief Object name.
     *
     */
    std::string _name;
};

} // namespace object
