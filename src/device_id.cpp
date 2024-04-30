/**
 * Copyright (c) 2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#include "device_id.hpp"

#include "log.hpp"
#include "util.hpp"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include <algorithm>
#include <ranges>
#include <set>

namespace device_id
{

namespace syntax
{

// BracketRange ///////////////////////////////////////////////////////////////

// marcinw:TODO: move
BracketRange::BracketRange(const DeviceIndex& left, const DeviceIndex& right) :
    left(left), right(right)
{
    if (left > right)
    {
        // marcinw:TODO: error message
        throw std::runtime_error("BracketRange: ! left > right");
    }
}

// BracketRangeMap ////////////////////////////////////////////////////////////

BracketRangeMap::BracketRangeMap(BracketRange&& from, BracketRange&& to) :
    from(std::move(from)), to(std::move(to))
{
    if (!(from.size() == to.size() || to.size() == 1))
    {
        // marcinw:TODO: error message
        throw std::runtime_error("BracketRangeMap:"
                                 " from.size() == to.size() || to.size() == 1");
    }
}

BracketRangeMap::operator BracketMap() const
{
    BracketMap result;
    if (to.size() == 1)
    {
        const auto& value = *to.begin();
        for (const auto& key : from)
        {
            result[key] = value;
        }
    }
    else
    {
        for (auto keysIt = from.begin(), valuesIt = to.begin();
             keysIt != from.end(); ++keysIt, ++valuesIt)
        {
            result[*keysIt] = *valuesIt;
        }
    }
    return result;
}

// IndexedBracketMap //////////////////////////////////////////////////////////

const int IndexedBracketMap::indexPosImplicit = -1;

bool IndexedBracketMap::isImplicit() const
{
    return inputPosition == indexPosImplicit;
}
void IndexedBracketMap::setInputPosition(unsigned pos)
{
    inputPosition = pos;
}

int IndexedBracketMap::getInputPosition() const
{
    return inputPosition;
}

BracketMap IndexedBracketMap::map() const
{
    return indexMap;
}

} // namespace syntax

const int PatternIndex::unspecified = -1;

// PatternIndex ///////////////////////////////////////////////////////////////

unsigned PatternIndex::dim() const
{
    return indexes.size();
}

int PatternIndex::operator[](unsigned i) const
{
    return i < indexes.size() ? indexes.at(i) : unspecified;
}

bool PatternIndex::operator==(const PatternIndex& other) const
{
    return this == &other || this->indexes == other.indexes;
}

/**
 * Ordering of pattern indexes are analogous to the ordering of natural numbers
 * viewed from their positional encoding perspective.
 */
bool PatternIndex::operator<(const PatternIndex& other) const
{
    for (unsigned i = 0; i < std::max(this->dim(), other.dim()); ++i)
    {
        // "unspecified" as the negative number is lesser than any specified
        // value
        if ((*this)[i] < other[i])
        {
            return true;
        }
        else if ((*this)[i] > other[i])
        {
            return false;
        }
    }
    // If none of the digits on any position are neither lesser nor greater then
    // they're all equal, which means (*this) == other
    return false;
}

void PatternIndex::set(unsigned i, int value)
{
    int actualValue = value >= 0 ? value : unspecified;
    if (indexes.size() <= i)
    {
        if (actualValue != unspecified)
        {
            indexes.resize(i + 1, unspecified);
            indexes[i] = actualValue;
        }
    }
    else // indexes.size() > i
    {
        indexes[i] = actualValue;
        normalize();
    }
}

void PatternIndex::normalize()
{
    while (indexes.size() > 0 && indexes.back() < 0)
    {
        indexes.pop_back();
    }
    for (unsigned i = 0u; i < indexes.size(); ++i)
    {
        if (indexes.at(i) < 0)
        {
            indexes[i] = PatternIndex::unspecified;
        }
    }
}

// CartesianProductRange //////////////////////////////////////////////////////

