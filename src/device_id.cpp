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
        throw std::runtime_error(
            "BracketRange: Invalid range [" + std::to_string(left) + "-" +
            std::to_string(right) +
            "] - left boundary must not exceed right boundary.");
    }
}

// BracketRangeMap ////////////////////////////////////////////////////////////

/**
 * @brief Construct a BracketRangeMap from two BracketRanges.
 *
 * This constructor creates a mapping from the @p from range to the @p to range.
 * Three types of mappings are supported:
 * 1. One-to-one mapping: Both ranges must have the same size
 *    Example: [0-7:1-8] maps 0->1, 1->2, ..., 7->8
 * 2. Many-to-one mapping: The @p to range must contain exactly one element
 *    Example: [0-7:2] maps 0->2, 1->2, ..., 7->2
 * 3. One-to-many mapping: The @p from range must contain exactly one element
 *    Example: [0:0-1] maps 0->0, 0->1
 *
 * @param[in] from The input range (domain of the mapping)
 * @param[in] to The output range (codomain of the mapping)
 *
 * @throws std::runtime_error if the ranges have incompatible sizes
 *         (i.e., from.size() != to.size() and to.size() != 1 and from.size() !=
 * 1)
 */
BracketRangeMap::BracketRangeMap(BracketRange&& from, BracketRange&& to) :
    from(std::move(from)), to(std::move(to))
{
    const auto fromSize = this->from.size();
    const auto toSize = this->to.size();

    if (!(fromSize == toSize || toSize == 1 || fromSize == 1))
    {
        throw std::runtime_error(
            "BracketRangeMap: Invalid range sizes - 'from' range has " +
            std::to_string(fromSize) + " element(s), 'to' range has " +
            std::to_string(toSize) +
            " element(s). Ranges must have equal sizes (for 1-to-1 mapping), "
            "'to' must have exactly 1 element (for many-to-1 mapping), or "
            "'from' must have exactly 1 element (for 1-to-many mapping).");
    }
}

/**
 * @brief Convert a BracketRangeMap to a BracketMap (explicit std::map).
 *
 * This conversion operator generates the explicit key-value mapping represented
 * by this BracketRangeMap. The behavior depends on the type of mapping:
 *
 * - If to.size() == 1 (many-to-one mapping):
 *   Maps all keys from the @c from range to the single value in @c to range.
 *   Example: [0-7:2] produces {0->2, 1->2, 2->2, ..., 7->2}
 *
 * - If to.size() == from.size() (one-to-one mapping):
 *   Maps each key in @c from to the corresponding value in @c to in order.
 *   Example: [0-7:1-8] produces {0->[1], 1->[2], 2->[3], ..., 7->[8]}
 *
 * - If from.size() == 1 (one-to-many mapping):
 *   Maps the single key to all values in the @c to range.
 *   Example: [0:0-1] produces {0->[0, 1]}
 *
 * @return BracketMap containing all the key-value pairs of the mapping
 *         (values are vectors, with single element for one-to-one/many-to-one,
 *          multiple elements for one-to-many)
 */
