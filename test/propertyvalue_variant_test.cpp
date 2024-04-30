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

TEST(PropertyValue, CopyOperator)
{
    PropertyValue string(std::string{"001"});
    auto integer = string;
    EXPECT_EQ(integer.isValidInteger(), true);
    EXPECT_EQ(integer.getInteger(), 1);
}

TEST(PropertyValue, Clear)
{
    PropertyValue string(std::string{"001"});
    EXPECT_EQ(string.getInteger(), 1);
    EXPECT_EQ(string.getString().empty(), false);
    EXPECT_EQ(string.empty(), false);
    string.clear();
    EXPECT_EQ(string.getInteger(), 0);
    EXPECT_NE(string.getString().empty(), false);
    EXPECT_NE(string.empty(), false);
}

TEST(PropertyValue, VariantInteger)
{
    int value = 5;
    PropertyVariant variant = value;
    PropertyValue prop{variant};
    EXPECT_EQ(prop.getInteger(), 5);
    EXPECT_EQ(prop.isValidInteger(), true);
}

TEST(PropertyValue, VariantHexa)
{
    int value = 0xff;
    PropertyVariant variant = value;
    PropertyValue prop{variant};
    EXPECT_EQ(prop.getInteger(), 255);
    EXPECT_EQ(prop.isValidInteger(), true);
}

TEST(PropertyValue, VariantBoolean)
{
    bool boolean = false;
    PropertyVariant variant = boolean;
    PropertyValue prop{variant};
    EXPECT_EQ(prop.getInteger(), 0);
    EXPECT_EQ(prop.isValidInteger(), true);
}

TEST(PropertyValue, VariantDouble)
{
    double value = 10.0;
    PropertyVariant variant = value;
    PropertyValue prop{variant};
    EXPECT_EQ(prop.getInteger(), 10);
    EXPECT_EQ(prop.isValidInteger(), true);
}

TEST(PropertyValue, VariantFloat)
{
    float value = 10.0;
    PropertyVariant variant = value;
    PropertyValue prop{variant};
    EXPECT_EQ(prop.getInteger(), 10);
    EXPECT_EQ(prop.isValidInteger(), true);
}

TEST(PropertyValue, Bitmask16Bits)
{
    int16_t value = 0xff; // bits 0-3 set
    PropertyVariant variant = value;
    PropertyValue prop{variant};
    EXPECT_EQ(prop.bitmask(0x07), true);
    EXPECT_EQ(prop.bitmask(0x03), true);
    EXPECT_EQ(prop.bitmask(0x01), true);
}

TEST(PropertyValue, Bitmask32Bits)
{
    int32_t value = 0x1001; // bit 0, 12
    PropertyVariant variant = value;
    PropertyValue prop{variant};
    EXPECT_EQ(prop.getInteger(), 0x1001);
    EXPECT_EQ(prop.bitmask(0x01), true);   // bit 0
    EXPECT_EQ(prop.bitmask(0x1000), true); // bit 12
    EXPECT_NE(prop.bitmask(0x02), true);   // bit 2 not set
}

TEST(PropertyValue, Bitmask64Bits)
{
    int32_t value = 0x100001; // bit 0, 20
    PropertyVariant variant = value;
    PropertyValue prop{variant};
    EXPECT_EQ(prop.getInteger(), 0x100001);
    EXPECT_EQ(prop.bitmask(0x01), true);     // bit 0
    EXPECT_EQ(prop.bitmask(0x100000), true); // bit 20
    EXPECT_EQ(prop.bitmask(0x02), false);    // bit 2 not set
}

TEST(PropertyValue, Bitmask2Properties)
{
    int32_t value = 0x100001; // bit 0, 20
    PropertyVariant variant = value;
    PropertyValue prop{variant};
    EXPECT_EQ(prop.getInteger(), 0x100001);

    PropertyVariant variantBit0 = 0x01;
    PropertyValue propBit0{variantBit0};
    EXPECT_EQ(prop.bitmask(propBit0), true);

    PropertyVariant variantBit20 = 0x100000;
    PropertyValue propBit20{variantBit20};
    EXPECT_EQ(prop.bitmask(propBit20), true);

    PropertyVariant variantBit2 = 0x2;
    PropertyValue propBit2{variantBit2};
    EXPECT_EQ(prop.bitmask(propBit2), false);
}