PatternIndex CartesianProductRange::iterator_t::operator*()
{
    // masterIndex(k) =
    // res[0] * limits[1] * limits[2] * ... * limits[k-1] +
    // res[1] * limits[2] * ... * limits[k-1] +
    // ...
    // res[k-3] * limits[k-2] * limits[k-1] +
    // res[k-2] * limits[k-1] +
    // res[k-1]
    //
    // masterIndex(k) * limits[k] + res[k] =
    // res[0] * limits[1] * limits[2] * ... * limits[k-1] * limits[k] +
    // res[1] * limits[2] * ... * limits[k-1] * limits[k] +
    // ...
    // res[k-3] * limits[k-2] * limits[k-1] * limits[k] +
    // res[k-2] * limits[k-1] * limits[k] +
    // res[k-1] * limits[k]
    // res[k]
    //
    // masterIndex(k) * limits[k] + res[k] = masterIndex(k+1)
    // masterIndex(k-1) * limits[k-1] + res[k-1] = masterIndex(k)
    //
    // masterIndex(k) = masterIndex(k-1) * limits[k-1] + res[k-1]
    // masterIndex(0) = 0
    //
    // while:
    // 0 <= res[k-1] < limits[k-1]
    //
    // =>
    // res[k-1] = masterIndex(k) % limits[k-1]
    // masterIndex(k-1) = floor(masterIndex(k) / limits[k-1])

    PatternIndex result;
    unsigned mi = masterIndex;
    if (ranges.size() > 0)
    {
        for (int i = ranges.size() - 1; i >= 0; --i)
        {
            auto s = ranges[i].size();
            result.set(i, ranges[i][mi % s]);
            mi = mi / s;
        }
    }
    return result;
}

CartesianProductRange::iterator_t&
    CartesianProductRange::iterator_t::operator++()
{
    ++masterIndex;
    return *this;
}

bool CartesianProductRange::iterator_t::operator==(
    const iterator_t& other) const
{
    return this == &other || (&this->ranges == &other.ranges &&
                              this->masterIndex == other.masterIndex);
}

bool CartesianProductRange::iterator_t::operator!=(
    const iterator_t& other) const
{
    return !(*this == other);
}

CartesianProductRange::iterator_t CartesianProductRange::begin() const
{
    return cbegin();
}

CartesianProductRange::iterator_t CartesianProductRange::end() const
{
    return cend();
}

CartesianProductRange::iterator_t CartesianProductRange::cbegin() const
{
    return iterator_t(0, ranges);
}

CartesianProductRange::iterator_t CartesianProductRange::cend() const
{
    return iterator_t(size(), ranges);
}

unsigned CartesianProductRange::size() const
{
    return product(ranges);
}

unsigned CartesianProductRange::product(
    const std::vector<PatternInputDomain>& values)
{
    unsigned result = 1;
    for (const auto& elem : values)
    {
        result *= elem.size();
    }
    return result;
}

std::map<unsigned, unsigned>
    mappingsSum(const std::vector<std::map<unsigned, unsigned>>& maps)
{
    std::map<unsigned, unsigned> result;
    for (unsigned i = 0; i < maps.size(); ++i)
    {
        const auto& map = maps.at(i);
        for (const auto& [key, value] : map)
        {
            if (!result.contains(key))
            {
                result[key] = value;
            }
            else if (value != result.at(key))
            {
                std::stringstream ss;
                ss << "Key mapping conflict: '" << key << "' -> '" << value
                   << "' in map at position " << i << " vs '" << key << "' -> '"
                   << result.at(key) << "' in one of previous maps";
                throw std::runtime_error(ss.str().c_str());
            }
        }
    }
    return result;
}

// PatternInputDomain ////////////////////////////////////////////////////////

PatternInputDomain::PatternInputDomain() : _unspecified(true), _domain()
{}

std::size_t PatternInputDomain::size() const
{
    return _unspecified ? 1 : _domain.size();
}