BracketRangeMap::operator BracketMap() const
{
    BracketMap result;

    // One-to-many mapping: single key maps to multiple values
    if (from.size() == 1 && to.size() > 1)
    {
        const auto& key = *from.begin();
        std::vector<DeviceIndex> values;
        for (const auto& value : to)
        {
            values.push_back(value);
        }
        result[key] = values;
    }
    // Many-to-one mapping: all keys map to the single value
    else if (to.size() == 1)
    {
        const auto& value = *to.begin();
        for (const auto& key : from)
        {
            result[key] = {value}; // Vector with single element
        }
    }
    // One-to-one mapping: parallel iteration over both ranges
    else
    {
        for (auto keysIt = from.begin(), valuesIt = to.begin();
             keysIt != from.end(); ++keysIt, ++valuesIt)
        {
            result[*keysIt] = {*valuesIt}; // Vector with single element
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

bool IndexedBracketMap::hasOneToMany() const
{
    // Check if any value vector has more than one element
    for (const auto& [key, values] : indexMap)
    {
        if (values.size() > 1)
        {
            return true;
        }
    }
    return false;
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

std::map<unsigned, unsigned> mappingsSum(
    const std::vector<std::map<unsigned, unsigned>>& maps)
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

PatternInputDomain::PatternInputDomain() : _unspecified(true), _domain() {}

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

    // Store all mappings (now BracketMap supports one-to-many natively with
    // vectors)
    for (const auto& ibm : indexedBracketMappings)
    {
        _bracketInputMappings.push_back(ibm.map());
    }
    // _bracketInputMappings = vector{
    //     [0]: map{ [1]: [0], [2]: [1], [3]: [2], [4]: [3],
    //               [5]: [4], [6]: [5], [7]: [6], [8]: [7] },
    //     [1]: map{ [1]: [1], [2]: [2], [3]: [3], [4]: [4],
    //               [5]: [5], [6]: [6], [7]: [7], [8]: [8] }
    // }
    // For one-to-many: [0]: map{ [0]: [0, 1], [1]: [2, 3] }

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
        auto inputValue = pi[inputPos];
        const auto& mapping = _bracketInputMappings.at(bracketPos);
        const auto& values = mapping.at(inputValue);
        // Use first value (for one-to-many, this is the first output)
        return values[0];
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

std::vector<std::string> DeviceIdPattern::evalAll(const PatternIndex& pi) const
{
    checkIsInDomain(pi);

    // Get all output values for each bracket (now stored as vectors in
    // BracketMap)
    std::vector<std::vector<unsigned>> bracketOutputs;
    bool hasOneToMany = false;

    for (unsigned bracketPos = 0; bracketPos < _bracketPosToInputPos.size();
         ++bracketPos)
    {
        auto inputPos = _bracketPosToInputPos[bracketPos];
        auto inputValue = pi[inputPos];
        const auto& mapping = _bracketInputMappings.at(bracketPos);
        const auto& values = mapping.at(inputValue);

        bracketOutputs.push_back(values);
        if (values.size() > 1)
        {
            hasOneToMany = true;
        }
    }

    // If no one-to-many mappings, return single result
    if (!hasOneToMany)
    {
        return {eval(pi)};
    }

    // Generate all combinations of bracket outputs
    std::vector<std::string> results;

    // Calculate total combinations
    unsigned totalCombinations = 1;
    for (const auto& outputs : bracketOutputs)
    {
        totalCombinations *= outputs.size();
    }

    // Generate each combination
    for (unsigned combo = 0; combo < totalCombinations; ++combo)
    {
        std::stringstream ss;
        ss << _patternNonBracketFragments[0];

        unsigned remainder = combo;
        for (unsigned i = 0; i < bracketOutputs.size(); ++i)
        {
            unsigned index = remainder % bracketOutputs[i].size();
            remainder /= bracketOutputs[i].size();
            ss << bracketOutputs[i][index];
            ss << _patternNonBracketFragments[i + 1];
        }

        results.push_back(ss.str());
    }

    return results;
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
    // Check if any bracket has one-to-many (one input -> multiple outputs)
    bool hasOneToMany = false;
    for (const auto& mapping : _bracketInputMappings)
    {
        for (const auto& entry : mapping)
        {
            if (entry.second.size() > 1)
            {
                hasOneToMany = true;
                break;
            }
        }
        if (hasOneToMany)
        {
            break;
        }
    }
    if (hasOneToMany)
    {
        // Return all unique outputs (order unspecified)
        std::set<std::string> uniqueValues;
        for (const auto& x : domain())
        {
            auto allOutputs = evalAll(x);
            for (const auto& s : allOutputs)
            {
                uniqueValues.insert(s);
            }
        }
        return std::vector<std::string>(uniqueValues.begin(),
                                        uniqueValues.end());
    }
    // One value per domain element, in domain order (preserves API: values[i]
    // == eval(domain()[i]))
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
    // For one-to-many patterns, str may match any output from evalAll(arg)
    std::vector<PatternIndex> res;
    for (const auto& arg : this->domain())
    {
        auto allOutputs = this->evalAll(arg);
        if (std::find(allOutputs.begin(), allOutputs.end(), str) !=
            allOutputs.end())
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
PatternInputDomain calcInputDomain(
    const std::vector<unsigned>& bracketPositions,
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
        std::erase_if(resultSet, [&allBracketMappings,
                                  &bracketPositions](unsigned elem) {
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