TEST(PropertyValue, CheckNegativeBitmaskEmptyPropertyValue)
{
    CheckDefinitionMap accessorCHECK = {{"bitmask", "0x01"}};
    PropertyValue propertyValueFail(std::string{""});
    EXPECT_NE(propertyValueFail.check(accessorCHECK), true);
}

TEST(PropertyValue, CheckPositiveBitmaskPropertyValue)
{
    CheckDefinitionMap accessorCHECK = {{"bitmask", "0x01"}};
    PropertyValue propertyValuePass(std::string{"0x03"});
    EXPECT_EQ(propertyValuePass.check(accessorCHECK), true);
}

TEST(PropertyValue, CheckNegativePropertyValue)
{
    const CheckDefinitionMap accessorCHECK = {{"bitmask", "0x01"}};
    PropertyValue propertyValueFail{std::string{"2"}};
    EXPECT_NE(propertyValueFail.check(accessorCHECK), true);
}

TEST(PropertyValue, CheckEqualString)
{
    const CheckDefinitionMap accessorCHECK = {{"equal", "0x01"}};
    PropertyValue propertyValueOK{std::string{"1"}};
    EXPECT_EQ(propertyValueOK.check(accessorCHECK), true);
}

TEST(PropertyValue, CheckEqualInteger)
{
    const CheckDefinitionMap accessorCHECK = {{"equal", "0x01"}};
    PropertyValue propertyValueInteger{1};
    EXPECT_EQ(propertyValueInteger.check(accessorCHECK), true);
}

TEST(PropertyValue, CheckNotEqual)
{
    const CheckDefinitionMap accessorCHECK = {{"not_equal", "0x01"}};
    PropertyValue propertyValueInteger{1};
    // they are equal, then not_equal returns false
    EXPECT_EQ(propertyValueInteger.check(accessorCHECK), false);

    PropertyValue propertyValueIntegerDiff{2};
    // they are different, then not_equal returns true
    EXPECT_EQ(propertyValueIntegerDiff.check(accessorCHECK), true);
}

TEST(PropertyValue, CheckPositiveLookupString)
{
    CheckDefinitionMap accessorCHECK = {{"lookup", "0x40"}};
    PropertyValue propertyValue{std::string{"0x30 0x40 0x50"}};
    EXPECT_EQ(propertyValue.check(accessorCHECK), true);
}

TEST(PropertyValue, CheckPositiveBitmaskPropertyValueRedefinition)
{
    CheckDefinitionMap accessorCHECK = {{"bitmask", "0x01"}};
    PropertyValue propertyValuePass(std::string{"0x02"});

    PropertyVariant maskRedefinition(int64_t(0x02));
    EXPECT_EQ(propertyValuePass.check(accessorCHECK, maskRedefinition), true);
}

TEST(PropertyValue, CheckNegativeBitmaskPropertyValueRedefinition)
{
    CheckDefinitionMap accessorCHECK = {{"bitmask", "0x01"}};
    PropertyValue propertyValueFail(std::string{"0x03"});

    PropertyVariant maskRedefinition(int64_t(0x04));
    EXPECT_NE(propertyValueFail.check(accessorCHECK, maskRedefinition), true);
}

TEST(PropertyValue, EqualOperator)
{
    EXPECT_EQ(PropertyValue(std::string("001")) == PropertyValue(1), true);
    EXPECT_EQ(PropertyValue(std::string("001")) == PropertyValue(0x01), true);
}