int PatternInputDomain::operator[](int i) const
{
    return _unspecified ? PatternIndex::unspecified : _domain[i];
}

bool PatternInputDomain::operator==(const PatternInputDomain& other) const
{
    return this == &other || (this->_unspecified && other._unspecified) ||
           (!this->_unspecified && !other._unspecified &&
            this->_domain == other._domain);
    // As long as the domain is sorted (it is) the vector equality suffices
}

bool PatternInputDomain::isUnspecified() const
{
    return _unspecified;
}

bool PatternInputDomain::contains(int elem) const
{
    return _unspecified ||
           (elem >= 0 && std::find(_domain.cbegin(), _domain.cend(), elem) !=
                             _domain.cend());
}

// DeviceIdPattern ////////////////////////////////////////////////////////////

DeviceIdPattern::DeviceIdPattern(const std::string& devIdPattern) :
    _rawPattern(devIdPattern)
{
    // The data transformation flow is best explained by
    // following example execution for
    //
    // devIdPattern = "FPGA_SXM[0|1-8:0-7]_EROT_RECOV_L GPU_SXM_[0|1-8]"

    // _rawPattern = "FPGA_SXM[0|1-8:0-7]_EROT_RECOV_L GPU_SXM_[0|1-8]"
    using ::operator<<;
    std::vector<std::string> bracketContents;
    DeviceIdPattern::separateTextAndBracketContents(
        _rawPattern, _patternNonBracketFragments, bracketContents);
    // bracketContents = 'vector{ [0]: "0|1-8:0-7", [1]: "0|1-8" }'
    //
    // _patternNonBracketFragments = 'vector{ [0]: "FPGA_SXM", [1]:
    // "_EROT_RECOV_L GPU_SXM_", [2]: "" }'

    std::vector<syntax::IndexedBracketMap> indexedBracketMappings;
    for (const auto& bracketContent : bracketContents)
    {
        indexedBracketMappings.push_back(
            syntax::IndexedBracketMap::parse(bracketContent));
    }
    // indexedBracketMappings = vector{
    //     [0]: (-1: map{ [1]: 0, [2]: 1, [3]: 2, [4]: 3,
    //                    [5]: 4, [6]: 5, [7]: 6, [8]: 7 }),
    //     [1]: (0: map{ [1]: 1, [2]: 2, [3]: 3, [4]: 4,
    //                   [5]: 5, [6]: 6, [7]: 7, [8]: 8 })
    // }

    syntax::IndexedBracketMap::fillImplicitInputPositions(
        indexedBracketMappings);
    // indexedBracketMappings = vector{
    //     [0]: (0: map{ [1]: 0, [2]: 1, [3]: 2, [4]: 3,
    //                   [5]: 4, [6]: 5, [7]: 6, [8]: 7 }),
    //     [1]: (0: map{ [1]: 1, [2]: 2, [3]: 3, [4]: 4,
    //                   [5]: 5, [6]: 6, [7]: 7, [8]: 8 })
    // }

    // _bracketPosToInputPos = vector{}
    for (const auto& ibm : indexedBracketMappings)
    {
        _bracketPosToInputPos.push_back(ibm.getInputPosition());
    }
    // _bracketPosToInputPos = vector{ [0]: 0, [1]: 0 }

    for (const auto& ibm : indexedBracketMappings)
    {
        _bracketInputMappings.push_back(ibm.map());
    }
    // _bracketInputMappings = vector{
    //     [0]: map{ [1]: 0, [2]: 1, [3]: 2, [4]: 3,
    //               [5]: 4, [6]: 5, [7]: 6, [8]: 7 },
    //     [1]: map{ [1]: 1, [2]: 2, [3]: 3, [4]: 4,
    //               [5]: 5, [6]: 6, [7]: 7, [8]: 8 }
    // }

    // calcInputIndexToBracketPoss(_bracketPosToInputPos) =
    //     vector{ [0]: vector{ [0]: 0, [1]: 1 } }
    _patternInputDomains =
        calcInputDomains(calcInputIndexToBracketPoss(_bracketPosToInputPos),
                         _bracketInputMappings);
}

