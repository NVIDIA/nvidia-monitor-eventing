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

#include "property_variant.hpp"

#include <boost/algorithm/string/join.hpp>

#include <map>
#include <string>

namespace data_accessor
{

using CheckDefinitionMap = std::map<std::string, std::string>;

constexpr auto bitmaskKey = "bitmask";
constexpr auto notBitmaskKey = "not_bitmask";
constexpr auto bitmapKey = "bitmap";
constexpr auto bitsetKey = "bitset";
constexpr auto notBitsetKey = "not_bitset";
constexpr auto lookupKey = "lookup";
constexpr auto equalKey = "equal";
constexpr auto notEqualKey = "not_equal";

/**
 * @brief PropertyValueData represents any Data with 64 bit integer
 *
 *     Fields:
 *           strValue      string representation
 *           value64       64 bits value intended for bit operations
 *           state         State of the current Data @sa DataState
 */
struct PropertyValueData
{
    enum DataState
    {
        /** No data (nothing), nor an empty string which is data */
        Empty,
        /** The Data contains a valid Integer value even 0 */
        Integer,
        /** The data is string, the conversion to Integer was not possible or
         *   it is not desired */
        StringOnly
    };
    std::string strValue;
    uint64_t value64;
    DataState state;
    PropertyValueData() : strValue{""}, value64(0), state(Empty)
    {
        // Empty
    }
    explicit PropertyValueData(const std::string& str) :
        strValue(str), value64(0), state(StringOnly)
    {
        // Empty
    }
};

/**
 * @brief  Helper class to get values from std::variant and store that in
 *         a PropertyValueData structure
 */
template <class T>
class PropertyValueDataHelper
{
  private:
    PropertyValueDataHelper();

  public:
    /**
     * @brief converts integer 8/16 bits and double values to integer 64 bits
     *
     * @param varVar the integer as std::variant
     *
     * @param pointer of PropertyValueData to store results
     *
     * @return true if integer/double could be extracted from varVar
     */
    static bool setInteger(const PropertyVariant& varVar,
                           PropertyValueData* data)
    {
        bool isInteger = std::is_same<T, uint8_t>::value ||
                         std::is_same<T, int16_t>::value ||
                         std::is_same<T, uint16_t>::value ||
                         std::is_same<T, int32_t>::value ||
                         std::is_same<T, uint32_t>::value ||
                         std::is_same<T, int64_t>::value ||
                         std::is_same<T, uint64_t>::value ||
                         std::is_same<T, double>::value;
        if (isInteger && std::holds_alternative<T>(varVar) == true)
        {
            auto value = std::get<T>(varVar);
            data->value64 = static_cast<uint64_t>(value);
            data->strValue = std::to_string(value);
            data->state = PropertyValueData::Integer;
            return true;
        }
        return false;
    }
    /**
     * @brief setBoolean converts bool values into integer 64 bits
     *                   and sets the string representation of the data
     *
     * @param varVar the boolean value as std::variant
     *
     * @param pointer of PropertyValueData to store results
     *
     * @return true if boolean data could be extracted from varVar
     */
    static bool setBoolean(const PropertyVariant& varVar,
                           PropertyValueData* data)
    {
        if (std::holds_alternative<bool>(varVar) == true)
        {
            auto booleanData = std::get<bool>(varVar);
            data->state = PropertyValueData::Integer;
            if (booleanData == true)
            {
                data->strValue = "true";
                data->value64 = 1;
            }
            else
            {
                data->strValue = "false";
                data->value64 = 0;
            }
            return true;
        }
        return false;
    }
    /**
     * @brief  gets the string from varVar and puts it in a PropertyValueData
     *
     *
     * @param varVar the string as std::variant
     *
     * @param pointer of PropertyValueData to store results
     *
     * @return true if string data could be extracted from varVar
     */
    static bool setString(const PropertyVariant& varVar,
                          PropertyValueData* data)
    {
        if (std::holds_alternative<std::string>(varVar) == true)
        {
            data->strValue = std::get<std::string>(varVar);
            data->value64 = 0;
            data->state = PropertyValueData::StringOnly;
            return true;
        }
        return false;
    }