TEST(PropertyValue, Bitset)
{
    PropertyValue bits_4_5_12_13(0x3030);
    uint64_t bit_4 = 0x10;
    uint64_t bit_5 = 0x20;
    uint64_t bit_12 = 0x1000;
    uint64_t bit_13 = 0x2000;

    EXPECT_EQ(bits_4_5_12_13.bitset(bit_4), true);
    EXPECT_EQ(bits_4_5_12_13.bitset(bit_5), true);
    EXPECT_EQ(bits_4_5_12_13.bitset(bit_12), true);
    EXPECT_EQ(bits_4_5_12_13.bitset(bit_13), true);
    EXPECT_EQ(bits_4_5_12_13.bitset(PropertyValue(bit_4)), true);
    EXPECT_EQ(bits_4_5_12_13.bitset(PropertyValue(bit_5)), true);
    EXPECT_EQ(bits_4_5_12_13.bitset(PropertyValue(bit_12)), true);
    EXPECT_EQ(bits_4_5_12_13.bitset(PropertyValue(bit_13)), true);

    EXPECT_EQ(bits_4_5_12_13.bitset(uint64_t(0x00)), false);
    EXPECT_EQ(bits_4_5_12_13.bitset(uint64_t(0x01)), false);
    EXPECT_EQ(bits_4_5_12_13.bitset(uint64_t(0x02)), false);
    EXPECT_EQ(bits_4_5_12_13.bitset(uint64_t(0x03)), false);
    EXPECT_EQ(bits_4_5_12_13.bitset(uint64_t(0x05)), false);
    EXPECT_EQ(bits_4_5_12_13.bitset(uint64_t(0x06)), false);

    uint64_t zero = 0x00;
    PropertyValue zeroMask(zero);
    EXPECT_EQ(bits_4_5_12_13.bitset(zeroMask), false);
    PropertyValue zeroValue(zero);
    EXPECT_EQ(zeroValue.bitset(bits_4_5_12_13), false);
}

TEST(PropertyValue, NotBitset)
{
    PropertyValue bits_4_5_12_13(0x3030);
    uint64_t bit_4 = 0x10;
    uint64_t bit_5 = 0x20;
    uint64_t bit_12 = 0x1000;
    uint64_t bit_13 = 0x2000;

    EXPECT_NE(bits_4_5_12_13.notBitset(bit_4), true);
    EXPECT_NE(bits_4_5_12_13.notBitset(bit_5), true);
    EXPECT_NE(bits_4_5_12_13.notBitset(bit_12), true);
    EXPECT_NE(bits_4_5_12_13.notBitset(bit_13), true);
    EXPECT_NE(bits_4_5_12_13.notBitset(PropertyValue(bit_4)), true);
    EXPECT_NE(bits_4_5_12_13.notBitset(PropertyValue(bit_5)), true);
    EXPECT_NE(bits_4_5_12_13.notBitset(PropertyValue(bit_12)), true);
    EXPECT_NE(bits_4_5_12_13.notBitset(PropertyValue(bit_13)), true);

    EXPECT_EQ(bits_4_5_12_13.notBitset(uint64_t(0x00)), true);
    EXPECT_EQ(bits_4_5_12_13.notBitset(uint64_t(0x01)), true);
    EXPECT_EQ(bits_4_5_12_13.notBitset(uint64_t(0x02)), true);
    EXPECT_EQ(bits_4_5_12_13.notBitset(uint64_t(0x03)), true);
    EXPECT_EQ(bits_4_5_12_13.notBitset(uint64_t(0x05)), true);
    EXPECT_EQ(bits_4_5_12_13.notBitset(uint64_t(0x06)), true);

    uint64_t zero = 0x00;
    PropertyValue zeroMask(zero);
    EXPECT_EQ(bits_4_5_12_13.notBitset(zeroMask), true);
    PropertyValue zeroValue(zero);
    EXPECT_EQ(zeroValue.notBitset(bits_4_5_12_13), true);
}

TEST(PropertyValue, NotBitmask)
{
    PropertyValue bits_0_1_2(0x07);
    uint64_t bit_0 = 0x01;
    uint64_t bit_1 = 0x02;
    uint64_t bit_2 = 0x04;
    uint64_t bit_3 = 0x08;
    uint64_t bit_4 = 0x10;
    uint64_t mask = 0x07;

    EXPECT_EQ(bits_0_1_2.bitmask(bits_0_1_2), true); // itself
    EXPECT_EQ(bits_0_1_2.bitmask(mask), true); // same value

    EXPECT_NE(bits_0_1_2.notBitmask(bits_0_1_2), true); // itself
    EXPECT_NE(bits_0_1_2.notBitmask(mask), true); // same value

    EXPECT_NE(bits_0_1_2.notBitmask(bit_0), true);
    EXPECT_NE(bits_0_1_2.notBitmask(bit_1), true);
    EXPECT_NE(bits_0_1_2.notBitmask(bit_2), true);
    EXPECT_EQ(bits_0_1_2.notBitmask(bit_3), true);
    EXPECT_EQ(bits_0_1_2.notBitmask(bit_4), true);
}