std::string DeviceIdPattern::eval(const PatternIndex& pi) const
{
    checkIsInDomain(pi);
    // Calculate the values of brackets for the given pattern index
    auto evalBracket = [this, &pi](auto bracketPos) {
        auto inputPos = _bracketPosToInputPos[bracketPos];
        return _bracketInputMappings.at(bracketPos).at(pi[inputPos]);
    };
    auto values = std::views::iota(0u, _bracketInputMappings.size()) |
                  std::views::transform(evalBracket);
    // Interleave the bracket results with static strings
    std::stringstream ss;
    ss << _patternNonBracketFragments[0];
    for (unsigned i = 0; i < _bracketPosToInputPos.size(); ++i)
    {
        ss << values[i];
        ss << _patternNonBracketFragments[i + 1];
    }
    return ss.str();
}

CartesianProductRange DeviceIdPattern::domain() const
{
    return CartesianProductRange(_patternInputDomains);
}

std::vector<PatternIndex> DeviceIdPattern::domainVec() const
{
    auto d = domain();
    return std::vector<PatternIndex>(d.begin(), d.end());
}

std::vector<std::string> DeviceIdPattern::values() const
{
    // marcinw:TODO: change implementation to return a range over values
    std::vector<std::string> result;
    for (const auto& x : domain())
    {
        result.push_back(eval(x));
    }
    return result;
}

std::vector<std::string> DeviceIdPattern::valuesVec() const
{
    auto v = values();
    return std::vector<std::string>(v.begin(), v.end());
}

bool DeviceIdPattern::matches(const std::string& str) const
{
    // marcinw:TODO: more efficient implementation
    return !match(str).empty();
    // auto values = this->values();
    // return !(std::find(values.cbegin(), values.cend(), str) ==
    //          values.cend());
}

std::vector<PatternIndex> DeviceIdPattern::match(const std::string& str) const
{
    // marcinw:TODO: more efficient implementation
    std::vector<PatternIndex> res;
    for (const auto& arg : this->domain())
    {
        if (this->eval(arg) == str)
        {
            res.push_back(arg);
        }
    }
    return res;
}

bool DeviceIdPattern::isInjective() const
{
    // marcinw:TODO: more efficient implementation
    auto d = domain();
    auto v = values();
    return std::set<PatternIndex>(d.cbegin(), d.cend()).size() ==
           std::set<std::string>(v.cbegin(), v.cend()).size();
}

unsigned DeviceIdPattern::dim() const
{
    return _patternInputDomains.size();
}

void DeviceIdPattern::checkIsInDomain(const PatternIndex& pi) const
{
    auto badPositions =
        std::views::iota(0u, dim()) |
        std::views::filter([this, &pi](unsigned patternInputIndex) {
            return !_patternInputDomains[patternInputIndex].contains(
                pi[patternInputIndex]);
        });
    if (!std::ranges::empty(badPositions))
    {
        std::stringstream ss;
        ss << "Given pattern index tuple '" << pi
           << "' contains indexes outside of pattern's domain at axes: ";
        std::copy(badPositions.begin(), badPositions.end(),
                  std::ostream_iterator<unsigned>(ss, ", "));
        ss << "(values ";
        for (const auto& pos : badPositions)
        {
            PatternIndex::printValueTo(pi[pos], ss);
            ss << ", ";
        }
        ss << ")";
        throw std::runtime_error(ss.str().c_str());
    }
}

std::string DeviceIdPattern::pattern() const
{
    return _rawPattern;
}

PatternInputDomain DeviceIdPattern::dimDomain(unsigned axis) const
{
    if (axis < dim())
    {
        return _patternInputDomains[axis];
    }
    else
    {
        return PatternInputDomain();
    }
}

// Helper functions ///////////////////////////////////////////////////////////