    /**
     * @brief  gets the list of strings from varVar and puts it in a
     *           PropertyValueData
     *
     * @param varVar the std::vector<string> as std::variant
     *
     * @param pointer of PropertyValueData to store results
     *
     * @return true if string data could be extracted from varVar
     */
    static bool setVectorStrings(const PropertyVariant& varVar,
                                 PropertyValueData* data)
    {
         if (std::holds_alternative<std::vector<std::string>>(varVar) == true)
         {
             std::vector<std::string> list =
                        std::get<std::vector<std::string>>(varVar);
             data->strValue = boost::join(list, " ");
             data->value64 = 0;
             data->state = PropertyValueData::StringOnly;
             return true;
         }
         return false;
    }
};

/**
 * @brief The PropertyValue intends to perform operations in between to values
 *
 *   Converts supported types to integer 64 bits and does operations such as
 *      and bit a bit among others
 *
 *   Explanation:
 *
 *   PropertyValue  itself stores an integer or a string in the struct '_data'
 *   Considering integer values, the integer is '_data.value64' then
 *     the following boolean operations are available:
 *
 *    Key           Method in PropertyValue class
 *  ----------------------------------------------------------------------------
 *  'bitmask'     bool bitmask(const uint64_t mask)     (value64 & mask) == mask
 *  'not_bitmask' bool notBitmask(const uint64_t mask)  (value64 & mask) != mask
 *  'bitset'      bool bitset(const uint64_t mask)      (value64 & mask) != 0
 *  'not_bitset'  bool notBitset(const uint64_t mask)   (value64 & mask) == 0
 *
 *  Wrappers to perform bit operations between 2 PropertyValue objects:
 *
 *     bool bitmask(const PropertyValue& other)
 *     bool notBitmask(const PropertyValue& other)
 *     bool bitset(const PropertyValue& other)
 *     bool notBitset(const PropertyValue& other)
 *
 *  @note
 *     'bitmap' key in the class PropertyValue behaves as 'bitmask' key,
 *     the shifting is managed by the class @sa CheckAccessor
 *     passing it as different criteria for @sa PropertyValue::check(),
 *     @sa PropertyValue::check() and @sa criteria::getValueFromCriteria()
 *
 *
 */
class PropertyValue
{
  public:
    /**
     * @brief The CaseSensitivity enum
     *
     * Defines the type of string search used in lookup() methods
     */
    enum CaseSensitivity
    {
        /**
         * "AA" != "aa"
         */
        caseSensitive,

        /**
         * "AA" == "aa"
         */
        caseInsensitive
    };
    PropertyValue();
    explicit PropertyValue(const PropertyVariant& value);
    explicit PropertyValue(const std::string& value);
    explicit PropertyValue(const std::string& valueStr, uint64_t value64);
    virtual ~PropertyValue();

    /**
     * @brief operator ==  Compares two PropertyValue Data
     *
     *      This comparing returns true:
     * @code
     *         if (PropertyValue(std::string("001")) == PropertyValue(0x01))
     *         {
     *             std::cout << "They are equal";
     *         }
     * @endcode
     *
     * @param other other PropertyValue object to compare
     *
     * @return  returns true if both data have equal values even strings differ
    */
    inline bool operator==(const PropertyValue& other) const
    {
        if (this->isValidInteger() == true && other.isValidInteger() == true)
        {
            return this->getInteger() == other.getInteger();
        }
        return this->getString() == other.getString();
    }

    /**
     * @brief check() does high level operation between 2 PropertyValue objects
     *
     *
     * @param map a std::map of two strings: 'key' and its 'value'
     *
     *             key:   can be: "bitmask", "lookup" or other operation
     *             value: the value to perform the operation against this
     *
     * @return  the check result between this and the other PropertyValue object
     */
    bool check(const CheckDefinitionMap& map,
               const PropertyVariant& redefCriteria = PropertyVariant()) const;

    /**
     * @brief checks if a bitmask is present in @a _data.value64
     * @param mask  the bitmask (bits set) to check
     * @return true if all bits in mask are also in @a _data.value64
     */
    bool bitmask(const uint64_t mask) const;

    /** [redefinition] performs bitmask operation between two  objects */
    bool bitmask(const PropertyValue& otherMask) const;

    /**
     * @brief checks if a bitmask is @bold not present in @a _data.value64
     * @param mask  the bitmask (bits set) to check
     * @return true if all bits in mask are not in @a _data.value64
     */
    bool notBitmask(const uint64_t mask) const;

