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

#include "printing_util.hpp"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include <array>
#include <charconv>
#include <iostream>
#include <list>
#include <map>
#include <ranges>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace device_id
{

///////////////////////////////////////////////////////////////////////////////
//                                Declarations                               //
///////////////////////////////////////////////////////////////////////////////

namespace syntax
{

// DeviceIndex ("1", "0", "10", ...) //////////////////////////////////////////

using DeviceIndex = unsigned;

// BracketRange ("1-8") ///////////////////////////////////////////////////////

struct BracketRange
{
    DeviceIndex left;
    DeviceIndex right;

    // marcinw:TODO: move
    BracketRange(const DeviceIndex& left, const DeviceIndex& right);

    bool operator==(const BracketRange& other) const = default;
    bool operator!=(const BracketRange& other) const = default;

    unsigned size() const
    {
        // no overflow because 'right >= left' if constructed
        return right - left + 1;
    }

    auto begin() const
    {
        return std::views::iota(left, right + 1).begin();
    }

    auto end() const
    {
        return std::views::iota(left, right + 1).end();
    }

    template <typename StringRange>
    static BracketRange parse(const StringRange& range);
};

// BracketMap ("1,4-8,0-7:1-8") ///////////////////////////////////////////////

using BracketMap = std::map<DeviceIndex, DeviceIndex>;

// BracketRangeMap (eg. "0-7:1-8") ////////////////////////////////////////////

struct BracketRangeMap
{
    BracketRange from;
    BracketRange to;

    /**
     * Ranges must be of the same length (eg. "0-7" and "1-8"), or the @c to
     * range must contain just 1 element (eg. "0-7" and "2"). Otherwise there is
     * too little information to define the mapping.
     */
    BracketRangeMap(BracketRange&& from, BracketRange&& to);

    bool operator==(const BracketRangeMap& other) const = default;
    bool operator!=(const BracketRangeMap& other) const = default;

    operator BracketMap() const;

    template <typename StringRange>
    static BracketRangeMap parse(const StringRange& range);
};

// IndexedBracketMap ("0|1,4-8,0-7:1-8") //////////////////////////////////////

struct IndexedBracketMap
{
    IndexedBracketMap(unsigned inputPosition, BracketMap&& indexMap) :
        inputPosition((int)inputPosition), indexMap(std::move(indexMap))
    {}

    IndexedBracketMap(BracketMap&& indexMap) :
        inputPosition(indexPosImplicit), indexMap(std::move(indexMap))
    {}

    bool operator==(const IndexedBracketMap& other) const = default;
    bool operator!=(const IndexedBracketMap& other) const = default;

    bool isImplicit() const;
    void setInputPosition(unsigned pos);
    int getInputPosition() const;
    BracketMap map() const;

    static const int indexPosImplicit;

    template <typename StringRange>
    static IndexedBracketMap parse(const StringRange& range);

    template <class CharT>
    friend std::basic_ostream<CharT>& operator<<(std::basic_ostream<CharT>& os,
                                                 const IndexedBracketMap& ibm);

    // marcinw:TODO:
    // ranges::input_range
    template <typename BracketMappingsRange>
    static void
        fillImplicitInputPositions(BracketMappingsRange& bracketMappings);

  private:
    int inputPosition;
    BracketMap indexMap;
};

} // namespace syntax

// PatternIndex ///////////////////////////////////////////////////////////////

/** @brief Class representing index argument of a device id pattern - a tuple of
 * integers.
 *
 * For example, for the pattern
 * "HGX_PCIeRetimer_[0-7]/Sensors/HGX_PCIeRetimer_[0-7]_Temp_0" the following
 * arguments make sense: '(0, 0)', '(0, 1)', '(0, 2)', ..., '(7, 6)', '(7, 7)'.
 *
 * The valid arguments for the pattern "GPU_SXM_[1-8]" are '(1)', '(2)', ...,
 *'(8)'.
 *
 * Patterns not containing any brackets, like "FPGA_0", are indexable by exactly
 *one argument - an empty tuple '()'.
 *
 * Objects of this class are intended to be used as immutable values. Default
 * assignment operators, copy constructors, etc are provided only not to upset
 * the typical C++ workflow.
 **/

class PatternIndex
{
  public:
    static const int unspecified;

    PatternIndex() : indexes()
    {}

    /** @brief Construct the index tuple from the given indexes sequence
     *
     * All @c Args types will be converted to @c unsigned.
     **/
    template <typename... Args>
    explicit PatternIndex(int arg0, Args... args) :
        indexes({arg0, static_cast<int>(args >= 0 ? args : unspecified)...})
    {
        normalize();
    }

    unsigned dim() const;
    int operator[](unsigned i) const;
    bool operator==(const PatternIndex& other) const;
    bool operator<(const PatternIndex& other) const;

    template <class CharT>
    friend std::basic_ostream<CharT>& operator<<(std::basic_ostream<CharT>& os,
                                                 const PatternIndex& pi);

    // marcinw:TODO: make private
    void set(unsigned i, int value);

    template <class CharT>
    static void printValueTo(int value, std::basic_ostream<CharT>& os);

  private:
    std::vector<int> indexes;

    void normalize();
};

// PatternInputDomain ////////////////////////////////////////////////////////

class PatternInputDomain
{
  public:
    PatternInputDomain();

    template <typename Range>
    PatternInputDomain(Range arguments);

    std::size_t size() const;
    int operator[](int i) const;
    bool operator==(const PatternInputDomain& other) const;

    bool isUnspecified() const;
    bool contains(int elem) const;

    template <class CharT>
    friend std::basic_ostream<CharT>& operator<<(std::basic_ostream<CharT>& os,
                                                 const PatternInputDomain& pim);

  public:
    bool _unspecified;
    // Values are sorted ascendingly
    std::vector<unsigned> _domain;
};

// CartesianProductRange //////////////////////////////////////////////////////

// marcinw:TODO: move into PatternIndex class, share 'ranges' with 'domains'
class CartesianProductRange
{
  public:
    explicit CartesianProductRange(
        const std::vector<PatternInputDomain>& ranges) :
        ranges(ranges)
    {}

    class iterator_t
    {
      public:
        using difference_type = int;
        using value_type = PatternIndex;
        using pointer = PatternIndex*;
        using reference = PatternIndex&;
        using iterator_category = std::input_iterator_tag;

        iterator_t(unsigned masterIndex,
                   const std::vector<PatternInputDomain>& ranges) :
            masterIndex(masterIndex),
            ranges(ranges)
        {}

        iterator_t& operator++();
        PatternIndex operator*();
        bool operator==(const iterator_t& other) const;
        bool operator!=(const iterator_t& other) const;

      private:
        unsigned masterIndex;
        const std::vector<PatternInputDomain>& ranges;
    };

    iterator_t begin() const;
    iterator_t end() const;
    iterator_t cbegin() const;
    iterator_t cend() const;
    unsigned size() const;

    using sentinel_t = iterator_t;
    using range_difference_t = int;
    using range_size_t = unsigned;
    using range_value_t = PatternIndex;
    using range_reference_t = PatternIndex&;
    using range_rvalue_reference_t = PatternIndex&&;

  private:
    std::vector<PatternInputDomain> ranges;

    static unsigned product(const std::vector<PatternInputDomain>& values);
};

// DeviceIdPattern ////////////////////////////////////////////////////////////

class DeviceIdPattern
{
  public:
    /**
     * @brief Construct an object representing the given device id pattern.
     *
     * All parsing is done during construction. Successful construction is
     * equivalent to @c devIdPattern representing a valid pattern in the device
     * id language. Otherwise some instance of @c std::runtime_error is thrown.
     **/
    // marcinw:TODO: make sure constructor is consistent with fields
    explicit DeviceIdPattern(const std::string& devIdPattern);

    /**
     * @brief Construct the pattern object corresponding to an empty string
     *
     * (Totally valid.)
     */
    // marcinw:TODO: make sure constructor is consistent with fields
    DeviceIdPattern() : DeviceIdPattern(std::string(""))
    {}

    /**
     * @brief Plug the numbers from @c into brackets occuring in the pattern.
     *
     * For example, the pattern  "NVSwitch_[0-3]/Ports/NVLink_[0-39]" evaluated
     * at '(1, 3)' is "NVSwitch_1/Ports/NVLink_3". The pattern
     * "HGX_GPU_SXM_[0-3:5-8,4-7:1-4]" evaluated at '(1)' is "HGX_GPU_SXM_6".
     *
     * Patterns with no brackets can be successfully evaluated only with an
     * empty index tuple.
     *
     * @param[in] arg An index tuple instantiating the pattern.
     *
     * The dimension of @c pi must not be lesser than the input dimension of the
     * pattern ('pi.dim() >= this->dim()'). Otherwise an instance of
     * std::domain_error is thrown. If it's greater the excessive indexes are
     * ignored.
     *
     * Every index in @pi must be in the corresponding domain
     * (dimDomain(i).contains(pi[i]) for all 0 <= i < dim()).
     * Otherwise an instance of std::domain is thrown.
     *
     * @return A pattern instantiated with the given index tuple as a newly
     * allocated string.
     *
     **/
    std::string eval(const PatternIndex& pi) const;

    template <typename... Args>
    std::string operator()(Args... args)
    {
        return eval(PatternIndex(args...));
    }

    /** @brief Return a list of all index tuples this pattern can be
     * successfully evaluated at.
     *
     * For example, the domain of "HGX_GPU_SXM_[0-3:5-8,4-7:1-4]" is '{(0), (1),
     * (2), (3), (4), (5), (6), (7)}'. The domain of "[1-2,7]_[51-52]" is '{(1,
     * 51), (1, 52), (2, 51), (2, 52), (7, 51), (7, 52)}'.
     *
     * The indexes are returned in natural order, with the rightmost values in
     * tuple being the least significant.
     *
     **/
    CartesianProductRange domain() const;
    std::vector<PatternIndex> domainVec() const;

    /**
     * @brief Return the range of all strings this pattern can be successfully
     * evaluated to.
     *
     * For example, the values of "HGX_GPU_SXM_[0-3:5-8,4-7:1-4]" are
     * {
     *     "HGX_GPU_SXM_5",
     *     "HGX_GPU_SXM_6",
     *     "HGX_GPU_SXM_7",
     *     "HGX_GPU_SXM_8",
     *     "HGX_GPU_SXM_1",
     *     "HGX_GPU_SXM_2",
     *     "HGX_GPU_SXM_3",
     *     "HGX_GPU_SXM_4"
     * }
     *
     * The values of "[1-2,7]_[51-52]" are
     * {
     *     "1_51",
     *     "1_52",
     *     "2_51",
     *     "2_52",
     *     "7_51",
     *     "7_52"
     * }
     *
     * The values are returned in the same order as their corresponding
     * arguments returned by @c domain() would be (such that for every pattern
     * @c p: p.eval(p.domain[i]) == p.values()[i] for all the relevant indexes
     * @c i)
     */
    // WARNING: the result type of 'values()' will change to iterator. Result of
    // 'valuesVec()' will stay as it is.
    std::vector<std::string> values() const;
    std::vector<std::string> valuesVec() const;

    /**
     * @brief Return true if @c belongs to pattern's values.
     *
     * For example "HGX_GPU_SXM_8" belongs to values of the pattern
     * "HGX_GPU_SXM_[0-3:5-8,4-7:1-4]", but "HGX_GPU_SXM_10" does not, and
     * neither "GPU_SXM_8".
     *
     * Note that "PCIeRetimer_8" DOESN'T belong to the values of
     * "PCIeRetimer_[1-8:0-7]" (although '8' belongs to its domain).
     *
     * This function returning @c true guarantees that the result of @c match
     * will contain at least one element.
     */
    bool matches(const std::string& str) const;

    /**
     * @brief Return the list of all arguments for which this pattern evaluated
     * at would yield @c str.
     *
     * In more formal terms return the set '{x: p.eval(x) == str}'.
     *
     * If 'p.matches(str)' is @c false then the result of 'p.match(str)' will
     * always be empty.
     *
     * For example, matching the string "NVSwitch_0 NVLink_13" against the
     * pattern "NVSwitch_[0-3] NVLink_[0-39]" will result in '{(0, 13)}'.
     * Matching "NVSwitch_100 NVLink_13" against the same pattern will result in
     * '{}', and so will matching "foo".
     *
     * Because non-injective mappings in brackets are allowed it's possible that
     * the returned list will be longer than 1. For example, to express the
     * association between switches and hot swap controllers, which is
     *
     *     NVSwitch_0 -> HSC_8
     *     NVSwitch_1 -> HSC_8
     *     NVSwitch_2 -> HSC_9
     *     NVSwitch_3 -> HSC_9
     *
     * a pattern like "HSC_[0-1:8,2-3:9]" can be used. Matching it against the
     * string "HSC_9" would then yield '{(2), (3)}'.
     *
     */
    std::vector<PatternIndex> match(const std::string& str) const;

    /**
     * @brief Return true if the mapping defined by this pattern is injective
     * (no two different arguments producing the same value)
     *
     * If @c isBijective() is @c true then the result of @c match will never
     * have more than one element.
     */
    bool isInjective() const;

    /**
     * @brief Return dimension of the domain, that is the minimum length of
     * index tuples the 'eval(...)' accepts.
     *
     * For example the dimension of pattern "NVSwitch_[0-3]/Ports/NVLink_[0-39]"
     * is 2. The dimension of pattern "ERoT_NVSwitch_[0-3]" is 1. The dimension
     * of
     *
     * "FPGA_NVSW_[0|0-1:0,2-3:2][0|0-1:1,2-3:3]_VDD1V8_PWR_EN NVSwitch_[0|0-3]"
     *
     * representing the mapping:
     *
     *     (0) ->  FPGA_NVSW_01_VDD1V8_PWR_EN NVSwitch_0
     *     (1) ->  FPGA_NVSW_01_VDD1V8_PWR_EN NVSwitch_1
     *     (2) ->  FPGA_NVSW_23_VDD1V8_PWR_EN NVSwitch_2
     *     (3) ->  FPGA_NVSW_23_VDD1V8_PWR_EN NVSwitch_3
     *
     * is also 1 (all the brackets run in parallel at input index 0).
     *
     * The dimension of "HMC" is 0.
     */
    unsigned dim() const;

    /**
     * @brief Return the list of values allowed at the corresponding input axis
     *
     * For example, the domain of pattern "NVSwitch_[0-3]/Ports/NVLink_[0-39]"
     * along the dimension 1 is '{0, 1, 2, ..., 39}'. The resulting set is
     * sorted ascendingly.
     */
    PatternInputDomain dimDomain(unsigned axis) const;

    template <class CharT>
    friend std::basic_ostream<CharT>& operator<<(std::basic_ostream<CharT>& os,
                                                 const DeviceIdPattern& dip);

    std::string pattern() const;

  public:
    /**
     * A primitve parser separating bracket contents and the texts
     * between them.
     *
     * "GPU_SXM_[1-8]/NVLink_[0-39]" ->
     * texts:           "GPU_SXM_", "/NVLink_", ""
     * bracketContents: "1-8", "0-39"
     */

    template <typename StringType>
    static void separateTextAndBracketContents(
        const std::string& str, std::vector<StringType>& texts,
        std::vector<StringType>& bracketContents);

    void checkIsInDomain(const PatternIndex& pi) const;

  private:
    std::string _rawPattern;
    std::vector<std::string> _patternNonBracketFragments;

    /** @brief Mapping of the bracket positions to input positions
     *
     * Eg. for "[0|9][0|9][|9][4|9]" that would be
     *
     * 0 -> 0
     * 1 -> 0
     * 2 -> 2
     * 3 -> 4
     *
     * because:
     *
     *   v    v    v   v      input positions: 0, 0, 2 (implied), 4
     * "[0|9][0|9][|9][4|9]"
     *  ^---^^---^^--^^---^
     *    0    1   2    3     bracket positions: 0, 1, 2, 3
     *
     * Constraints:
     * bracketPosToInputPos.size() + 1 == _patternNonBracketFragments.size()
     * _bracketPosToInputPos contains only indexes 0 <= i < dim()
     */
    std::vector<unsigned> _bracketPosToInputPos;

    /**
     * @brief Sets of values allowed at each input position
     */
    std::vector<PatternInputDomain> _patternInputDomains;

    /**
     * @brief Integer mapping for each of the bracket
     *
     * The domain is given by the bracket's input position. The values are
     * specified per each bracket.
     *
     * For example, given the pattern
     * "FPGA_SXM[0|1-8:0-7]_EROT_RECOV_L GPU_SXM_[0|1-8]" the
     * _bracketInputMappings data structure would be
     *
     * _bracketInputMappings = vector{
     *     [0]: map{ [1]: 0, [2]: 1, [3]: 2, [4]: 3,
     *               [5]: 4, [6]: 5, [7]: 6, [8]: 7 },
     *     [1]: map{ [1]: 1, [2]: 2, [3]: 3, [4]: 4,
     *               [5]: 5, [6]: 6, [7]: 7, [8]: 8 }
     * }
     *
     * Notice that the arguments for both mappings are the same (because both
     * refer to the same input position 0), but the values are different. This
     * allows for expressing shifted indexing.
     */
    std::vector<syntax::BracketMap> _bracketInputMappings;
};

// Helper functions ///////////////////////////////////////////////////////////

PatternInputDomain
    calcInputDomain(const std::vector<unsigned>& bracketPositions,
                    const std::vector<syntax::BracketMap>& allBracketMappings);

std::vector<PatternInputDomain> calcInputDomains(
    const std::vector<std::vector<unsigned>>& inputPosToBracketPos,
    const std::vector<syntax::BracketMap>& bracketMappings);

/**
 * @return Elements are sorted ascendingly
 */
std::vector<std::vector<unsigned>> calcInputIndexToBracketPoss(
    const std::vector<unsigned>& bracketPosToInputPos);

///////////////////////////////////////////////////////////////////////////////
//                              Implementations                              //
///////////////////////////////////////////////////////////////////////////////

namespace syntax
{

/** Parse the number and return as unsigned.
 *
 * "0", "7", "81"
 *
 * @c Range type must provide:
 *     data() -> const char*,
 *     size()
 */
template <typename StringRange>
unsigned parseNonNegativeInt(const StringRange& range)
{
    int value;
    auto rangeEnd = range.data() + range.size();
    auto [readEnd, errc]{std::from_chars(range.data(), rangeEnd, value)};
    if (errc == std::errc{})
    {
        if (readEnd == rangeEnd)
        {
            if (value >= 0)
            {
                return value;
            }
            else // ! value >= 0
            {
                throw std::runtime_error("parseNonNegativeInt: ! value >= 0");
            }
        }
        else
        {
            throw std::runtime_error(
                "parseNonNegativeInt: ! readEnd == rangeEnd");
        }
    }
    else // ! std::from_chars(s.begin(), s.end(), value).ec == std::errc{}
    {
        // marcinw:TODO: more concrete error type
        throw std::runtime_error(
            "parseNonNegativeInt: ! "
            "std::from_chars(range.begin(), range.end(), value).ec == std::errc{}");
    }
}

/** Divide the text by '-'. Pass the elements to
 * 'parseNonNegativeInt'. Construct the 'BracketRange' from it.
 *
 * Examples: "0-7", "0-39", "9", "10"
 *
 * @c Range type must provide:
 *     data() -> const char*,
 *     size()
 *     begin()
 *     end()
 */
template <typename StringRange>
BracketRange BracketRange::parse(const StringRange& range)
{
    std::vector<std::string_view> elements;
    boost::split(elements, range, boost::is_any_of("-"),
                 boost::token_compress_off);
    if (elements.size() == 1)
    {
        auto number = parseNonNegativeInt(elements[0]);
        return BracketRange(number, number);
    }
    else if (elements.size() == 2)
    {
        auto left = parseNonNegativeInt(elements[0]);
        auto right = parseNonNegativeInt(elements[1]);
        if (left <= right)
        {
            return BracketRange(left, right);
        }
        else // ! left <= right
        {
            // marcinw:TODO: error message
            throw std::runtime_error("BracketRange::parse: ! left <= right");
        }
    }
    else
    {
        // marcinw:TODO: error message
        throw std::runtime_error(
            "BracketRange::parse: ! elements.size() in {1, 2}");
    }
}

// BracketRangeMap ////////////////////////////////////////////////////////////

/** Divide the text by ':'. Pass the elements to 'BracketRange::parse'. If
 * only one element infer the other. Construct a bracket map,
 * return.
 *
 * Examples: "0-7:1-8", "9:10", "0-7", "0-39", "9", "10"
 */
template <typename StringRange>
BracketRangeMap BracketRangeMap::parse(const StringRange& range)
{
    std::vector<std::string_view> elements;
    boost::split(elements, range, boost::is_any_of(":"),
                 boost::token_compress_off);
    if (elements.size() == 1)
    {
        auto bracketRange = BracketRange::parse(elements[0]);
        auto bracketRangeCopy = bracketRange;
        return BracketRangeMap(std::move(bracketRange),
                               std::move(bracketRangeCopy));
    }
    else if (elements.size() == 2)
    {
        return BracketRangeMap(BracketRange::parse(elements[0]),
                               BracketRange::parse(elements[1]));
    }
    else
    {
        // marcinw:TODO: error message
        throw std::runtime_error(
            "BracketRangeMap: ! elements.size() in {1, 2}");
    }
}

// BracketMap /////////////////////////////////////////////////////////////////

/** Divide the text by ','. Pass every element to
 * 'BracketMap::parse'. 'dictSum' the results and return.
 *
 * Examples: "0-7:1-8,0-39", "1,4", "0-7:1-8", "9:10", "0-7",
 * "0-39", "9", "10"
 */
template <typename StringRange>
BracketMap parseBracketMap(const StringRange& range)
{
    std::vector<std::string_view> elements;
    boost::split(elements, range, boost::is_any_of(","),
                 boost::token_compress_off);
    // marcinw:TODO:
    if (elements.size() == 1)
    {
        return BracketRangeMap::parse(elements[0]);
    }
    else // ! elements.size() == 1
    {
        // marcinw:TODO: error message
        throw std::runtime_error(
            "parseBracketMap: ! series of mappings not supported yet");
    }
}

// IndexedBracketMap //////////////////////////////////////////////////////////

/** Divide the text by '|'. Extract what's on the left. Pass the
 * right to 'parseBracketMap'.
 *
 * Examples: "0|0-7:1-8,0-39", "1|1,4", "0-7:1-8", "9:10", "0-7",
 * "0-39", "9", "10"
 */
template <typename StringRange>
IndexedBracketMap IndexedBracketMap::parse(const StringRange& range)
{
    std::vector<std::string_view> elements;
    boost::split(elements, range, boost::is_any_of("|"),
                 boost::token_compress_off);
    if (elements.size() == 1)
    {
        return IndexedBracketMap(parseBracketMap(elements[0]));
    }
    else if (elements.size() == 2)
    {
        return IndexedBracketMap(parseNonNegativeInt(elements[0]),
                                 parseBracketMap(elements[1]));
    }
    else
    {
        // marcinw:TODO: error message
        throw std::runtime_error(
            "IndexedBracketMap::parse: ! elements.size() in {1,2}");
    }
}

// marcinw:TODO:
// ranges::input_range
template <typename BracketMappingsRange>
void IndexedBracketMap::fillImplicitInputPositions(
    BracketMappingsRange& bracketMappings)
{
    unsigned i = 0;
    for (IndexedBracketMap& bracketMapping : bracketMappings)
    {
        if (bracketMapping.isImplicit())
        {
            bracketMapping.setInputPosition(i);
        }
        ++i;
    }
}

template <class CharT>
std::basic_ostream<CharT>& operator<<(std::basic_ostream<CharT>& os,
                                      const IndexedBracketMap& ibm)
{
    using ::operator<<;
    os << "(" << ibm.inputPosition << ": " << ibm.indexMap << ")";
    return os;
}

} // namespace syntax

// PatternInputDomain /////////////////////////////////////////////////////////

template <typename Range>
PatternInputDomain::PatternInputDomain(Range arguments) :
    _unspecified(false), _domain(arguments.begin(), arguments.end())
{
    std::ranges::sort(_domain);
}

template <class CharT>
std::basic_ostream<CharT>& operator<<(std::basic_ostream<CharT>& os,
                                      const PatternInputDomain& pid)
{
    using ::operator<<;
    if (pid._unspecified)
    {
        os << "PatternInputDomain([unspecified])";
    }
    else // ! pid._unspecified
    {
        os << "PatternInputDomain(_domain = '" << pid._domain << "')";
    }
    return os;
}

// PatternIndex ///////////////////////////////////////////////////////////////

/** @brief Print to @c os the pattern index in a tuple form, like "(1, 5, 0)",
 * "(7)", "()".
 *
 * The @c os includes @c std::cout, @c std::cerr, @c std::stringstream.
 *
 * The last one is useful when using Simple Log framework:
 *
 * @code
 *   std::stringstream ss;
 *   ss << patternIndex;
 *   log_dbg("%s", ss.str().c_str());
 * @endcode
 **/

template <class CharT>
std::basic_ostream<CharT>& operator<<(std::basic_ostream<CharT>& os,
                                      const PatternIndex& pi)
{
    os << "(";
    for (unsigned i = 0; i < pi.indexes.size(); ++i)
    {
        if (i > 0u)
        {
            os << ", ";
        }
        PatternIndex::printValueTo(pi.indexes.at(i), os);
    }
    os << ")";
    return os;
}

template <class CharT>
void PatternIndex::printValueTo(int value, std::basic_ostream<CharT>& os)
{
    if (value == PatternIndex::unspecified)
    {
        os << "_";
    }
    else
    {
        os << value;
    }
}

// DeviceIdPattern ////////////////////////////////////////////////////////////

/**
 * @brief Divide the given string into "passive" and "active" elements
 *
 * Logic best explained by example:
 *
 * "FPGA_SXM[0|1-8:0-7]_EROT_RECOV_L GPU_SXM_[0|1-8]"
 *  ------------------------------------------------  str
 *  --------           ----------------------       . texts
 *           ---------                        -----   bracketContents
 *
 * The length of @c texts vector is always the length of bracketContents plus 1.
 * If a bracket is found at the ends of the @c str then the @c texts will
 * contain empty string as the last element. Similarly for the bracket at the
 * very beginning of @c str.
 *
 * The symbols '[' and ']' are stripped and don't make their way to neither @c
 * texts nor @c bracketContents.
 *
 * @param[in] str
 * @param[out] texts
 * @param[out] bracketContents
 */
template <typename StringType>
void DeviceIdPattern::separateTextAndBracketContents(
    const std::string& str, std::vector<StringType>& texts,
    std::vector<StringType>& bracketContents)
{
    std::vector<StringType> result;
    // TODO
    boost::algorithm::split(result, str, boost::algorithm::is_any_of("[]"));
    // in a properly structured pattern the number of elements should be odd
    if (result.size() % 2 == 1)
    {
        texts.push_back(result[0]);
        for (unsigned i = 1; i < result.size(); i += 2)
        {
            bracketContents.push_back(result[i]);
            texts.push_back(result[i + 1]);
        }
    }
    else
    {
        // marcinw:TODO: error message
        throw std::runtime_error("separateTextAndBracketContents: ! "
                                 "result.size() % 2 == 1");
    }
}

/** @brief Print to @c os the pattern object in its typical form, eg.
 * "GPU_SXM_[0|1-8]_DRAM_0".
 *
 * The @c os includes @c std::cout, @c std::cerr, @c std::stringstream.
 *
 * The last one is useful when using Simple Log framework:
 *
 * @code
 *   std::stringstream ss;
 *   ss << pattern;
 *   log_dbg("%s", ss.str().c_str());
 * @endcode
 **/
template <class CharT>
std::basic_ostream<CharT>& operator<<(std::basic_ostream<CharT>& os,
                                      const DeviceIdPattern& dip)
{
    os << dip._rawPattern;
    return os;
}

} // namespace device_id