/**
 * @brief Calculate the domain of the pattern input at a given position
 *
 * The brackets in device id lang specify independent integer mappings. The
 * values for pattern evaluation aren't provided to the brackets directly,
 * however, but to the "input positions" with which the bracket mappings are
 * associated. From these input positions the pattern arguments are forwarded to
 * bracket mappings and translated there into what will finally replace the
 * bracket.
 *
 * If the input position is associated with just one bracket mapping then the
 * domain of this position is just the domain of the bracket mapping. However,
 * if the same input position is associated with multiple brackets then the
 * domain must be the cross section of the domains from all these mappings.
 *
 * This function calculates this cross section issuing warnings along the way if
 * there are some elements left (while it's not an error, the behavior of such
 * pattern may be surprising to the user).
 *
 * @param[in] inputPos
 *
 * @param[in] bracketPositions
 *
 * @param[in] allBracketMappings
 */
PatternInputDomain
    calcInputDomain(const std::vector<unsigned>& bracketPositions,
                    const std::vector<syntax::BracketMap>& allBracketMappings)
{
    if (bracketPositions.empty())
    {
        // If this input position is not used in any of the
        // brackets then it's unspecified
        return PatternInputDomain();
    }
    else // ! bracketPositions.empty()
    {
        // Calculate the intersection of the domains of all the mappings
        // corresponding to this input position. Start with the first set and
        // reduce it by every element which doesn't appear also in all other
        // bracket mapping domains.
        const auto& firstBracketArgs =
            allBracketMappings.at(bracketPositions.at(0)) | std::views::keys;
        std::set<unsigned> resultSet(firstBracketArgs.begin(),
                                     firstBracketArgs.end());
        std::erase_if(
            resultSet, [&allBracketMappings, &bracketPositions](unsigned elem) {
                return !std::ranges::all_of(
                    bracketPositions, [&allBracketMappings, elem](unsigned i) {
                        return allBracketMappings.at(i).contains(elem);
                    });
            });
        return PatternInputDomain(resultSet);
    }
}

/**
 * @brief Calculate input domains based on the bracket mappings
 *
 * Function extends the logic of 'calcInputDomain()' on all the possible input
 * positions, defined implicitly by the @c inputPosToBracketPos mapping.
 *
 * This may leave some input positions not associated with any bracket. The
 * domain of such input is unspecified ('isUnspecified()' returning true),
 * which, for all intents and purposes equals the full set of natural numbers -
 * whatever argument is passed at this position is accepted, but it doesn't
 * affect the evaluation of the pattern in any way.
 *
 * @param[in] inputPosToBracketPos Mapping of pattern input positions to the
 * positions of associated brackets
 *
 * @param[in] bracketMappings The mappings for each of the bracket in a pattern.
 */
std::vector<PatternInputDomain> calcInputDomains(
    const std::vector<std::vector<unsigned>>& inputPosToBracketPos,
    const std::vector<syntax::BracketMap>& bracketMappings)
{
    std::vector<PatternInputDomain> result(inputPosToBracketPos.size());
    for (unsigned inputPosition = 0;
         inputPosition < inputPosToBracketPos.size(); ++inputPosition)
    {
        result[inputPosition] = calcInputDomain(
            inputPosToBracketPos[inputPosition], bracketMappings);
    }
    return result;
}

std::vector<std::vector<unsigned>> calcInputIndexToBracketPoss(
    const std::vector<unsigned>& bracketPosToInputPos)
{
    if (bracketPosToInputPos.size() > 0)
    {
        std::vector<std::vector<unsigned>> result(
            *std::ranges::max_element(bracketPosToInputPos.cbegin(),
                                      bracketPosToInputPos.cend()) +
            1);
        for (unsigned i = 0; i < bracketPosToInputPos.size(); ++i)
        {
            result[bracketPosToInputPos[i]].push_back(i);
        }
        return result;
    }
    else // ! bracketPosToInputPos
    {
        return std::vector<std::vector<unsigned>>();
    }
}

} // namespace device_id
