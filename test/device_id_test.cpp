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

#include "device_id.hpp"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/token_iterator.hpp>
#include <boost/tokenizer.hpp>

#include <array>
#include <charconv>
#include <iostream>
#include <list>
#include <ranges>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;
using namespace std::literals;

namespace device_id
{

// PatternIndex ///////////////////////////////////////////////////////////////

TEST(DeviceIdTest, PatternIndex_Construction_outoperator)
{
    std::cout << PatternIndex() << std::endl;  // expected output: '()'
    std::cout << PatternIndex(7) << std::endl; // expected output: '(7)'
    std::cout << PatternIndex(4, 6, 5)
              << std::endl; // expected output: '(4, 6, 5)'
    std::cout << PatternIndex(-3, 7) << std::endl; // expected output: '(_, 7)'
    std::cout << PatternIndex(-3, 7, -2, -1)
              << std::endl; // expected output: '(_, 7)'
    std::cout << PatternIndex(PatternIndex::unspecified, 7)
              << std::endl; // expected output: '(_, 7)'
    std::cout << PatternIndex(PatternIndex::unspecified)
              << std::endl; // expected output: '()'
}

TEST(DeviceIdTest, PatternIndex_dim)
{
    EXPECT_EQ(PatternIndex().dim(), 0);
    EXPECT_EQ(PatternIndex(7).dim(), 1);
    EXPECT_EQ(PatternIndex(4, 6, 5).dim(), 3);
    EXPECT_EQ(PatternIndex(-3, 7).dim(), 2);
    EXPECT_EQ(PatternIndex(-3, 7, -2, -1).dim(), 2);
}

TEST(DeviceIdTest, PatternIndex_index)
{
    EXPECT_EQ(PatternIndex(7)[0], 7);
    EXPECT_EQ(PatternIndex(4, 6, 5)[0], 4);
    EXPECT_EQ(PatternIndex(4, 6, 5)[1], 6);
    EXPECT_EQ(PatternIndex(4, 6, 5)[2], 5);
    EXPECT_EQ(PatternIndex(4, 6, 5)[3], PatternIndex::unspecified);
    EXPECT_EQ(PatternIndex(4, 6, 5)[4], PatternIndex::unspecified);
}

// CartesianProductRange //////////////////////////////////////////////////////

TEST(DeviceIdTest, CartesianProductRange_PatternInputDomain)
{
    unsigned i = 0;
    auto expectedResults = std::vector<PatternIndex>{
        PatternIndex(4, 5, 100),  PatternIndex(4, 5, 200),
        PatternIndex(4, 7, 100),  PatternIndex(4, 7, 200),
        PatternIndex(4, 9, 100),  PatternIndex(4, 9, 200),
        PatternIndex(4, 10, 100), PatternIndex(4, 10, 200),
        PatternIndex(5, 5, 100),  PatternIndex(5, 5, 200),
        PatternIndex(5, 7, 100),  PatternIndex(5, 7, 200),
        PatternIndex(5, 9, 100),  PatternIndex(5, 9, 200),
        PatternIndex(5, 10, 100), PatternIndex(5, 10, 200),
        PatternIndex(8, 5, 100),  PatternIndex(8, 5, 200),
        PatternIndex(8, 7, 100),  PatternIndex(8, 7, 200),
        PatternIndex(8, 9, 100),  PatternIndex(8, 9, 200),
        PatternIndex(8, 10, 100), PatternIndex(8, 10, 200)};
    for (const auto& elem : CartesianProductRange(
             {PatternInputDomain(std::vector<syntax::DeviceIndex>{4, 5, 8}),
              PatternInputDomain(std::vector<syntax::DeviceIndex>{7, 5, 9, 10}),
              PatternInputDomain(std::vector<syntax::DeviceIndex>{100, 200})}))
    {
        EXPECT_TRUE(i < expectedResults.size());
        EXPECT_EQ(elem, expectedResults.at(i));
        i++;
    }
    EXPECT_EQ(i, expectedResults.size());
}

TEST(DeviceIdTest, CartesianProductRange_PatternInputDomain1)
{
    unsigned i = 0;
    auto expectedResults = std::vector<PatternIndex>{};
    for (const auto& elem : CartesianProductRange(
             {PatternInputDomain(std::vector<syntax::DeviceIndex>{4, 5, 8}),
              PatternInputDomain(std::vector<syntax::DeviceIndex>()),
              PatternInputDomain(std::vector<syntax::DeviceIndex>{100, 200})}))
    {
        std::cout << "elem = '" << elem << "'" << std::endl;
        EXPECT_TRUE(i < expectedResults.size());
        EXPECT_EQ(elem, expectedResults.at(i));
        i++;
    }
    EXPECT_EQ(i, expectedResults.size());
}

// DeviceIdPattern ////////////////////////////////////////////////////////////

std::map<unsigned, unsigned>
    mappingsSum(const std::vector<std::map<unsigned, unsigned>>& maps);

// testing::internal::PrintTo(
//     mappingsSum(
//         {{{2, 4}, {4, 16}, {6, 36}}, {{1, 1}, {3, 9}, {5, 25}, {7, 49}}}),
//     &std::cout);

TEST(DeviceIdTest, mappingsSum_empty)
{
    std::map<unsigned, unsigned> val({});
    EXPECT_EQ(mappingsSum({}), val);
}

TEST(DeviceIdTest, mappingsSum_single)
{
    std::map<unsigned, unsigned> map(
        {{1, 1}, {2, 1}, {3, 2}, {4, 3}, {5, 5}, {6, 8}, {7, 13}});
    EXPECT_EQ(mappingsSum(std::vector<std::map<unsigned, unsigned>>{map}), map);
}

TEST(DeviceIdTest, mappingsSum_disjoint)
{
    std::map<unsigned, unsigned> result(
        {{1, 1}, {2, 4}, {3, 9}, {4, 16}, {5, 25}, {6, 36}, {7, 49}});
    EXPECT_EQ(mappingsSum({{{2, 4}, {4, 16}, {6, 36}},
                           {{1, 1}, {3, 9}, {5, 25}, {7, 49}}}),
              result);
}

TEST(DeviceIdTest, mappingsSum_overlapping)
{
    std::map<unsigned, unsigned> result(
        {{1, 2}, {2, 3}, {3, 4}, {4, 5}, {5, 6}, {6, 7}});
    EXPECT_EQ(mappingsSum({{{1, 2}, {2, 3}, {3, 4}, {4, 5}},
                           {{3, 4}, {4, 5}, {5, 6}, {6, 7}}}),
              result);
}

TEST(DeviceIdTest, DeviceIdPattern_domain_iteration_single)
{
    DeviceIdPattern pat("[0|1-4:2-5]");
    auto expectedResults = std::vector<PatternIndex>{
        PatternIndex(1),
        PatternIndex(2),
        PatternIndex(3),
        PatternIndex(4),
    };
    unsigned i = 0;
    PatternIndex prevPi;
    for (const auto& pi : pat.domain())
    {
        EXPECT_TRUE(i < expectedResults.size());
        EXPECT_EQ(pi, expectedResults.at(i));
        EXPECT_TRUE(prevPi < pi);
        prevPi = pi;
        ++i;
    }
    EXPECT_EQ(i, expectedResults.size());
}

TEST(DeviceIdTest, DeviceIdPattern_domain_iteration_regular)
{
    DeviceIdPattern pat("[0|1-4:2-5][1|3-6:4-7][0|1-4:2-5]");
    unsigned i = 0;
    PatternIndex prevPi;
    for (const auto& pi : pat.domain())
    {
        switch (i)
        {
            case 0:
                EXPECT_EQ(pi, PatternIndex(1, 3));
                break;
            case 1:
                EXPECT_EQ(pi, PatternIndex(1, 4));
                break;
            case 2:
                EXPECT_EQ(pi, PatternIndex(1, 5));
                break;
            case 3:
                EXPECT_EQ(pi, PatternIndex(1, 6));
                break;
            case 4:
                EXPECT_EQ(pi, PatternIndex(2, 3));
                break;
            case 5:
                EXPECT_EQ(pi, PatternIndex(2, 4));
                break;
            case 6:
                EXPECT_EQ(pi, PatternIndex(2, 5));
                break;
            case 7:
                EXPECT_EQ(pi, PatternIndex(2, 6));
                break;
            case 8:
                EXPECT_EQ(pi, PatternIndex(3, 3));
                break;
            case 9:
                EXPECT_EQ(pi, PatternIndex(3, 4));
                break;
            case 10:
                EXPECT_EQ(pi, PatternIndex(3, 5));
                break;
            case 11:
                EXPECT_EQ(pi, PatternIndex(3, 6));
                break;
            case 12:
                EXPECT_EQ(pi, PatternIndex(4, 3));
                break;
            case 13:
                EXPECT_EQ(pi, PatternIndex(4, 4));
                break;
            case 14:
                EXPECT_EQ(pi, PatternIndex(4, 5));
                break;
            case 15:
                EXPECT_EQ(pi, PatternIndex(4, 6));
                break;
        }
        EXPECT_TRUE(prevPi < pi);
        prevPi = pi;
        ++i;
    }
    EXPECT_EQ(i, 16);
}

TEST(DeviceIdTest, DeviceIdPattern_domain_iteration_gap)
{
    DeviceIdPattern pat("[3|1-4:2-5]");
    unsigned i = 0;
    PatternIndex prevPi;
    for (const auto& pi : pat.domain())
    {
        switch (i)
        {
            case 0:
                EXPECT_EQ(pi, PatternIndex(PatternIndex::unspecified,
                                           PatternIndex::unspecified,
                                           PatternIndex::unspecified, 1));
                break;
            case 1:
                EXPECT_EQ(pi, PatternIndex(PatternIndex::unspecified,
                                           PatternIndex::unspecified,
                                           PatternIndex::unspecified, 2));
                break;
            case 2:
                EXPECT_EQ(pi, PatternIndex(PatternIndex::unspecified,
                                           PatternIndex::unspecified,
                                           PatternIndex::unspecified, 3));
                break;
            case 3:
                EXPECT_EQ(pi, PatternIndex(PatternIndex::unspecified,
                                           PatternIndex::unspecified,
                                           PatternIndex::unspecified, 4));
                break;
        }
        EXPECT_TRUE(prevPi < pi);
        prevPi = pi;
        ++i;
    }
    EXPECT_EQ(i, 4);
}

TEST(DeviceIdTest, DeviceIdPattern_domain_iteration_gap1)
{
    DeviceIdPattern pat("[1|21-22:11][3|1-4:2-5]");
    unsigned i = 0;
    PatternIndex prevPi;
    for (const auto& pi : pat.domain())
    {
        switch (i)
        {
            case 0:
                EXPECT_EQ(pi, PatternIndex(PatternIndex::unspecified, 21,
                                           PatternIndex::unspecified, 1));
                break;
            case 1:
                EXPECT_EQ(pi, PatternIndex(PatternIndex::unspecified, 21,
                                           PatternIndex::unspecified, 2));
                break;
            case 2:
                EXPECT_EQ(pi, PatternIndex(PatternIndex::unspecified, 21,
                                           PatternIndex::unspecified, 3));
                break;
            case 3:
                EXPECT_EQ(pi, PatternIndex(PatternIndex::unspecified, 21,
                                           PatternIndex::unspecified, 4));
                break;
            case 4:
                EXPECT_EQ(pi, PatternIndex(PatternIndex::unspecified, 22,
                                           PatternIndex::unspecified, 1));
                break;
            case 5:
                EXPECT_EQ(pi, PatternIndex(PatternIndex::unspecified, 22,
                                           PatternIndex::unspecified, 2));
                break;
            case 6:
                EXPECT_EQ(pi, PatternIndex(PatternIndex::unspecified, 22,
                                           PatternIndex::unspecified, 3));
                break;
            case 7:
                EXPECT_EQ(pi, PatternIndex(PatternIndex::unspecified, 22,
                                           PatternIndex::unspecified, 4));
                break;
        }
        EXPECT_TRUE(prevPi < pi);
        prevPi = pi;
        ++i;
    }
    EXPECT_EQ(i, 8);
}

TEST(DeviceIdTest, DeviceIdPattern_domain_iteration_empty)
{
    DeviceIdPattern pat;
    unsigned i = 0;
    for (const auto& pi : pat.domain())
    {
        switch (i)
        {
            case 0:
                EXPECT_EQ(pi, PatternIndex());
                break;
        }
        ++i;
    }
    EXPECT_EQ(i, 1);
}

// marcinw:TODO: to remove
// TEST(DeviceIdTest, DeviceIdPattern_domainVec)
// {
//     DeviceIdPattern pat =
//         DeviceIdPattern(std::vector<unsigned>({1, 0}),
//                         std::vector<std::map<unsigned, unsigned>>(
//                             {{{17, 71}, {13, 31}}, {{731, 137}, {246,
//                             642}}}));
//     EXPECT_THAT(pat.domainVec(),
//                 ElementsAre(PatternIndex(246, 13), PatternIndex(246, 17),
//                             PatternIndex(731, 13), PatternIndex(731, 17)));
// }

// TEST(DeviceIdTest, DeviceIdPattern_checkIsInDomain_regular)
// {
//     DeviceIdPattern pat =
//         DeviceIdPattern(std::vector<unsigned>({1, 0}),
//                         std::vector<std::map<unsigned, unsigned>>(
//                             {{{17, 71}, {13, 31}}, {{731, 137}, {246,
//                             642}}}));
//     EXPECT_ANY_THROW(
//         pat.checkIsInDomain(PatternIndex(PatternIndex::unspecified, 17)));
//     EXPECT_NO_THROW(pat.checkIsInDomain(PatternIndex(731, 13)));
//     EXPECT_NO_THROW(pat.checkIsInDomain(PatternIndex(731, 13, 5)));
//     EXPECT_ANY_THROW(pat.checkIsInDomain(PatternIndex(731)));
//     EXPECT_ANY_THROW(pat.checkIsInDomain(PatternIndex()));
// }

// TEST(DeviceIdTest, DeviceIdPattern_checkIsInDomain_gap)
// {
//     DeviceIdPattern pat("[1|17:71,13:31]");
//     // = DeviceIdPattern(
//     //    std::vector<unsigned>({1}),
//     //    std::vector<std::map<unsigned, unsigned>>({{{17, 71}, {13, 31}}}));
//     EXPECT_NO_THROW(
//         pat.checkIsInDomain(PatternIndex(PatternIndex::unspecified, 17)));
//     EXPECT_NO_THROW(pat.checkIsInDomain(PatternIndex(31337, 17)));
//     EXPECT_ANY_THROW(
//         pat.checkIsInDomain(PatternIndex(17, PatternIndex::unspecified)));
//     EXPECT_ANY_THROW(pat.checkIsInDomain(PatternIndex()));
//     EXPECT_ANY_THROW(pat.checkIsInDomain(PatternIndex(17)));
// }

TEST(DeviceIdTest, DeviceIdPattern_checkIsInDomain_single)
{
    DeviceIdPattern five("_[5]_");
    EXPECT_NO_THROW(five.checkIsInDomain(PatternIndex(5)));
    EXPECT_ANY_THROW(five.checkIsInDomain(PatternIndex(6)));
    EXPECT_ANY_THROW(five.checkIsInDomain(PatternIndex()));
    EXPECT_NO_THROW(five.checkIsInDomain(PatternIndex(5, 2, 2)));
    DeviceIdPattern five1("_[1|5]_");
    EXPECT_NO_THROW(
        five1.checkIsInDomain(PatternIndex(PatternIndex::unspecified, 5)));
    EXPECT_NO_THROW(five1.checkIsInDomain(
        PatternIndex(PatternIndex::unspecified, 5, PatternIndex::unspecified)));
    EXPECT_NO_THROW(
        five1.checkIsInDomain(PatternIndex(PatternIndex::unspecified, 5, 6)));
    EXPECT_NO_THROW(five1.checkIsInDomain(PatternIndex(1, 5, 6)));
    EXPECT_ANY_THROW(
        five1.checkIsInDomain(PatternIndex(1, PatternIndex::unspecified, 6)));
    EXPECT_ANY_THROW(five1.checkIsInDomain(PatternIndex(1, 7, 6)));
}

TEST(DeviceIdTest, DeviceIdPattern_checkIsInDomain_span)
{
    DeviceIdPattern span("_[10-20]_");
    for (unsigned i = 0u; i < 30; ++i)
    {
        if (10 <= i && i <= 20)
        {
            EXPECT_NO_THROW(span.checkIsInDomain(PatternIndex(i)));
            EXPECT_NO_THROW(span.checkIsInDomain(
                PatternIndex(i, PatternIndex::unspecified)));
            EXPECT_NO_THROW(span.checkIsInDomain(PatternIndex(i, 4, 5, 0)));
            EXPECT_NO_THROW(span.checkIsInDomain(PatternIndex(i, i)));
            EXPECT_ANY_THROW(span.checkIsInDomain(
                PatternIndex(PatternIndex::unspecified, i)));
            EXPECT_ANY_THROW(span.checkIsInDomain(PatternIndex(0, i)));
        }
        else // ! 10 <= i && i <= 20
        {
            EXPECT_ANY_THROW(span.checkIsInDomain(PatternIndex(i)));
            EXPECT_ANY_THROW(span.checkIsInDomain(
                PatternIndex(i, PatternIndex::unspecified)));
            EXPECT_ANY_THROW(span.checkIsInDomain(PatternIndex(i, 4, 5, 0)));
            EXPECT_ANY_THROW(span.checkIsInDomain(PatternIndex(i, i)));
            EXPECT_ANY_THROW(span.checkIsInDomain(
                PatternIndex(PatternIndex::unspecified, i)));
            EXPECT_ANY_THROW(span.checkIsInDomain(PatternIndex(0, i)));
        }
    }
}

TEST(DeviceIdTest, DeviceIdPattern_checkIsInDomain_span_gap)
{
    DeviceIdPattern span("_[2|10-20]_");
    for (unsigned i = 0u; i < 30; ++i)
    {
        if (10 <= i && i <= 20)
        {
            EXPECT_NO_THROW(span.checkIsInDomain(PatternIndex(
                PatternIndex::unspecified, PatternIndex::unspecified, i)));
        }
        else // ! 10 <= i && i <= 20
        {
            EXPECT_ANY_THROW(span.checkIsInDomain(PatternIndex(
                PatternIndex::unspecified, PatternIndex::unspecified, i)));
        }
    }
}

TEST(DeviceIdTest, DeviceIdPattern_checkIsInDomain_empty)
{
    DeviceIdPattern pat;
    EXPECT_NO_THROW(pat.checkIsInDomain(PatternIndex()));
    EXPECT_NO_THROW(
        pat.checkIsInDomain(PatternIndex(PatternIndex::unspecified)));
    EXPECT_NO_THROW(pat.checkIsInDomain(PatternIndex(1, 4)));
}

TEST(DeviceIdTest, DeviceIdPattern_eval)
{
    DeviceIdPattern pat("abc_[1|17-18:71-72]_def_[0|731-732:246-247]_ghi");
    EXPECT_EQ(pat.eval(PatternIndex(731, 18)), "abc_72_def_246_ghi");
    EXPECT_EQ(pat.eval(PatternIndex(732, 18)), "abc_72_def_247_ghi");
    EXPECT_EQ(pat.eval(PatternIndex(731, 17)), "abc_71_def_246_ghi");
    EXPECT_EQ(pat.eval(PatternIndex(732, 17)), "abc_71_def_247_ghi");
    EXPECT_EQ(pat.eval(PatternIndex(732, 17, 100)), "abc_71_def_247_ghi");
    EXPECT_EQ(pat.eval(PatternIndex(732, 17, 100, 9)), "abc_71_def_247_ghi");
    EXPECT_ANY_THROW(pat.eval(PatternIndex(732)));
    EXPECT_ANY_THROW(pat.eval(PatternIndex(732, 19)));
}

TEST(DeviceIdTest, DeviceIdPattern_values)
{
    DeviceIdPattern pat("abc_[1|17-18:71-72]_def_[0|731-732:246-247]_ghi");
    unsigned i = 0;
    for (const auto& v : pat.values())
    {
        switch (i)
        {
            case 0:
                EXPECT_EQ(v, "abc_71_def_246_ghi");
                break;
            case 1:
                EXPECT_EQ(v, "abc_72_def_246_ghi");
                break;
            case 2:
                EXPECT_EQ(v, "abc_71_def_247_ghi");
                break;
            case 3:
                EXPECT_EQ(v, "abc_72_def_247_ghi");
                break;
        }
        ++i;
    }
    EXPECT_EQ(i, 4);
    EXPECT_TRUE(pat.isInjective());
}

TEST(DeviceIdTest, DeviceIdPattern_match_empty)
{
    DeviceIdPattern pat("PCIeRetimer");
    EXPECT_FALSE(pat.matches("GPU_SXM_2"));
    EXPECT_THAT(pat.match("GPU_SXM_2"), ElementsAre());
    EXPECT_FALSE(pat.matches(""));
    EXPECT_THAT(pat.match(""), ElementsAre());
    EXPECT_TRUE(pat.matches("PCIeRetimer"));
    EXPECT_THAT(pat.match("PCIeRetimer"), ElementsAre(PatternIndex()));
    EXPECT_TRUE(pat.isInjective());
}

TEST(DeviceIdTest, DeviceIdPattern_match_single)
{
    DeviceIdPattern pat("GPU_SXM_[1-8]");
    EXPECT_TRUE(pat.matches("GPU_SXM_2"));
    EXPECT_THAT(pat.match("GPU_SXM_2"), ElementsAre(PatternIndex(2)));
    EXPECT_TRUE(pat.matches("GPU_SXM_5"));
    EXPECT_THAT(pat.match("GPU_SXM_5"), ElementsAre(PatternIndex(5)));
    EXPECT_FALSE(pat.matches("GPU_SXM_10"));
    EXPECT_THAT(pat.match("GPU_SXM_10"), ElementsAre());
    EXPECT_FALSE(pat.matches("HGX_GPU_SXM_1"));
    EXPECT_THAT(pat.match("HGX_GPU_SXM_1"), ElementsAre());
    EXPECT_TRUE(pat.isInjective());
}

TEST(DeviceIdTest, DeviceIdPattern_match_double)
{
    DeviceIdPattern pat("GPU_SXM_[1-8]/NVLink_[0-39]");
    EXPECT_TRUE(pat.matches("GPU_SXM_4/NVLink_13"));
    EXPECT_THAT(pat.match("GPU_SXM_4/NVLink_13"),
                UnorderedElementsAre(PatternIndex(4, 13)));
    EXPECT_TRUE(pat.matches("GPU_SXM_1/NVLink_0"));
    EXPECT_THAT(pat.match("GPU_SXM_1/NVLink_0"),
                UnorderedElementsAre(PatternIndex(1, 0)));
    EXPECT_TRUE(pat.matches("GPU_SXM_8/NVLink_39"));
    EXPECT_THAT(pat.match("GPU_SXM_8/NVLink_39"),
                UnorderedElementsAre(PatternIndex(8, 39)));
    EXPECT_FALSE(pat.matches("GPU_SXM_9/NVLink_14"));
    EXPECT_THAT(pat.match("GPU_SXM_9/NVLink_14"), UnorderedElementsAre());
    EXPECT_TRUE(pat.isInjective());
}

TEST(DeviceIdTest, DeviceIdPattern_dimDomain)
{
    DeviceIdPattern pat("[1-3:11-13][4-7]");
    EXPECT_EQ(pat.dimDomain(0),
              PatternInputDomain(std::vector<unsigned>({1, 2, 3})));
    EXPECT_EQ(pat.dimDomain(1),
              PatternInputDomain(std::vector<unsigned>({4, 5, 6, 7})));
    EXPECT_EQ(pat.dimDomain(2), PatternInputDomain());
}

// TEST(DeviceIdTest, DeviceIdPattern_match_noninjective)
// {
//     // "HSC_[0|0-3:8,4-7:9]";
//     DeviceIdPattern pat("HSC_[0|0-3:8,4-7:9]");
//     EXPECT_TRUE(pat.matches("HSC_8"));
//     EXPECT_THAT(pat.match("HSC_8"),
//                 UnorderedElementsAre(PatternIndex(0), PatternIndex(1),
//                                      PatternIndex(2), PatternIndex(3)));
//     EXPECT_TRUE(pat.matches("HSC_9"));
//     EXPECT_THAT(pat.match("HSC_9"),
//                 UnorderedElementsAre(PatternIndex(4), PatternIndex(5),
//                                      PatternIndex(6), PatternIndex(7)));
//     EXPECT_FALSE(pat.matches("HSC_0"));
//     EXPECT_THAT(pat.match("HSC_0"), UnorderedElementsAre());
//     EXPECT_FALSE(pat.isInjective());
// }

TEST(DeviceIdTest, DeviceIdPattern_separateTextAndBracketContents_single)
{
    std::vector<std::string> texts;
    std::vector<std::string> bracketContents;
    DeviceIdPattern::separateTextAndBracketContents("GPU_SXM_[1-8]", texts,
                                                    bracketContents);
    EXPECT_THAT(texts, ElementsAre("GPU_SXM_", ""));
    EXPECT_THAT(bracketContents, ElementsAre("1-8"));
}

TEST(DeviceIdTest, DeviceIdPattern_separateTextAndBracketContents_double)
{
    std::vector<std::string> texts;
    std::vector<std::string> bracketContents;
    DeviceIdPattern::separateTextAndBracketContents(
        "GPU_SXM_[1-8]/NVLink_[0-39]", texts, bracketContents);
    EXPECT_THAT(texts, ElementsAre("GPU_SXM_", "/NVLink_", ""));
    EXPECT_THAT(bracketContents, ElementsAre("1-8", "0-39"));
}

TEST(DeviceIdTest, DeviceIdPattern_separateTextAndBracketContents_none)
{
    std::vector<std::string> texts;
    std::vector<std::string> bracketContents;
    DeviceIdPattern::separateTextAndBracketContents("Baseboard", texts,
                                                    bracketContents);
    EXPECT_THAT(texts, ElementsAre("Baseboard"));
    EXPECT_THAT(bracketContents, ElementsAre());
}

TEST(DeviceIdTest, DeviceIdPattern_separateTextAndBracketContents_empty)
{
    std::vector<std::string> texts;
    std::vector<std::string> bracketContents;
    DeviceIdPattern::separateTextAndBracketContents("", texts, bracketContents);
    EXPECT_THAT(texts, ElementsAre(""));
    EXPECT_THAT(bracketContents, ElementsAre());
}

TEST(DeviceIdTest, shiftedMappingsSameInputPos)
{
    DeviceIdPattern pat("FPGA_SXM[1-8:0-7]_EROT_RECOV_L GPU_SXM_[0|1-8]");
    EXPECT_THAT(pat.valuesVec(),
                ElementsAre("FPGA_SXM0_EROT_RECOV_L GPU_SXM_1",
                            "FPGA_SXM1_EROT_RECOV_L GPU_SXM_2",
                            "FPGA_SXM2_EROT_RECOV_L GPU_SXM_3",
                            "FPGA_SXM3_EROT_RECOV_L GPU_SXM_4",
                            "FPGA_SXM4_EROT_RECOV_L GPU_SXM_5",
                            "FPGA_SXM5_EROT_RECOV_L GPU_SXM_6",
                            "FPGA_SXM6_EROT_RECOV_L GPU_SXM_7",
                            "FPGA_SXM7_EROT_RECOV_L GPU_SXM_8"));
}

TEST(DeviceIdTest, DeviceIdPattern_full)
{
    DeviceIdPattern ip(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_[1-8]");

    EXPECT_THAT(ip.domainVec(),
                ElementsAre(PatternIndex(1), PatternIndex(2), PatternIndex(3),
                            PatternIndex(4), PatternIndex(5), PatternIndex(6),
                            PatternIndex(7), PatternIndex(8)));

    EXPECT_THAT(
        ip.values(),
        ElementsAre(
            "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_1",
            "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_2",
            "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_3",
            "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_4",
            "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_5",
            "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_6",
            "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_7",
            "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_8"));

    EXPECT_EQ(ip(6),
              "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_6");
    EXPECT_EQ(ip(1),
              "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_1");
    EXPECT_EQ(ip(5, 7),
              "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_5");

    EXPECT_TRUE(ip.matches(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_4"));
    EXPECT_FALSE(ip.matches(""));
    EXPECT_FALSE(ip.matches(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_23"));

    EXPECT_THAT(
        ip.match("/xyz/openbmc_project/inventory/system/processors/GPU_SXM_2"),
        ElementsAre(PatternIndex(2)));
    EXPECT_THAT(
        ip.match("/xyz/openbmc_project/inventory/system/processors/GPU_SXM_4"),
        ElementsAre(PatternIndex(4)));
    EXPECT_THAT(ip.match("GPU_SXM_4"), ElementsAre());

    EXPECT_TRUE(ip.isInjective());

    EXPECT_EQ(ip.dim(), 1);

    // EXPECT_THAT(ip.dimDomain(0), ElementsAre(1, 2, 3, 4, 5, 6, 7, 8));
}

// Consistency check on all the strings found in event_info.json
TEST(DeviceIdTest, DeviceIdPattern_consistency_all_event_info)
{
    DeviceIdPattern ip("Critical");
    auto eventInfoStrings = {
        "",
        "-invalidate GPU_SXM_[1-8]",
        "-invalidate NVSwitch_[0-3]",
        "-invalidate PCIeSwitch_0",
        "/xyz/openbmc_project/GpioStatusHandler",
        "/xyz/openbmc_project/GpuOobRecovery",
        "/xyz/openbmc_project/inventory/system/chassis/HGX_Chassis_0",
        "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_[1-8]",
        "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_[1-8]/PCIeDevices/GPU_SXM_[0|1-8]",
        "/xyz/openbmc_project/inventory/system/chassis/HGX_NVSwitch_0/PCIeDevices/NVSwitch_0",
        "/xyz/openbmc_project/inventory/system/chassis/HGX_NVSwitch_[0-3]",
        "/xyz/openbmc_project/inventory/system/chassis/HGX_NVSwitch_[0-3]/PCIeDevices/NVSwitch_[0|0-3]",
        "/xyz/openbmc_project/inventory/system/chassis/HGX_PCIeSwitch_0/PCIeDevices/PCIeSwitch_0",
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_[0-3]",
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_[0-3]/Ports/NVLink_[0-39]",
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_[0-7]/Switches/PCIeRetimer_[0|0-7]",
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_[0-7]/Switches/PCIeRetimer_[0|0-7]/Ports/DOWN_0",
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeSwitchTopology_0/Switches/PCIeSwitch_0/Ports/DOWN_[0-3]",
        "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeSwitchTopology_0/Switches/PCIeSwitch_0/Ports/UP_0",
        "/xyz/openbmc_project/inventory/system/memory/GPU_SXM_[1-8]_DRAM_0",
        "/xyz/openbmc_project/inventory/system/processors/FPGA_0/Ports/PCIeToHMC_0",
        "/xyz/openbmc_project/inventory/system/processors/FPGA_0/Ports/PCIeToHost_0",
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_[1-8]",
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_[1-8]/Ports/NVLink_[0-17]",
        "/xyz/openbmc_project/sensors/power/HGX_Chassis_0_HSC_[0-9]_Power_0",
        "/xyz/openbmc_project/sensors/temperature/HGX_Chassis_0_PCB_0_Temp_0",
        "/xyz/openbmc_project/sensors/temperature/HGX_GPU_SXM_[1-8]_TEMP_0",
        "/xyz/openbmc_project/sensors/temperature/HGX_NVSwitch_[0-3]_TEMP_0",
        "/xyz/openbmc_project/sensors/temperature/HGX_PCIeRetimer_[0-7]_TEMP_0",
        "/xyz/openbmc_project/sensors/temperature/HGX_PCIeSwitch_0_TEMP_0",
        "/xyz/openbmc_project/software/HGX_FW_BMC_0",
        "/xyz/openbmc_project/software/HGX_FW_GPU_SXM_[1-8]",
        "/xyz/openbmc_project/software/HGX_FW_NVSwitch_[0-3]",
        "0",
        "0x0000",
        "0x7FFFFFFF",
        "0xfa1d",
        "1",
        "1024",
        "128",
        "16",
        "16384",
        "1V8 Abnormal Power Change",
        "2",
        "2048",
        "256",
        "32",
        "32768",
        "3V3 Abnormal Power Change",
        "4",
        "4096",
        "512",
        "5V Abnormal Power Change",
        "6",
        "64",
        "768",
        "769",
        "770",
        "774",
        "8",
        "8192",
        "<NOTANEVENT> GPU mctp-vdm-util cache invalidation",
        "<NOTANEVENT> NVSwitch mctp-vdm-util cache invalidation",
        "<NOTANEVENT> PCIeSwitch mctp-vdm-util cache invalidation",
        "AP0_BOOTCOMPLETE_TIMEOUT GPU_SXM_[1-8]",
        "AP0_BOOTCOMPLETE_TIMEOUT NVSwitch_[0-3]",
        "AP0_BOOTCOMPLETE_TIMEOUT PCIeSwitch_0",
        "AP0_SPI_READ_FAILURE GPU_SXM_[1-8]",
        "AP0_SPI_READ_FAILURE NVSwitch_[0-3]",
        "AP0_SPI_READ_FAILURE PCIeSwitch_0",
        "Abnormal Power Change",
        "Abnormal Presence State Change",
        "Abnormal Speed Change",
        "Abnormal State Change",
        "Abnormal Width Change",
        "AbnormalPowerChange",
        "Absent",
        "ActiveWidth",
        "Alert caused by HSC alert",
        "BMC PCIe Uncorrectable Error",
        "BackendErrorCode",
        "Base.1.13.ResetRecommended",
        "Base.1.13.ResetRequired",
        "BaseBoard components will retry if encounters errors. If problem persists, analyze FPGA log or ERoT log.",
        "Baseboard GPU Presence change Interrupt",
        "Baseboard_0",
        "Baseboard_0 HSC Fault",
        "Boot Complete Failure",
        "Boot completed failure",
        "CMDLINE",
        "Chassis_0_PCB_0_Temp_0",
        "Circuit breaker fault triggered or status is present is STATUS_OTHER byte",
        "Collect ERoT logs and cordon HGX server from the cluster for RMA evaluation.",
        "Communication fault has occurred",
        "Controlled by Host BMC or a power cycle of baseboard would re-enable SMBPBI access privilege to HMC",
        "Cordon the HGX server node for further diagnosis.",
        "Cordon the HGX server node from the cluster for RMA evaluation.",
        "Correctable Error Count {PCIe Correctable Error}",
        "Critical",
        "CriticalHigh",
        "Current PCIe link speed",
        "Current PCIe link width",
        "Current Temperature",
        "CurrentDeviceName",
        "CurrentSpeed",
        "DBUS",
        "DIRECT",
        "DVDD Abnormal Power Change",
        "DataCRCCount",
        "Device is busy and could not respond to PMBus access",
        "DeviceCoreAPI",
        "Drain and Reset Recommended",
        "DrainAndResetRequired",
        "EEPROM Error",
        "ERoT Fatal Abnormal State Change",
        "ERoT Recovery",
        "ERoT is in recovery mode, and HostBMC need to deassert EROT_RECOV and reboot the device. If problem persists, collect ERoT logs and cordon HGX server from the cluster for RMA evaluation.",
        "ERoT will try to recover AP by a reset/reboot. If there is still a problem, collect ERoT logs and reflash AP FW with recovery flow.",
        "ERoT_GPU_SXM_[0|1-8] ERoT_Fatal",
        "ERoT_GPU_SXM_[0|1-8] Firmware",
        "ERoT_NVSwitch_[0|0-3] ERoT_Fatal",
        "ERoT_NVSwitch_[0|0-3] Firmware",
        "ERoT_PCIeSwitch_0 ERoT_Fatal",
        "ERoT_PCIeSwitch_0 Firmware",
        "FET is shorted or manufacture specific fault or warning has occurred",
        "FPGA - BMC PCIe Error",
        "FPGA - HMC PCIe Error",
        "FPGA BMC PCIe Correctable Error Count",
        "FPGA BMC PCIe Fatal Error Count",
        "FPGA BMC PCIe Non Fatal Error Count",
        "FPGA HMC PCIe Correctable Error Count",
        "FPGA HMC PCIe Fatal Error Count",
        "FPGA HMC PCIe Non Fatal Error Count",
        "FPGA Temp Alert",
        "FPGA Temperature",
        "FPGA Thermal Parameter",
        "FPGA_0",
        "FPGA_0 PCIe",
        "FPGA_0 Temperature",
        "FPGA_0_TEMP_0",
        "FPGA_NVSW[0-3]_EROT_RECOV_L NVSwitch_[0-3]",
        "FPGA_PEXSW_EROT_RECOV_L PCIeSwitch_0",
        "FPGA_SXM[0-7]_EROT_RECOV_L GPU_SXM_[1-8]",
        "FW Update with WP engaged - GPU",
        "FW Update with WP engaged - HMC",
        "FW Update with WP engaged - NVSwitch",
        "FW_ERoT_GPU_SXM_[0|1-8]",
        "FW_ERoT_NVSwitch_[0|0-3]",
        "FW_ERoT_PCIeSwitch_0",
        "Failover to Redundant Interface(i2c)",
        "Fatal Error Count {PCIe Fatal Error}",
        "FlitCRCCount",
        "For triage, contact Nvidia support.",
        "ForceRestart",
        "GPU",
        "GPU Drain and Reset Recommended",
        "GPU Nvlink Recovery Error",
        "GPU Nvlink Training Error",
        "GPU Power Good Abnormal change",
        "GPU Reset Required",
        "GPU_SXM_[0|1-8]",
        "GPU_SXM_[0|1-8] Firmware",
        "GPU_SXM_[0|1-8] I2C",
        "GPU_SXM_[0|1-8] Memory",
        "GPU_SXM_[0|1-8] NVLink_[1|0-17]",
        "GPU_SXM_[0|1-8] PCIe",
        "GPU_SXM_[0|1-8] PowerGood",
        "GPU_SXM_[0|1-8] SRAM",
        "GPU_SXM_[0|1-8] Temperature",
        "GPU_SXM_[0|1-8] ThrottleReason",
        "GPU_SXM_[0|1-8]/NVLink_[1|0-17]",
        "GPU_SXM_[0|1-8]_DRAM_0",
        "GPU_SXM_[0|1-8]_TEMP_0",
        "GPU_SXM_[0|1-8]_VR",
        "GPU_SXM_[1-8]",
        "HMC SMBPBI Failover",
        "HMC {PCIe Uncorrectable Error}",
        "HMC_0",
        "HMC_0_VR_1V8",
        "HMC_0_VR_3V3",
        "HSC Current Power",
        "HSC Power Good Abnormal change",
        "HSC detailed alert status",
        "HSC_[0-9]",
        "HSC_[0|0-9]_VR",
        "HVDD Abnormal Power Change",
        "Hang",
        "HardShutdownHigh",
        "I2C EEPROM Error",
        "I2C Hang - Bus Timeout",
        "I2C Hang - FSM Timeout",
        "I2C3_ALERT",
        "I2C4_ALERT",
        "If WP enabled, ERoT/AP shall not be able to update SPI. This is a HW protection.",
        "Incorrect link speed",
        "Incorrect link width",
        "Increase the BaseBoard cooling and check thermal environmental to ensure Inlet Temperature reading is under UpperCritical threshold.",
        "Increase the BaseBoard cooling and check thermal environmental to ensure Inlet Temperature reading is under UpperCritical threshold. ",
        "Increase the BaseBoard cooling and check thermal environmental to ensure the GPU operates under UpperCritical threshold. ",
        "Increase the BaseBoard cooling and check thermal environmental to ensure the NVSwtich operates under UpperCritical threshold. ",
        "Inlet Sensor Temperature Alert",
        "Inlet Thermal Parameters",
        "Inlet_Temp_0",
        "Inlet_Temp_0 (FPGA_0) Temperature",
        "Input voltage or current fault has occurred",
        "LTSSM Link Down",
        "LTSSMState",
        "LanesInUse",
        "MCTP Runtime Failure",
        "MCTP runtime failures",
        "MinSpeed",
        "NVLink Data CRC Error Count",
        "NVLink Data CRC Error count",
        "NVLink Flit CRC Error Count",
        "NVLink Flit CRC Error count",
        "NVLink Recovery Error Count",
        "NVLink Recovery Error count",
        "NVLink Recovery Error count is {Nvlink Recovery Error}",
        "NVLink Replay Error Count",
        "NVLink Replay Error count",
        "NVLink Training Error count is {Nvlink Training Error}",
        "NVSW PRSNT state run time change",
        "NVSW[1-4]_INT_PRSNT_N NVSwitch_[0-3]",
        "NVSwitch",
        "NVSwitch 1V8 Abnormal change",
        "NVSwitch 3V3 Abnormal change",
        "NVSwitch 5V Abnormal change",
        "NVSwitch DVDD Abnormal change",
        "NVSwitch HVDD Abnormal change",
        "NVSwitch IBC Abnormal change",
        "NVSwitch NVLink Training Error",
        "NVSwitch Nvlink Recovery Error",
        "NVSwitch PCIe link speed",
        "NVSwitch VDD Abnormal change",
        "NVSwitch_[0-3]",
        "NVSwitch_[0|0-3]",
        "NVSwitch_[0|0-3] Firmware",
        "NVSwitch_[0|0-3] I2C",
        "NVSwitch_[0|0-3] NVLink_[1|0-39]",
        "NVSwitch_[0|0-3] PCIe",
        "NVSwitch_[0|0-3] PowerGood",
        "NVSwitch_[0|0-3] SMBPBI",
        "NVSwitch_[0|0-3] Temperature",
        "NVSwitch_[0|0-3]/NVLink_[1|0-39]",
        "NVSwitch_[0|0-3]_TEMP_0",
        "NVSwitch_[0|0-3]_VR_3V3",
        "NVSwitch_[0|0-3]_VR_5V",
        "NVSwitch_[0|0-3]_VR_DVDD",
        "NVSwitch_[0|0-3]_VR_HVDD",
        "NVSwitch_[0|0-3]_VR_VDD",
        "NVSwitch_[0|0-3]_VR_VDD_1V8",
        "No immediate recovery required. If this event occurs multiple times in a short period, need to check other PCIe error status for further diagnostics.",
        "No immediate recovery required. Reset the GPU at next service window.",
        "No recovery required since FPGA will recover the fault automatically",
        "No recovery required.",
        "NonFatal Error Count {PCIe NonFatal Error}",
        "Not supported; always 0",
        "One or more bits in MFR_SYSTEM_STATUS1 are set",
        "Output Overcurrent fault has occurred",
        "Output Overvoltage fault has occurred",
        "Output current/power fault or warning has occurred",
        "Output voltage fault or warning has occurred",
        "OverTemp",
        "PCB Thermal Warning",
        "PCIe LTSSM state",
        "PCIe LTSSM state downstream",
        "PCIe LTSSM state upstream",
        "PCIe Link Error - Downstream",
        "PCIe Link Error - Upstream",
        "PCIe Link Speed State Change",
        "PCIe Link Width State Change",
        "PCIe Switch 0V8 Abnormal Power change",
        "PCIe Uncorrectable Error {PCIe Uncorrectable Error}",
        "PCIe fatal error counts - Downstream",
        "PCIe fatal error counts - Upstream",
        "PCIe link error - Correctable",
        "PCIe link error - Fatal",
        "PCIe link error - NonFatal",
        "PCIe link goes down - Downstream",
        "PCIe link goes down - Upstream",
        "PCIe link speed state",
        "PCIe link width state",
        "PCIe non fatal error counts - Downstream",
        "PCIe non fatal error counts - Upstream",
        "PCIe uncorrectable error - Downstream",
        "PCIe uncorrectable error - Upstream",
        "PCIeRetimer 0V9 Abnormal Power Change",
        "PCIeRetimer 1V8VDD Abnormal Power Change",
        "PCIeRetimer PCIe Link Speed",
        "PCIeRetimer PCIe Link Width",
        "PCIeRetimer Power Change",
        "PCIeRetimer_[0-7]",
        "PCIeRetimer_[0|0-7] 0v9 PowerGood",
        "PCIeRetimer_[0|0-7] 1.8v PowerGood",
        "PCIeRetimer_[0|0-7] Firmware",
        "PCIeRetimer_[0|0-7] PCIe",
        "PCIeRetimer_[0|0-7] PowerGood",
        "PCIeRetimer_[0|0-7] Temperature",
        "PCIeRetimer_[0|0-7]_TEMP_0",
        "PCIeRetimer_[0|0-7]_VR_0V9",
        "PCIeRetimer_[0|0-7]_VR_1V8VDD",
        "PCIeSwitch PCIe Link Speed",
        "PCIeSwitch PCIe Link Width",
        "PCIeSwitch_0",
        "PCIeSwitch_0 Firmware",
        "PCIeSwitch_0 PCIe Upstream Port",
        "PCIeSwitch_0 PowerGood",
        "PCIeSwitch_0 Temperature",
        "PCIeSwitch_0 UP PCIe",
        "PCIeSwitch_0/DOWN_[0-3]",
        "PCIeSwitch_0/DOWN_[0|0-3]",
        "PCIeSwitch_0/DOWN_[0|0-3] PCIe",
        "PCIeSwitch_0/DOWN_[0|0-3] PCIe Downstream Port",
        "PCIeSwitch_0_TEMP_0",
        "PCIeSwitch_0_VR_0V8",
        "PCIeType",
        "Physically Missing",
        "Power Good signal has been negated",
        "Power cycle the BaseBoard. If the problem persists, isolate the server for RMA evaluation",
        "Power cycle the BaseBoard. If the problem persists, isolate the server for RMA evaluation.",
        "Power off the BaseBoard within 1 second, and GPU will shutdown autonomously. Check thermal environmental to ensure the GPU operates under UpperFatal threshold and power cycle the Baseboard to recover.",
        "Power off the BaseBoard within 1 second, and the Retimer will be held in reset. Check thermal environmental to ensure the Retimer operates under UpperFatal threshold and power cycle the BaseBoard to recover. ",
        "Power off the BaseBoard within 1 second, the NVSwitch will be held in reset. Check thermal environmental to ensure the NVSwitch operates under UpperFatal threshold and power cycle the BaseBoard to recover. ",
        "Power off the BaseBoard within 1 second, the PCIe Switch will be held in reset. Check thermal environmental to ensure the PCIe Switch operates under UpperFatal threshold and power cycle the BaseBoard to recover. ",
        "Power off the BaseBoard within 1 second. Check thermal environmental to ensure FPGA operates under UpperFatal threshold and power cycle the BaseBoard to recover. ",
        "Privilege Disabled",
        "RecoveryCount",
        "ReplayErrorsCount",
        "Reset Required",
        "Reset the GPU or power cycle the BaseBoard. If problem persists, isolate the server for RMA evaluation.",
        "Reset the GPU. If the GPU continues to exhibit the problem, isolate the server for RMA evaluation.",
        "Reset the link. If problem persists, isolate the server for RMA evaluation.",
        "ResetRequired",
        "ResourceEvent.1.0.ResourceErrorThresholdExceeded",
        "ResourceEvent.1.0.ResourceErrorsDetected",
        "ResourceEvent.1.0.ResourceWarningThresholdExceeded",
        "ResourceEvent.1.1.ResourceStateChanged",
        "Restart Fabric Manager or power cycle the BaseBoard. If problem persists, isolate the server for RMA evaluation.",
        "Retimer PWR_GD state run time change",
        "Row Remapping Count - Correctable",
        "Row Remapping Count - Uncorrectable",
        "Row Remapping Failure",
        "Row Remapping Failure Count",
        "Row Remapping Pending",
        "Row Remapping Pending Count",
        "Row-Remapping Failure",
        "Row-Remapping Pending",
        "RowRemappingFailureState",
        "RowRemappingPendingState",
        "SMBPBI Fencing state change",
        "SMBPBI Server Unavailable",
        "SMBPBIFencingState",
        "SMBPBIInterface",
        "SPI Flash Error",
        "SPI flash error",
        "SRAM ECC uncorrectable error count",
        "SYS VR 1V 8 Abnormal Power change",
        "SYS VR 3V3 Abnormal Power change",
        "Secure Boot Failure",
        "Secure boot failure",
        "SelfTest",
        "Server Unavailable",
        "System VR 1V8 Fault",
        "System VR 3V3 Fault",
        "THERM_OVERT",
        "Talk to ERoT directly via PLDM vendor command to triage.",
        "Temperature fault or warning has occurred",
        "The MOSFET is not switched on for any reason or hot swap gate is off",
        "Thermal Warning",
        "ThrottleReason",
        "Throttled State",
        "Training Error count is {NVLink Training Error}",
        "TrainingError",
        "Uncorrectable ECC Error",
        "Unknown fault or warning has occurred",
        "Upper Critical Temperature",
        "Upper Critical Temperature Threshold",
        "Upper Fatal Temperature",
        "VDD Abnormal Power Change",
        "VIN under voltage fault has occurred",
        "VR Failure",
        "VR Failure - 1V8 Abnormal Power Change",
        "VR Failure - 3V3 Abnormal Power Change",
        "VR Failure - 5V Abnormal Power Change",
        "VR Failure - DVDD Abnormal Power Change",
        "VR Failure - HVDD Abnormal Power Change",
        "VR Failure - VDD Abnormal Power Change",
        "VR Fault",
        "VR Fault 0.9V",
        "VR Fault 1.8V",
        "Value",
        "VendorId",
        "Warning",
        "WriteProtected",
        "active_auth_status GPU_SXM_[1-8]",
        "active_auth_status NVSwitch_[0-3]",
        "active_auth_status PCIeSwitch_0",
        "baseboard.pcb.temperature.alert",
        "cache_invalidation",
        "ceCount",
        "ceRowRemappingCount",
        "com.nvidia.MemoryRowRemapping",
        "com.nvidia.SMPBI",
        "erot_control",
        "false",
        "feCount",
        "firmware_status",
        "fpga.thermal.alert",
        "fpga.thermal.temperature.singlePrecision",
        "fpga_regtbl_wrapper",
        "gpu.interrupt.PresenceInfo",
        "gpu.interrupt.erot",
        "gpu.interrupt.powerGoodAbnormalChange",
        "gpu.thermal.alert",
        "gpu.thermal.temperature.overTemperatureInfo",
        "hmc.interrupt.erot",
        "hsc.device.alert",
        "hsc.power.abnormalPowerChange",
        "inletTemp.thermal.alert",
        "interface_status",
        "mctp-error-detection",
        "mctp-vdm-util-wrapper",
        "nonfeCount",
        "nvswitch.1V8.abnormalPowerChange",
        "nvswitch.3V3.abnormalPowerChange",
        "nvswitch.5V.abnormalPowerChange",
        "nvswitch.device.abnormalPresenceChange",
        "nvswitch.dvdd.abnormalPowerChange",
        "nvswitch.hvdd.abnormalPowerChange",
        "nvswitch.interrupt.erot",
        "nvswitch.thermal.alert",
        "nvswitch.thermal.temperature.overTemperatureInfo",
        "nvswitch.vdd.abnormalPowerChange",
        "pcie",
        "pcieretimer.0V9.abnormalPowerChange",
        "pcieretimer.1V8VDD.abnormalPowerChange",
        "pcieretimer.thermal.temperature.overTemperatureInfo",
        "pcieswitch.0V8.abnormalPowerChange",
        "pcieswitch.interrupt.erot",
        "pcieswitch.pcie.linkStatus.page6",
        "pcieswitch.thermal.temperature.overTemperatureInfo",
        "pin_status",
        "power_rail",
        "protocol_status",
        "range",
        "sysvr1v8.power.abnormalPowerChange",
        "sysvr3v3.power.abnormalPowerChange",
        "true",
        "ueCount",
        "ueRowRemappingCount",
        "xyz.openbmc_project.GpioStatus",
        "xyz.openbmc_project.GpuOobRecovery.Server",
        "xyz.openbmc_project.Inventory.Item.Cpu.OperatingConfig",
        "xyz.openbmc_project.Inventory.Item.PCIeDevice",
        "xyz.openbmc_project.Inventory.Item.PCIeDevice.PCIeTypes.Gen4",
        "xyz.openbmc_project.Inventory.Item.PCIeDevice.PCIeTypes.Gen5",
        "xyz.openbmc_project.Inventory.Item.PCIeDevice.PCIeTypes.Unknown",
        "xyz.openbmc_project.Inventory.Item.Port",
        "xyz.openbmc_project.Inventory.Item.Switch",
        "xyz.openbmc_project.Memory.MemoryECC",
        "xyz.openbmc_project.PCIe.LTSSMState",
        "xyz.openbmc_project.PCIe.PCIeECC",
        "xyz.openbmc_project.Sensor.Threshold.Critical",
        "xyz.openbmc_project.Sensor.Threshold.HardShutdown",
        "xyz.openbmc_project.Sensor.Value",
        "xyz.openbmc_project.Software.Settings",
        "xyz.openbmc_project.State.PowerChange",
        "xyz.openbmc_project.State.ProcessorPerformance",
        "xyz.openbmc_project.State.ProcessorPerformance.ThrottleReasons.None",
        "xyz.openbmc_project.State.ResetStatus",
        "{ERoTId} ERoT_Fatal",
        "{HMCId} Firmware",
        "{HMCId} SMBPBI",
        "{HSCId} Alert",
        "{HSCId} PowerGood",
        "{PCBTempId} Temperature",
        "{SysVRId} PowerGood",
        "{Throttle Reason}",
        "{UpperCritical Threshold}",
        "{UpperFatal Threshold}"};

    for (const auto& str : eventInfoStrings)
    {
        EXPECT_NO_THROW({
            DeviceIdPattern pat(str);
            EXPECT_EQ(pat.pattern(), str);
            auto domain = pat.domainVec();
            auto values = pat.valuesVec();
            EXPECT_EQ(domain.size(), values.size());
            unsigned dimDomainProd = 1;
            for (unsigned i = 0u; i < pat.dim(); ++i)
            {
                dimDomainProd *= pat.dimDomain(i).size();
            }
            EXPECT_EQ(domain.size(), dimDomainProd);
            unsigned i = 0;
            for (const auto& arg : pat.domain())
            {
                auto value = pat.eval(arg);
                EXPECT_EQ(domain.at(i), arg);
                EXPECT_EQ(values.at(i), value);
                EXPECT_TRUE(pat.matches(value));
                auto matchRes = pat.match(value);
                EXPECT_EQ(matchRes.size(), 1);
                EXPECT_EQ(matchRes.at(0), arg);
                i++;
            }
        });
    }
}

TEST(DeviceIdTest, PatternInputMapping_ZeroPattern)
{
    DeviceIdPattern ip("Critical");

    EXPECT_THAT(ip.domainVec(), ElementsAre(PatternIndex()));

    EXPECT_THAT(ip.values(), ElementsAre("Critical"));

    EXPECT_EQ(ip(6), "Critical");
    EXPECT_EQ(ip(1), "Critical");
    EXPECT_EQ(ip(5, 7), "Critical");
    EXPECT_EQ(ip(), "Critical");

    EXPECT_FALSE(ip.matches(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_4"));
    EXPECT_FALSE(ip.matches(""));
    EXPECT_TRUE(ip.matches("Critical"));

    EXPECT_THAT(ip.match("Critical"), ElementsAre(PatternIndex()));
    EXPECT_THAT(ip.match("foo/bar"), ElementsAre());

    EXPECT_TRUE(ip.isInjective());
    EXPECT_EQ(ip.dim(), 0);
}

// Helper functions ///////////////////////////////////////////////////////////

TEST(DeviceIdTest, calcInputIndexToBracketPoss)
{
    EXPECT_THAT(
        calcInputIndexToBracketPoss(std::vector<unsigned>({0, 4, 0, 1, 1})),
        UnorderedElementsAre(std::vector<unsigned>{0, 2},
                             std::vector<unsigned>{3, 4},
                             std::vector<unsigned>{}, std::vector<unsigned>{},
                             std::vector<unsigned>{1}));
    EXPECT_THAT(calcInputIndexToBracketPoss(std::vector<unsigned>()),
                UnorderedElementsAre());
}

TEST(DeviceIdTest, calcInputDomain)
{
    EXPECT_EQ(calcInputDomain({}, {}), PatternInputDomain());
    auto primes =
        syntax::BracketMap{{2, 1}, {3, 2}, {5, 3}, {7, 4}, {11, 5}, {13, 6}};
    EXPECT_EQ(calcInputDomain({0}, {primes}),
              PatternInputDomain(
                  std::vector<syntax::DeviceIndex>{2, 3, 5, 7, 11, 13}));
    auto primes1 =
        syntax::BracketMap{{3, 2}, {5, 3}, {7, 4}, {11, 5}, {13, 6}, {17, 7}};
    EXPECT_EQ(
        calcInputDomain({0, 1}, {primes, primes1}),
        PatternInputDomain(std::vector<syntax::DeviceIndex>{3, 5, 7, 11, 13}));
    auto odds = syntax::BracketMap{{1, 1}, {3, 2},  {5, 3},  {7, 4},
                                   {9, 5}, {11, 6}, {13, 7}, {15, 8}};
    auto evens = syntax::BracketMap{{2, 1},  {4, 2},  {6, 3},  {8, 4},
                                    {10, 5}, {12, 6}, {14, 7}, {16, 8}};
    EXPECT_EQ(
        calcInputDomain({0, 1}, {primes, odds}),
        PatternInputDomain(std::vector<syntax::DeviceIndex>{3, 5, 7, 11, 13}));
    EXPECT_EQ(calcInputDomain({0, 1}, {primes, evens}),
              PatternInputDomain(std::vector<syntax::DeviceIndex>{2}));
    EXPECT_EQ(calcInputDomain({0, 1}, {odds, evens}),
              PatternInputDomain(std::vector<syntax::DeviceIndex>()));
    EXPECT_EQ(calcInputDomain({0, 1, 2}, {odds, odds, odds}),
              PatternInputDomain(
                  std::vector<syntax::DeviceIndex>{1, 3, 5, 7, 9, 11, 13, 15}));
    EXPECT_ANY_THROW(calcInputDomain({0, 1, 2}, {odds}));
}

TEST(DeviceIdTest, calcInputDomains)
{
    auto primes =
        syntax::BracketMap{{2, 1}, {3, 2}, {5, 3}, {7, 4}, {11, 5}, {13, 6}};
    auto primes1 =
        syntax::BracketMap{{3, 2}, {5, 3}, {7, 4}, {11, 5}, {13, 6}, {17, 7}};
    auto odds = syntax::BracketMap{{1, 1}, {3, 2},  {5, 3},  {7, 4},
                                   {9, 5}, {11, 6}, {13, 7}, {15, 8}};
    auto evens = syntax::BracketMap{{2, 1},  {4, 2},  {6, 3},  {8, 4},
                                    {10, 5}, {12, 6}, {14, 7}, {16, 8}};
    // ""
    EXPECT_EQ(calcInputDomains({}, {}), std::vector<PatternInputDomain>({}));
    // "[0|<primes>]_[0|<primes1>]"
    EXPECT_EQ(calcInputDomains({{0, 1}}, {primes, primes}),
              std::vector<PatternInputDomain>({PatternInputDomain(
                  std::vector<syntax::DeviceIndex>({2, 3, 5, 7, 11, 13}))}));
    // "[0|<primes>]_[0|<primes1>]"
    EXPECT_EQ(calcInputDomains({{0, 1}}, {primes, primes1}),
              std::vector<PatternInputDomain>({PatternInputDomain(
                  std::vector<syntax::DeviceIndex>({3, 5, 7, 11, 13}))}));
    // "[0|<primes>]_[1|<odds>]_[2|<evens>]"
    EXPECT_EQ(calcInputDomains({{0}, {1}, {2}}, {primes, odds, evens}),
              std::vector<PatternInputDomain>(
                  {PatternInputDomain(
                       std::vector<syntax::DeviceIndex>{2, 3, 5, 7, 11, 13}),
                   PatternInputDomain(std::vector<syntax::DeviceIndex>{
                       1, 3, 5, 7, 9, 11, 13, 15}),
                   PatternInputDomain(std::vector<syntax::DeviceIndex>{
                       2, 4, 6, 8, 10, 12, 14, 16})}));
    // "[0|<primes>]_[2|<odds>]_[0|<evens>]"
    EXPECT_EQ(calcInputDomains({{0, 2}, {}, {1}}, {primes, odds, evens}),
              std::vector<PatternInputDomain>(
                  {PatternInputDomain(std::vector<syntax::DeviceIndex>{2}),
                   PatternInputDomain(),
                   PatternInputDomain(std::vector<syntax::DeviceIndex>{
                       1, 3, 5, 7, 9, 11, 13, 15})}));
}

namespace syntax
{

TEST(DeviceIdTest, parseNonNegativeInt)
{
    EXPECT_EQ(parseNonNegativeInt("123"sv), 123u);
    EXPECT_EQ(parseNonNegativeInt(std::string("0")), 0u);
    EXPECT_ANY_THROW(parseNonNegativeInt(""sv));
    EXPECT_ANY_THROW(parseNonNegativeInt("-12"sv));
    EXPECT_ANY_THROW(parseNonNegativeInt("-1.2"sv));
    EXPECT_ANY_THROW(parseNonNegativeInt(std::string("ab")));
    EXPECT_ANY_THROW(parseNonNegativeInt(std::string("  42")));
    EXPECT_ANY_THROW(parseNonNegativeInt(std::string("42  ")));
    EXPECT_ANY_THROW(parseNonNegativeInt(std::string("42othertrash")));
}

TEST(DeviceIdTest, BracketRange_parse)
{
    EXPECT_EQ(BracketRange::parse("123"sv), BracketRange(123, 123));
    EXPECT_EQ(BracketRange::parse(std::string("0")), BracketRange(0, 0));
    EXPECT_ANY_THROW(BracketRange::parse(""sv));
    EXPECT_ANY_THROW(BracketRange::parse("-12"sv));
    EXPECT_ANY_THROW(BracketRange::parse("-1.2"sv));
    EXPECT_ANY_THROW(BracketRange::parse(std::string("ab")));
    EXPECT_ANY_THROW(BracketRange::parse(std::string("  42")));
    EXPECT_ANY_THROW(BracketRange::parse(std::string("42  ")));
    EXPECT_ANY_THROW(BracketRange::parse(std::string("42othertrash")));

    EXPECT_EQ(BracketRange::parse("2-8"sv), BracketRange(2, 8));
    EXPECT_EQ(BracketRange::parse("0-10"sv), BracketRange(0, 10));
    EXPECT_EQ(BracketRange::parse("0"sv), BracketRange(0, 0));
    EXPECT_EQ(BracketRange::parse("5"sv), BracketRange(5, 5));
    EXPECT_EQ(BracketRange::parse("3-3"sv), BracketRange(3, 3));
    EXPECT_ANY_THROW(BracketRange::parse("0--5"sv));
    EXPECT_ANY_THROW(BracketRange::parse(""sv));
    EXPECT_ANY_THROW(BracketRange::parse("trash"sv));
    EXPECT_ANY_THROW(BracketRange::parse("0- 10"sv));
    EXPECT_ANY_THROW(BracketRange::parse("7-4"sv));
}

TEST(DeviceIdTest, BracketRangeMap_parse)
{
    EXPECT_EQ(BracketRangeMap::parse("123"sv),
              BracketRangeMap(BracketRange(123, 123), BracketRange(123, 123)));
    EXPECT_EQ(BracketRangeMap::parse(std::string("0")),
              BracketRangeMap(BracketRange(0, 0), BracketRange(0, 0)));
    EXPECT_ANY_THROW(BracketRangeMap::parse(""sv));
    EXPECT_ANY_THROW(BracketRangeMap::parse("-12"sv));
    EXPECT_ANY_THROW(BracketRangeMap::parse("-1.2"sv));
    EXPECT_ANY_THROW(BracketRangeMap::parse(std::string("ab")));
    EXPECT_ANY_THROW(BracketRangeMap::parse(std::string("  42")));
    EXPECT_ANY_THROW(BracketRangeMap::parse(std::string("42  ")));
    EXPECT_ANY_THROW(BracketRangeMap::parse(std::string("42othertrash")));

    EXPECT_EQ(BracketRangeMap::parse("2-8"sv),
              BracketRangeMap(BracketRange(2, 8), BracketRange(2, 8)));
    EXPECT_EQ(BracketRangeMap::parse("0-10"sv),
              BracketRangeMap(BracketRange(0, 10), BracketRange(0, 10)));
    EXPECT_EQ(BracketRangeMap::parse("0"sv),
              BracketRangeMap(BracketRange(0, 0), BracketRange(0, 0)));
    EXPECT_EQ(BracketRangeMap::parse("5"sv),
              BracketRangeMap(BracketRange(5, 5), BracketRange(5, 5)));
    EXPECT_EQ(BracketRangeMap::parse("3-3"sv),
              BracketRangeMap(BracketRange(3, 3), BracketRange(3, 3)));
    EXPECT_ANY_THROW(BracketRangeMap::parse("0--5"sv));
    EXPECT_ANY_THROW(BracketRangeMap::parse(""sv));
    EXPECT_ANY_THROW(BracketRangeMap::parse("trash"sv));
    EXPECT_ANY_THROW(BracketRangeMap::parse("0- 10"sv));
    EXPECT_ANY_THROW(BracketRangeMap::parse("7-4"sv));

    EXPECT_EQ(BracketRangeMap::parse("1"),
              BracketRangeMap(BracketRange(1, 1), BracketRange(1, 1)));
    EXPECT_EQ(BracketRangeMap::parse("0-3"sv),
              BracketRangeMap(BracketRange(0, 3), BracketRange(0, 3)));
    EXPECT_EQ(BracketRangeMap::parse("1-39"sv),
              BracketRangeMap(BracketRange(1, 39), BracketRange(1, 39)));
    EXPECT_EQ(BracketRangeMap::parse("0-3:0-3"sv),
              BracketRangeMap(BracketRange(0, 3), BracketRange(0, 3)));
    EXPECT_EQ(BracketRangeMap::parse(std::string("0-3:10")),
              BracketRangeMap(BracketRange(0, 3), BracketRange(10, 10)));
    EXPECT_EQ(BracketRangeMap::parse("0-7:1-8"sv),
              BracketRangeMap(BracketRange(0, 7), BracketRange(1, 8)));
    EXPECT_ANY_THROW(BracketRangeMap::parse("0-7:0-3"sv));
    EXPECT_ANY_THROW(BracketRangeMap::parse("0-7: 1-8"sv));
    EXPECT_ANY_THROW(BracketRangeMap::parse("0-7  :1-8"sv));
    EXPECT_ANY_THROW(BracketRangeMap::parse(" 0-7:1-8"sv));
    EXPECT_ANY_THROW(BracketRangeMap::parse("0-7:1-8 "sv));
    EXPECT_ANY_THROW(BracketRangeMap::parse("0-7::1-8"sv));
}

TEST(DeviceIdTest, parseBracketMap)
{
    EXPECT_EQ(
        parseBracketMap("1"),
        (BracketMap)BracketRangeMap(BracketRange(1, 1), BracketRange(1, 1)));
    EXPECT_EQ(
        parseBracketMap("0-3"sv),
        (BracketMap)BracketRangeMap(BracketRange(0, 3), BracketRange(0, 3)));
    EXPECT_EQ(
        parseBracketMap("1-39"sv),
        (BracketMap)BracketRangeMap(BracketRange(1, 39), BracketRange(1, 39)));
    EXPECT_EQ(
        parseBracketMap("0-3:0-3"sv),
        (BracketMap)BracketRangeMap(BracketRange(0, 3), BracketRange(0, 3)));
    EXPECT_EQ(
        parseBracketMap(std::string("0-3:10")),
        (BracketMap)BracketRangeMap(BracketRange(0, 3), BracketRange(10, 10)));
    EXPECT_EQ(
        parseBracketMap("0-7:1-8"sv),
        (BracketMap)BracketRangeMap(BracketRange(0, 7), BracketRange(1, 8)));
    EXPECT_ANY_THROW(parseBracketMap("0-7:0-3"sv));
    EXPECT_ANY_THROW(parseBracketMap("0-7: 1-8"sv));
    EXPECT_ANY_THROW(parseBracketMap("0-7  :1-8"sv));
    EXPECT_ANY_THROW(parseBracketMap(" 0-7:1-8"sv));
    EXPECT_ANY_THROW(parseBracketMap("0-7:1-8 "sv));
    EXPECT_ANY_THROW(parseBracketMap("0-7::1-8"sv));
    // marcinw:TODO: all the non-degenerated cases
}

TEST(DeviceIdTest, parseBracketMap_CommaSeparatedSeries)
{
    // Test comma-separated series of mappings: "0-1:0,2-3:1"
    // Should map: 0->0, 1->0, 2->1, 3->1
    BracketMap expected{{0, 0}, {1, 0}, {2, 1}, {3, 1}};
    EXPECT_EQ(parseBracketMap("0-1:0,2-3:1"sv), expected);

    // Test with more mappings: "0-1:5,2-3:6,4:7"
    // Should map: 0->5, 1->5, 2->6, 3->6, 4->7
    BracketMap expected2{{0, 5}, {1, 5}, {2, 6}, {3, 6}, {4, 7}};
    EXPECT_EQ(parseBracketMap("0-1:5,2-3:6,4:7"sv), expected2);

    // Test with single values: "0:10,1:11,2:12"
    // Should map: 0->10, 1->11, 2->12
    BracketMap expected3{{0, 10}, {1, 11}, {2, 12}};
    EXPECT_EQ(parseBracketMap("0:10,1:11,2:12"sv), expected3);
}

// ============================================================================
// Comprehensive tests for Type 1: BracketRange (Simple Index/Range)
// ============================================================================

TEST(DeviceIdTest, BracketRange_Construction)
{
    // Test single value construction
    BracketRange single(5, 5);
    EXPECT_EQ(single.left, 5);
    EXPECT_EQ(single.right, 5);
    EXPECT_EQ(single.size(), 1);

    // Test range construction
    BracketRange range(0, 7);
    EXPECT_EQ(range.left, 0);
    EXPECT_EQ(range.right, 7);
    EXPECT_EQ(range.size(), 8);

    // Test that invalid ranges throw
    EXPECT_ANY_THROW(BracketRange(7, 4)); // left > right should fail
}

TEST(DeviceIdTest, BracketRange_Iteration)
{
    // Test iteration over single value
    BracketRange single(5, 5);
    std::vector<unsigned> singleValues;
    for (auto val : single)
    {
        singleValues.push_back(val);
    }
    EXPECT_THAT(singleValues, ElementsAre(5));

    // Test iteration over range
    BracketRange range(3, 7);
    std::vector<unsigned> rangeValues;
    for (auto val : range)
    {
        rangeValues.push_back(val);
    }
    EXPECT_THAT(rangeValues, ElementsAre(3, 4, 5, 6, 7));

    // Test iteration over range 0-3
    BracketRange range0(0, 3);
    std::vector<unsigned> values0;
    for (auto val : range0)
    {
        values0.push_back(val);
    }
    EXPECT_THAT(values0, ElementsAre(0, 1, 2, 3));
}

TEST(DeviceIdTest, BracketRange_Equality)
{
    BracketRange range1(0, 7);
    BracketRange range2(0, 7);
    BracketRange range3(1, 8);
    BracketRange range4(0, 6);

    EXPECT_EQ(range1, range2);
    EXPECT_NE(range1, range3);
    EXPECT_NE(range1, range4);
}

// ============================================================================
// Comprehensive tests for Type 2: BracketRangeMap (Range-to-Range Mapping)
// ============================================================================

TEST(DeviceIdTest, BracketRangeMap_IdentityMapping)
{
    // Test identity mapping: [0-7] means 0->0, 1->1, ..., 7->7
    BracketRangeMap identity(BracketRange(0, 7), BracketRange(0, 7));
    BracketMap resultMap = identity;

    BracketMap expected{{0, 0}, {1, 1}, {2, 2}, {3, 3},
                        {4, 4}, {5, 5}, {6, 6}, {7, 7}};
    EXPECT_EQ(resultMap, expected);
}

TEST(DeviceIdTest, BracketRangeMap_OneToOneMapping)
{
    // Test 1-to-1 shifted mapping: [0-7:1-8] means 0->1, 1->2, ..., 7->8
    BracketRangeMap shifted(BracketRange(0, 7), BracketRange(1, 8));
    BracketMap resultMap = shifted;

    BracketMap expected{{0, 1}, {1, 2}, {2, 3}, {3, 4},
                        {4, 5}, {5, 6}, {6, 7}, {7, 8}};
    EXPECT_EQ(resultMap, expected);

    // Test another 1-to-1 mapping: [1-4:10-13]
    BracketRangeMap custom(BracketRange(1, 4), BracketRange(10, 13));
    BracketMap customMap = custom;

    BracketMap expected2{{1, 10}, {2, 11}, {3, 12}, {4, 13}};
    EXPECT_EQ(customMap, expected2);
}

TEST(DeviceIdTest, BracketRangeMap_ManyToOneMapping)
{
    // Test many-to-1 mapping: [0-7:2] means 0->2, 1->2, ..., 7->2
    BracketRangeMap manyToOne(BracketRange(0, 7), BracketRange(2, 2));
    BracketMap resultMap = manyToOne;

    BracketMap expected{{0, 2}, {1, 2}, {2, 2}, {3, 2},
                        {4, 2}, {5, 2}, {6, 2}, {7, 2}};
    EXPECT_EQ(resultMap, expected);

    // Test another many-to-1: [5-10:99]
    BracketRangeMap manyToOne2(BracketRange(5, 10), BracketRange(99, 99));
    BracketMap resultMap2 = manyToOne2;

    BracketMap expected2{{5, 99}, {6, 99}, {7, 99}, {8, 99}, {9, 99}, {10, 99}};
    EXPECT_EQ(resultMap2, expected2);
}

TEST(DeviceIdTest, BracketRangeMap_InvalidSizes)
{
    // Test that mismatched sizes throw (except when to.size() == 1)
    EXPECT_ANY_THROW(BracketRangeMap(BracketRange(0, 7), BracketRange(0, 5)));
    EXPECT_ANY_THROW(BracketRangeMap(BracketRange(0, 3), BracketRange(0, 7)));

    // These should work (to.size() == 1)
    EXPECT_NO_THROW(BracketRangeMap(BracketRange(0, 7), BracketRange(5, 5)));
    EXPECT_NO_THROW(
        BracketRangeMap(BracketRange(0, 100), BracketRange(42, 42)));
}

// ============================================================================
// Comprehensive tests for Type 4: IndexedBracketMap (Indexed Mapping)
// ============================================================================

TEST(DeviceIdTest, IndexedBracketMap_ExplicitPosition)
{
    // Test explicit input position: [0|1-8:0-7]
    BracketMap map{{1, 0}, {2, 1}, {3, 2}, {4, 3},
                   {5, 4}, {6, 5}, {7, 6}, {8, 7}};
    IndexedBracketMap indexed(0, std::move(map));

    EXPECT_FALSE(indexed.isImplicit());
    EXPECT_EQ(indexed.getInputPosition(), 0);

    BracketMap resultMap = indexed.map();
    BracketMap expected{{1, 0}, {2, 1}, {3, 2}, {4, 3},
                        {5, 4}, {6, 5}, {7, 6}, {8, 7}};
    EXPECT_EQ(resultMap, expected);
}

TEST(DeviceIdTest, IndexedBracketMap_ImplicitPosition)
{
    // Test implicit input position (will be filled in later)
    BracketMap map{{0, 0}, {1, 1}, {2, 2}};
    IndexedBracketMap indexed(std::move(map));

    EXPECT_TRUE(indexed.isImplicit());

    // Set the position explicitly
    indexed.setInputPosition(5);
    EXPECT_FALSE(indexed.isImplicit());
    EXPECT_EQ(indexed.getInputPosition(), 5);
}

TEST(DeviceIdTest, IndexedBracketMap_Parse)
{
    // Test parsing with explicit position: "0|1-8"
    auto indexed1 = IndexedBracketMap::parse("0|1-8"sv);
    EXPECT_FALSE(indexed1.isImplicit());
    EXPECT_EQ(indexed1.getInputPosition(), 0);

    // Test parsing with implicit position: "1-8"
    auto indexed2 = IndexedBracketMap::parse("1-8"sv);
    EXPECT_TRUE(indexed2.isImplicit());

    // Test parsing with explicit position and mapping: "2|0-3:10-13"
    auto indexed3 = IndexedBracketMap::parse("2|0-3:10-13"sv);
    EXPECT_FALSE(indexed3.isImplicit());
    EXPECT_EQ(indexed3.getInputPosition(), 2);

    BracketMap expected{{0, 10}, {1, 11}, {2, 12}, {3, 13}};
    EXPECT_EQ(indexed3.map(), expected);
}

TEST(DeviceIdTest, IndexedBracketMap_FillImplicitPositions)
{
    // Test automatic filling of implicit positions
    std::vector<IndexedBracketMap> mappings;

    BracketMap map1{{0, 0}};
    BracketMap map2{{1, 1}};
    BracketMap map3{{2, 2}};

    mappings.push_back(IndexedBracketMap(std::move(map1)));    // implicit -> 0
    mappings.push_back(IndexedBracketMap(2, std::move(map2))); // explicit 2
    mappings.push_back(IndexedBracketMap(std::move(map3)));    // implicit -> 2

    IndexedBracketMap::fillImplicitInputPositions(mappings);

    EXPECT_EQ(mappings[0].getInputPosition(), 0);
    EXPECT_EQ(mappings[1].getInputPosition(), 2);
    EXPECT_EQ(mappings[2].getInputPosition(), 2);
}

} // namespace syntax

TEST(DeviceIdTest, DeviceIdPattern_CommaSeparatedMappings)
{
    // Test the user's specific case: pattern with [0-1:0,2-3:1]
    // This should map input 0,1 -> output 0, and input 2,3 -> output 1
    DeviceIdPattern pat("Device_[0-1:0,2-3:1]");

    // Test domain - should contain all valid input indexes: 0, 1, 2, 3
    EXPECT_THAT(pat.domainVec(),
                UnorderedElementsAre(PatternIndex(0), PatternIndex(1),
                                     PatternIndex(2), PatternIndex(3)));

    // Test evaluation at each index
    EXPECT_EQ(pat.eval(PatternIndex(0)), "Device_0");
    EXPECT_EQ(pat.eval(PatternIndex(1)), "Device_0");
    EXPECT_EQ(pat.eval(PatternIndex(2)), "Device_1");
    EXPECT_EQ(pat.eval(PatternIndex(3)), "Device_1"); // User's specific case

    // Test values - should get all unique output strings
    EXPECT_THAT(pat.valuesVec(),
                ElementsAre("Device_0", "Device_0", "Device_1", "Device_1"));

    // Test matching
    EXPECT_TRUE(pat.matches("Device_0"));
    EXPECT_TRUE(pat.matches("Device_1"));
    EXPECT_FALSE(pat.matches("Device_2"));

    // Test match - should return multiple indexes for non-injective mapping
    EXPECT_THAT(pat.match("Device_0"),
                UnorderedElementsAre(PatternIndex(0), PatternIndex(1)));
    EXPECT_THAT(pat.match("Device_1"),
                UnorderedElementsAre(PatternIndex(2), PatternIndex(3)));
}

// ============================================================================
// Comprehensive tests for DeviceIdPattern with all mapping types
// ============================================================================

TEST(DeviceIdTest, DeviceIdPattern_SimpleRange)
{
    // Test pattern with simple range: GPU_SXM_[1-8]
    DeviceIdPattern pat("GPU_SXM_[1-8]");

    // Test dimension
    EXPECT_EQ(pat.dim(), 1);

    // Test domain
    EXPECT_THAT(pat.domainVec(),
                ElementsAre(PatternIndex(1), PatternIndex(2), PatternIndex(3),
                            PatternIndex(4), PatternIndex(5), PatternIndex(6),
                            PatternIndex(7), PatternIndex(8)));

    // Test evaluation
    EXPECT_EQ(pat.eval(PatternIndex(1)), "GPU_SXM_1");
    EXPECT_EQ(pat.eval(PatternIndex(5)), "GPU_SXM_5");
    EXPECT_EQ(pat.eval(PatternIndex(8)), "GPU_SXM_8");

    // Test values
    EXPECT_THAT(pat.valuesVec(),
                ElementsAre("GPU_SXM_1", "GPU_SXM_2", "GPU_SXM_3", "GPU_SXM_4",
                            "GPU_SXM_5", "GPU_SXM_6", "GPU_SXM_7",
                            "GPU_SXM_8"));

    // Test matching (injective)
    EXPECT_TRUE(pat.matches("GPU_SXM_5"));
    EXPECT_FALSE(pat.matches("GPU_SXM_0"));
    EXPECT_FALSE(pat.matches("GPU_SXM_9"));
    EXPECT_THAT(pat.match("GPU_SXM_5"), ElementsAre(PatternIndex(5)));

    // Test injectivity
    EXPECT_TRUE(pat.isInjective());
}

TEST(DeviceIdTest, DeviceIdPattern_ShiftedMapping)
{
    // Test 1-to-1 shifted mapping: [1-8:0-7]
    DeviceIdPattern pat("PCIeRetimer_[1-8:0-7]");

    EXPECT_EQ(pat.dim(), 1);

    // Domain is the input range: 1-8
    EXPECT_THAT(pat.domainVec(),
                ElementsAre(PatternIndex(1), PatternIndex(2), PatternIndex(3),
                            PatternIndex(4), PatternIndex(5), PatternIndex(6),
                            PatternIndex(7), PatternIndex(8)));

    // Values are the output range: 0-7
    EXPECT_THAT(pat.valuesVec(),
                ElementsAre("PCIeRetimer_0", "PCIeRetimer_1", "PCIeRetimer_2",
                            "PCIeRetimer_3", "PCIeRetimer_4", "PCIeRetimer_5",
                            "PCIeRetimer_6", "PCIeRetimer_7"));

    // Test evaluation: input 1 -> output 0, input 8 -> output 7
    EXPECT_EQ(pat.eval(PatternIndex(1)), "PCIeRetimer_0");
    EXPECT_EQ(pat.eval(PatternIndex(8)), "PCIeRetimer_7");
    EXPECT_EQ(pat.eval(PatternIndex(5)), "PCIeRetimer_4");

    // Test matching (still injective)
    EXPECT_TRUE(pat.isInjective());
    EXPECT_THAT(pat.match("PCIeRetimer_0"), ElementsAre(PatternIndex(1)));
    EXPECT_THAT(pat.match("PCIeRetimer_7"), ElementsAre(PatternIndex(8)));
}

TEST(DeviceIdTest, DeviceIdPattern_ManyToOneMapping)
{
    // Test many-to-1 mapping: [0-3:5]
    // Maps input 0,1,2,3 all to output 5
    DeviceIdPattern pat("HSC_[0-3:5]");

    EXPECT_EQ(pat.dim(), 1);

    // Domain contains all inputs
    EXPECT_THAT(pat.domainVec(), ElementsAre(PatternIndex(0), PatternIndex(1),
                                             PatternIndex(2), PatternIndex(3)));

    // All map to the same value
    EXPECT_EQ(pat.eval(PatternIndex(0)), "HSC_5");
    EXPECT_EQ(pat.eval(PatternIndex(1)), "HSC_5");
    EXPECT_EQ(pat.eval(PatternIndex(2)), "HSC_5");
    EXPECT_EQ(pat.eval(PatternIndex(3)), "HSC_5");

    // Values will have duplicates
    EXPECT_THAT(pat.valuesVec(),
                ElementsAre("HSC_5", "HSC_5", "HSC_5", "HSC_5"));

    // Test matching (non-injective) - should return all input indexes
    EXPECT_FALSE(pat.isInjective());
    EXPECT_THAT(pat.match("HSC_5"),
                UnorderedElementsAre(PatternIndex(0), PatternIndex(1),
                                     PatternIndex(2), PatternIndex(3)));
}

TEST(DeviceIdTest, DeviceIdPattern_MultiDimensionalSimple)
{
    // Test multi-dimensional pattern: NVSwitch_[0-3]/Port_[0-15]
    DeviceIdPattern pat("NVSwitch_[0-3]/Port_[0-15]");

    EXPECT_EQ(pat.dim(), 2);

    // Test domain size: 4 switches * 16 ports = 64 combinations
    EXPECT_EQ(pat.domain().size(), 64);

    // Test some evaluations
    EXPECT_EQ(pat.eval(PatternIndex(0, 0)), "NVSwitch_0/Port_0");
    EXPECT_EQ(pat.eval(PatternIndex(0, 15)), "NVSwitch_0/Port_15");
    EXPECT_EQ(pat.eval(PatternIndex(3, 7)), "NVSwitch_3/Port_7");

    // Test matching
    EXPECT_TRUE(pat.isInjective());
    EXPECT_THAT(pat.match("NVSwitch_2/Port_10"),
                ElementsAre(PatternIndex(2, 10)));
}

TEST(DeviceIdTest, DeviceIdPattern_MultiDimensionalWithShifting)
{
    // Test multi-dimensional with shifted mapping
    DeviceIdPattern pat("FPGA_[0|1-2:0-1]_Device_[1|3-5]");

    EXPECT_EQ(pat.dim(), 2);

    // Dimension 0: inputs 1,2 -> outputs 0,1
    // Dimension 1: inputs 3,4,5 -> outputs 3,4,5
    EXPECT_EQ(pat.dimDomain(0),
              PatternInputDomain(std::vector<unsigned>({1, 2})));
    EXPECT_EQ(pat.dimDomain(1),
              PatternInputDomain(std::vector<unsigned>({3, 4, 5})));

    // Test evaluation
    EXPECT_EQ(pat.eval(PatternIndex(1, 3)), "FPGA_0_Device_3");
    EXPECT_EQ(pat.eval(PatternIndex(2, 5)), "FPGA_1_Device_5");
}

TEST(DeviceIdTest, DeviceIdPattern_ExplicitIndexPositions)
{
    // Test pattern with explicit index positions that run in parallel
    // [0|0-1:0,2-3:2] and [0|0-1:1,2-3:3] both use input position 0
    // This creates: (0)->01, (1)->01, (2)->23, (3)->23
    DeviceIdPattern pat("FPGA_[0|0-1:0,2-3:2][0|0-1:1,2-3:3]");

    EXPECT_EQ(pat.dim(), 1); // Only one input dimension

    // Domain should have 4 values: 0, 1, 2, 3
    EXPECT_THAT(pat.domainVec(), ElementsAre(PatternIndex(0), PatternIndex(1),
                                             PatternIndex(2), PatternIndex(3)));

    // Test evaluations
    EXPECT_EQ(pat.eval(PatternIndex(0)), "FPGA_01");
    EXPECT_EQ(pat.eval(PatternIndex(1)), "FPGA_01");
    EXPECT_EQ(pat.eval(PatternIndex(2)), "FPGA_23");
    EXPECT_EQ(pat.eval(PatternIndex(3)), "FPGA_23");
}

TEST(DeviceIdTest, DeviceIdPattern_ComplexCommaSeparated)
{
    // Test complex pattern with multiple comma-separated ranges
    DeviceIdPattern pat("Switch_[0-1:10,2-3:20,4:30]");

    EXPECT_EQ(pat.dim(), 1);

    // Domain: 0,1,2,3,4
    EXPECT_THAT(pat.domainVec(),
                ElementsAre(PatternIndex(0), PatternIndex(1), PatternIndex(2),
                            PatternIndex(3), PatternIndex(4)));

    // Test mapping: 0,1 -> 10; 2,3 -> 20; 4 -> 30
    EXPECT_EQ(pat.eval(PatternIndex(0)), "Switch_10");
    EXPECT_EQ(pat.eval(PatternIndex(1)), "Switch_10");
    EXPECT_EQ(pat.eval(PatternIndex(2)), "Switch_20");
    EXPECT_EQ(pat.eval(PatternIndex(3)), "Switch_20");
    EXPECT_EQ(pat.eval(PatternIndex(4)), "Switch_30");

    // Non-injective
    EXPECT_FALSE(pat.isInjective());
    EXPECT_THAT(pat.match("Switch_10"),
                UnorderedElementsAre(PatternIndex(0), PatternIndex(1)));
    EXPECT_THAT(pat.match("Switch_20"),
                UnorderedElementsAre(PatternIndex(2), PatternIndex(3)));
    EXPECT_THAT(pat.match("Switch_30"), ElementsAre(PatternIndex(4)));
}

TEST(DeviceIdTest, DeviceIdPattern_EmptyPattern)
{
    // Test empty pattern (no brackets)
    DeviceIdPattern pat("Baseboard");

    EXPECT_EQ(pat.dim(), 0);

    // Domain should contain only empty PatternIndex
    EXPECT_THAT(pat.domainVec(), ElementsAre(PatternIndex()));

    // Evaluation with empty index
    EXPECT_EQ(pat.eval(PatternIndex()), "Baseboard");

    // Values
    EXPECT_THAT(pat.valuesVec(), ElementsAre("Baseboard"));

    // Matching
    EXPECT_TRUE(pat.matches("Baseboard"));
    EXPECT_FALSE(pat.matches("Other"));
    EXPECT_THAT(pat.match("Baseboard"), ElementsAre(PatternIndex()));
}

TEST(DeviceIdTest, DeviceIdPattern_SingleValue)
{
    // Test pattern with single specific value
    DeviceIdPattern pat("FPGA_[5]");

    EXPECT_EQ(pat.dim(), 1);

    // Domain contains only 5
    EXPECT_THAT(pat.domainVec(), ElementsAre(PatternIndex(5)));

    // Evaluation
    EXPECT_EQ(pat.eval(PatternIndex(5)), "FPGA_5");

    // Values
    EXPECT_THAT(pat.valuesVec(), ElementsAre("FPGA_5"));

    // Matching
    EXPECT_TRUE(pat.matches("FPGA_5"));
    EXPECT_FALSE(pat.matches("FPGA_4"));
    EXPECT_FALSE(pat.matches("FPGA_6"));
}

} // namespace device_id
