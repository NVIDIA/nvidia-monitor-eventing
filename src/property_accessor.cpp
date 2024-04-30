/*
 Copyright (c) 2024, NVIDIA CORPORATION.  All rights reserved.

 NVIDIA CORPORATION and its licensors retain all intellectual property
 and proprietary rights in and to this software, related documentation
 and any modifications thereto.  Any use, reproduction, disclosure or
 distribution of this software and related documentation without an express
 license agreement from NVIDIA CORPORATION is strictly prohibited.
*
*/

#include "property_accessor.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <regex>
#include <string>

/*
 * defines used in std::stdoull
 */
#define HEXA_BASE 16
#define DECIMAL_BASE 10

namespace data_accessor
{

PropertyString::PropertyString(const std::string& value) : PropertyValue()
{
    _data.strValue = value;
    _data.state = PropertyValueData::StringOnly;
}

PropertyString::PropertyString(const PropertyVariant& varVar) : PropertyValue()
{
    if (std::holds_alternative<std::string>(varVar) == true)
    {
        _data.strValue = std::get<std::string>(varVar);
        _data.state = PropertyValueData::StringOnly;
    }
    else
    {
        getPropertyDataFromVariant(varVar);
        // make sure it will not be used as Integer
        _data.state = PropertyValueData::StringOnly;
    }
}

PropertyValue::PropertyValue(const std::string& value)
{
    // string2Uint64 will set all fields from _data sructure
    string2Uint64(value);
}

PropertyValue::PropertyValue(const std::string& valueStr, uint64_t value64)
{
    _data.strValue = valueStr;
    _data.value64 = value64;
    _data.state = PropertyValueData::Integer;
}

PropertyValue::~PropertyValue()
{
    // Empty
}

PropertyValue::PropertyValue()
{
    // Empty
}

PropertyValue::PropertyValue(const PropertyVariant& value)
{
    getPropertyDataFromVariant(value);
}

bool PropertyValue::check(const CheckDefinitionMap& map,
                          const PropertyVariant& redefCriteria) const
{
    bool ret = false;
    for (auto& accessorCheck : map)
    {
        auto& key = accessorCheck.first;
        if (key == bitmaskKey || key == bitmapKey)
        {
            ret = bitmask(criteria::getValueFromCriteria(redefCriteria,
                                                         accessorCheck.second));
        }
        else if (key == bitsetKey)
        {
            ret = bitset(criteria::getValueFromCriteria(redefCriteria,
                                                         accessorCheck.second));
        }
        else if (key == notBitmaskKey)
        {
            ret = notBitmask(criteria::getValueFromCriteria(redefCriteria,
                                                        accessorCheck.second));
        }
        else if (key == notBitsetKey)
        {
            ret = notBitset(criteria::getValueFromCriteria(redefCriteria,
                                                            accessorCheck.second));
        }
        else if (key == lookupKey)
        {
            ret = lookup(criteria::getStringFromCriteria(redefCriteria,
                                                         accessorCheck.second));
        }
        else if (key == equalKey)
        {
            ret = (*this == criteria::getValueFromCriteria(
                                          redefCriteria, accessorCheck.second));
        }
        else if (key == notEqualKey)
        {
            auto equal = (*this == criteria::getValueFromCriteria(
                                          redefCriteria, accessorCheck.second));
            ret = (equal == false);
        }
        if (ret == false)
        {
            // perhaps there will be more pairs in the future
            break;
        }
    }
    return ret;
}

bool PropertyValue::notBitmask(const uint64_t mask) const
{
    return (bitmask(mask) == false);
}

bool PropertyValue::notBitmask(const PropertyValue& other) const
{
    return other.isValidInteger() && notBitmask(other._data.value64);
}

bool PropertyValue::bitset(const uint64_t mask) const
{
    bool ret = isValidInteger();
    if (ret == true)
    {
        uint64_t result = _data.value64 & mask;
        ret = (result != 0);
    }
    return ret;
}

bool PropertyValue::bitset(const PropertyValue& other) const
{
    return other.isValidInteger() && bitset(other._data.value64);
}

bool PropertyValue::notBitset(const uint64_t mask) const
{
    return (bitset(mask) == false);
}

bool PropertyValue::notBitset(const PropertyValue& other) const
{
    return other.isValidInteger() && notBitset(other._data.value64);
}

bool PropertyValue::bitmask(const uint64_t mask) const
{
    bool ret = isValidInteger();
    if (ret == true)
    {
        uint64_t result = _data.value64 & mask;
        ret = (result == mask);
    }
    return ret;
}

bool PropertyValue::bitmask(const PropertyValue& other) const
{
    return other.isValidInteger() && bitmask(other._data.value64);
}

bool PropertyValue::lookup(const std::string& lookupString,
                           CaseSensitivity cs) const
{
    bool ret = false;
    if (cs == caseInsensitive)
    {
        std::string lookUpp = lookupString;
        std::transform(lookUpp.begin(), lookUpp.end(), lookUpp.begin(),
                       ::toupper);
        std::string selfUpp = _data.strValue;
        std::transform(selfUpp.begin(), selfUpp.end(), selfUpp.begin(),
                       ::toupper);
        ret = selfUpp.find(lookUpp) != std::string::npos;
    }
    else
    {
        ret = _data.strValue.find(lookupString) != std::string::npos;
    }
    return ret;
}

bool PropertyValue::lookup(const PropertyValue& otherLookup,
                           CaseSensitivity cs) const
{
    return lookup(otherLookup._data.strValue, cs);
}

void PropertyValue::getPropertyDataFromVariant(const PropertyVariant& varVar)
{
    bool done = false;
    if (isValidVariant(varVar) == true)
    {
        done =
            PropertyValueDataHelper<std::string>::setString(varVar, &_data) ||
            PropertyValueDataHelper<uint8_t>::setInteger(varVar, &_data) ||
            PropertyValueDataHelper<int16_t>::setInteger(varVar, &_data) ||
            PropertyValueDataHelper<uint16_t>::setInteger(varVar, &_data) ||
            PropertyValueDataHelper<int32_t>::setInteger(varVar, &_data) ||
            PropertyValueDataHelper<uint32_t>::setInteger(varVar, &_data) ||
            PropertyValueDataHelper<int64_t>::setInteger(varVar, &_data) ||
            PropertyValueDataHelper<uint64_t>::setInteger(varVar, &_data) ||
            PropertyValueDataHelper<double>::setInteger(varVar, &_data) ||
            PropertyValueDataHelper<bool>::setBoolean(varVar, &_data) ||
            PropertyValueDataHelper<std::vector<std::string>>::setVectorStrings(
                                                             varVar, &_data);
    }
    if (done == false) // varVar.index() == 0 or type not handled above
    {
        _data = PropertyValueData(); // clear
    }
}

void PropertyValue::string2Uint64(const std::string& value)
{
    _data.strValue = value;
    _data.state = PropertyValueData::StringOnly;
    _data.value64 = 0;
    if (value.empty() == true || value.find_first_of(' ') != std::string::npos)
    {
        return;
    }
    // first check for strings "true" and "false"
    bool strTrue = (value == "true" || value == "True" || value == "TRUE");
    bool strFalse = (value == "false" || value == "False" || value == "FALSE");
    if (strTrue == true || strFalse == true)
    {
        _data.value64 = strTrue == true ? 1 : 0; // 1 if value is one of Trues
        _data.state = PropertyValueData::Integer;
    }
    else // any decimal or hexa value
    {
        uint64_t data = 0;
        if (value.at(0) == '0' && value.size() > 2 &&
            (value.at(1) == 'x' || value.at(1) == 'X'))
        {
            // Hexa first
            try
            {
                data = std::stoll(value, nullptr, HEXA_BASE);
                _data.state = PropertyValueData::Integer;
                _data.value64 = data;
            }
            catch (...)
            {
                _data.state = PropertyValueData::StringOnly;
            }
        }
        else
        {
            try
            {
                data = std::stoll(value, nullptr, DECIMAL_BASE);
                _data.state = PropertyValueData::Integer;
                _data.value64 = data;
            }
            catch (...)
            {
                _data.state = PropertyValueData::StringOnly;
            }
        }
    }
}

namespace criteria
{

PropertyString getStringFromCriteria(const PropertyVariant& redefCriteria,
                                     const std::string& accessorValue)
{
    PropertyString value = isValidVariant(redefCriteria) == true
                               ? PropertyString(redefCriteria)
                               : PropertyString(accessorValue);

    return value;
}

PropertyValue getValueFromCriteria(const PropertyVariant& redefCriteria,
                                   const std::string& accessorValue)
{
    PropertyValue value = isValidVariant(redefCriteria) == true
                              ? PropertyValue(redefCriteria)
                              : PropertyValue(accessorValue);

    return value;
}

} // namespace criteria

} // namespace data_accessor