    /** [redefinition] performs notBitmask operation between two  objects */
    bool notBitmask(const PropertyValue& other) const;

    /**
     * @brief checks if any bit from a bitmask is also in @a _data.value64
     * @param mask  the bitmask (bits set) to check
     * @return true if at least one bit in mask is also in @a _data.value64
     */
    bool bitset(const uint64_t mask) const;

    /** [redefinition] performs bitset operation between two  objects */
    bool bitset(const PropertyValue& other) const;

    /**
     * @brief checks if any bit from a bitmask is @bold not in @a _data.value64
     * @param mask  the bitmask (bits set) to check
     * @return true if no single bit in mask is also in @a _data.value64
     */
    bool notBitset(const uint64_t mask) const;

    /** [redefinition] performs notBitset operation between two  objects */
    bool notBitset(const PropertyValue& other) const;

    /**
     * @brief searches lookupString in the string representation of the _data
     *
     * @param lookupString string to search in _data
     *
     * @param cs  peforms the search case sensitive or insensitive
     *
     * @return true if lookupString was found in the string from _data
     */
    bool lookup(const std::string& lookupString,
                CaseSensitivity cs = caseSensitive) const;

    /**
     * @brief searches the string otherLookup._data.strValue in the
     *           string representation of the _data
     *
     * @param otherLookup contains the string representation to search
     *
     * @param cs  performs the search case sensitive or insensitive
     *
     * @return  true if otherLookup._data.strValue was found in the string
     *          from _data
     */
    bool lookup(const PropertyValue& otherLookup,
                CaseSensitivity cs = caseSensitive) const;

    /**
     * @brief getString
     * @return the string representation of the stored data
     */
    inline std::string getString() const
    {
        return _data.strValue;
    }

    /**
     * @brief getInteger
     * @return the 64 bits integer representation of the stored data
     */
    inline uint64_t getInteger() const
    {
        return _data.value64;
    }

    /**
     * @brief isValidInteger
     * @return true if the 64 bits integer conversion was OK even when zero
     */
    inline bool isValidInteger() const
    {
        return _data.state == PropertyValueData::Integer;
    }

    /**
     * @brief clear, just clears the data @sa empty()
     */
    inline void clear()
    {
        _data.strValue.clear();
        _data.value64 = 0;
        _data.state = PropertyValueData::Empty;
    }

    /**
     * @brief empty
     * @return  true if there is not stored data
     */
    inline bool empty() const
    {
        return _data.state == PropertyValueData::Empty;
    }

  protected:
    /**
     * @brief getPropertyDataFromVariant
     *
     *  Having a valid value stored in std::variant
     *    If it is any integer/float/double, this value is just casted into
     *       64 bits integer and sets valid=true.
     *    When this value is a string, calls string2Uint64()
     *     to check if it is a string representation of an integer,
     *     if so, converts the string into 64 bits integer and sets valid=true
     *     otherwise sets valid=false
     *
     * @param varVar
     */
    void getPropertyDataFromVariant(const PropertyVariant& varVar);

    /**
     * @brief  checks if 'value' is a representation of an integer
     *
     *         Sets all fiels from this->_data
     *
     * @param value  string to check
     *
     *
     */
    void string2Uint64(const std::string& value);

  protected:
    /**
     * @brief _data keeps both string and 64 bits integer representation of
     *              any data entered by PropertyVariant or just a string.
     */
    PropertyValueData _data;
};

/**
 * @brief The PropertyString does not perform any integer conversion
 */
class PropertyString : public PropertyValue
{
  public:
    explicit PropertyString(const std::string& value);
    explicit PropertyString(const PropertyVariant& varVar);
    PropertyString() = delete;
    ~PropertyString() = default;
};

namespace criteria
{
/**
 * @brief helper function to get redefined value if specified
 * @param redefCriteria
 * @param accessorValue
 * @return value from redefCriteria if that exists, otherwise accessorValue
 */
PropertyString getStringFromCriteria(const PropertyVariant& redefCriteria,
                                     const std::string& accessorValue);
/**
 * @brief helper function to get redefined value if specified
 * @param redefCriteria
 * @param accessorValue
 * @return value from redefCriteria if that exists, otherwise accessorValue
 */
PropertyValue getValueFromCriteria(const PropertyVariant& redefCriteria,
                                   const std::string& accessorValue);
} // namespace criteria

} // namespace data_accessor

