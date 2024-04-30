/**
 * Copyright (c) 2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#include "property_accessor.hpp"

#include <string>

#include <gtest/gtest.h>

using namespace data_accessor;

TEST(PropertyValue, LongLongIs64Bits)
{
    EXPECT_EQ(sizeof(long long), 8);
}

TEST(PropertyValue, StringHexaLowerCase)
{
    const std::string hexaVALUE{"0x01"};
    PropertyValue prop{hexaVALUE};
    EXPECT_EQ(prop.getInteger(), 1);
    EXPECT_EQ(prop.isValidInteger(), true);
}

TEST(PropertyValue, StringHexaUpperCase)
{
    const std::string hexa_upper{"0X10"};
    PropertyValue prop{hexa_upper};
    EXPECT_EQ(prop.getInteger(), 0x10);
    EXPECT_EQ(prop.isValidInteger(), true);
}

TEST(PropertyValue, StringDouble)
{
    const std::string strValue{"10.0"};
    PropertyValue prop{strValue};
    EXPECT_EQ(prop.getInteger(), 10);
    EXPECT_EQ(prop.isValidInteger(), true);
}

TEST(PropertyValue, StringInteger)
{
    const std::string strValue{"10"};
    PropertyValue prop{strValue};
    EXPECT_EQ(prop.getInteger(), 10);
    EXPECT_EQ(prop.isValidInteger(), true);
}

TEST(PropertyValue, StringIntegerSignal)
{
    const std::string strValue{"-10"};
    PropertyValue prop{strValue};
    EXPECT_EQ(prop.getInteger(), -10);
    EXPECT_EQ(prop.isValidInteger(), true);
}

TEST(PropertyValue, StringDoubleWithDecimals)
{
    const std::string strValue{"10.1"};
    PropertyValue prop{strValue};
    EXPECT_EQ(prop.getInteger(), 10);
    EXPECT_EQ(prop.isValidInteger(), true);
}

TEST(PropertyValue, StringHexaLowerCaseWithoutZeroX)
{
    const std::string strValue{"fff"};
    PropertyValue prop{strValue};
    EXPECT_EQ(prop.isValidInteger(), false);
    EXPECT_EQ(prop.getInteger(), 0);
}

TEST(PropertyValue, StringHexaUpperCaseWithoutZeroX)
{
    const std::string strValue{"EFF"};
    PropertyValue prop{strValue};
    EXPECT_EQ(prop.isValidInteger(), false);
    EXPECT_EQ(prop.getInteger(), 0);
}

TEST(PropertyValue, StringBooleanTrue)
{
    const std::string strValue{"TRUE"};
    PropertyValue prop{strValue};
    EXPECT_EQ(prop.isValidInteger(), true);
    EXPECT_EQ(prop.getInteger(), 1);
}

TEST(PropertyValue, StringBooleanFalse)
{
    const std::string strValue{"false"};
    PropertyValue prop{strValue};
    EXPECT_EQ(prop.isValidInteger(), true);
    EXPECT_EQ(prop.getInteger(), 0);
}

TEST(PropertyValue, LookupPositiveStringIgnoreCase)
{
    const std::string strValue{"ff 00 00 00 00 00 02 40 66 28"};
    PropertyValue prop{strValue};
    const std::string accessorLOOKUP{"FF 00 00"};
    EXPECT_EQ(prop.lookup(accessorLOOKUP, PropertyValue::caseInsensitive),
              true);
}

TEST(PropertyValue, LookupNegativeString)
{
    const std::string strValue{"ff 00 00 00 00 00 02 40 66 28"};
    PropertyValue prop{strValue};

    const std::string accessorLOOKUP{"FF aa 00"};
    EXPECT_NE(prop.lookup(accessorLOOKUP), true);
}
