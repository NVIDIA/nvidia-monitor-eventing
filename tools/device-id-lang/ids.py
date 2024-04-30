# Copyright (c) 2024, NVIDIA CORPORATION.  All rights reserved.
#
#  NVIDIA CORPORATION and its licensors retain all intellectual property
#  and proprietary rights in and to this software, related documentation
#  and any modifications thereto.  Any use, reproduction, disclosure or
#  distribution of this software and related documentation without an express
#  license agreement from NVIDIA CORPORATION is strictly prohibited.

import argparse
import lark
import pprint
import itertools as it
import more_itertools as mit
from functools import reduce
import re

###############################################################################
#                                    Syntax                                   #
###############################################################################

parser = lark.Lark(r"""

string_map: string? ( brack_map string? )*

brack_map          : "[" indexed_map "]"
indexed_map        : sum_map | proper_indexed_map
proper_indexed_map : number "|" sum_map
sum_map            : map ( "," map )*
map                : range | proper_map
proper_map         : range ":" range
range              : number | proper_range
proper_range       : number "-" number

number: /[0-9]+/
string: /[^[]+/""",
                   parser = "lalr",
                   start = "string_map")

def dictSum(ds, *, conflictStrategy = "error"):
    """Return a dict which a domain being a sum of domains of
    dictionaries in `ds'. If there are conflicts then use
    `conflictStrategy' to resolve them.

    "error": raise an error if conflict occurs (default)
    "first": favor the first mapping in the `ds' sequence
    "last": favor the last mapping in the `ds' sequence

    Preserve the order of keys: first all the keys of `d1' in the
    same order, then all the keys from `d2' left, in the same
    order as they appear in `d2', then `d3', and so on.
    """
    result = {}
    def conflictMsg(d, k):
        return (f"Key mapping conflict: " +
                f"‘{k}’: ‘{d[k]}’ vs "
                f"‘{k}’: ‘{result[k]}’")
    for d in ds:
        for k in d:
            if k not in result:
                result[k] = d[k]
            elif d[k] != result[k]:
                if conflictStrategy == "error":
                    raise Exception(conflictMsg(d, k))
                elif conflictStrategy == "last":
                    result[k] = d[k]
                elif conflictStrategy != "first":
                    raise Exception(
                        conflictMsg(d, k) +
                        f"and unrecognised ‘conflictStrategy’ " +
                        f"value: ‘{conflictStrategy}’")
    return result

def internalErr(tree):
    print(tree)
    print(tree.pretty())
    assert(False)

def sequencesDict(xs, ys):
    assert(len(ys) == 1 or len(xs) == len(ys))
    if len(ys) == 1:
        return {x : ys[0] for x in xs}
    else:
        return {x : ys[i] for i, x in enumerate(xs)}

def number__getVal(tree):
    assert(tree.data == "number")
    return int(tree.children[0].value)

def _range__values(tree: lark.Tree):
    if tree.data == "number":
        return [number__getVal(tree)]
    elif tree.data == "proper_range":
        return range(number__getVal(tree.children[0]),
                     number__getVal(tree.children[1]) + 1)
    else:
        internalErr(tree)

def _map__domain(tree: lark.Tree):
    if tree.data == "range":
        return _range__values(tree.children[0])
    elif tree.data == "proper_map":
        return _range__values(tree.children[0].children[0])
    else:
        internalErr(tree)

def _map__image(tree: lark.Tree):
    if tree.data == "range":
        return _map__domain(tree)
    elif tree.data == "proper_map":
        return _range__values(tree.children[1].children[0])
    else:
        internalErr(tree)

def map_mapping(tree: lark.Tree):
    assert(tree.data == "map")
    xs = _map__domain(tree.children[0])
    ys = _map__image(tree.children[0])
    return sequencesDict(xs, ys)

def sum_map__mapping(tree: lark.Tree):
    assert(tree.data == "sum_map")
    return dictSum([map_mapping(c) for c in tree.children],
                   conflictStrategy = "error")

def _indexed_map__getIndex(tree: lark.Tree):
    if tree.data == "sum_map":
        return None
    elif tree.data == "proper_indexed_map":
        # return tree.children[0].children[0].value
        return number__getVal(tree.children[0])
    else:
        internalErr(tree)

def _indexed_map__getMapping(tree: lark.Tree):
    if tree.data == "sum_map":
        return sum_map__mapping(tree)
    elif tree.data == "proper_indexed_map":
        if len(tree.children) >= 2:
            return sum_map__mapping(tree.children[1])
        else:
            return {}
    else:
        internalErr(tree)

def indexed_map__get(tree: lark.Tree):
    assert(tree.data == "indexed_map")
    return (_indexed_map__getIndex(tree.children[0]),
            _indexed_map__getMapping(tree.children[0]))

def string__getVal(tree: lark.Tree):
    assert(tree.data == "string")
    return tree.children[0].value

def string_map__getIds(pattern: str, tree: lark.Tree):
    assert(tree.data == "string_map")
    indexes = [i for i, c in enumerate(tree.children)
               if c.data == "brack_map"]
    mappings = [indexed_map__get(c.children[0])
                for c in tree.children
                if c.data == "brack_map"]
    def stringValOrEmpty(k):
        if k < len(tree.children) and tree.children[k].data == "string":
            return tree.children[k].children[0].value
        else:
            return ""
    strings = [stringValOrEmpty(0)] + [
        stringValOrEmpty(i+1) for i in indexes]
    return DeviceIdPattern(pattern, strings, mappings)

###############################################################################
#                                  Semantics                                  #
###############################################################################

def fillImplicitIndexes(partiallyIndexedMappings):
    return [(givenIndex if givenIndex is not None else autoIndex, mapping)
            for (autoIndex, (givenIndex, mapping))
            in zip(it.count(0), partiallyIndexedMappings)]

# `bracketsInputIndexes' for '[1|0-7]__[0|0-8]__[3|0-9]':
# [1, 0, 3]
def calcInputIndexToBracketPoss(bracketsInputIndexes: list):
    # For `i' being the index of a number in the input tuple, the
    # `result[i]' is the list of indexes the brackets in a
    # pattern where it should be evaluated.
    if bracketsInputIndexes:
        n = max(bracketsInputIndexes) + 1
        result = [[]] * n
        for k, mi in enumerate(bracketsInputIndexes):
            if result[mi] == []:
                result[mi] = [k]
            else:
                result[mi] += [k]
        return result
    else:
        return []

class DeviceIdPattern:
    # Fields:
    #
    # strings: list<str>
    #     length = K + 1
    # indexedMappings: list<pair<int,int→int>>
    #     length = K
    # domains: list<collection>
    #     length = N
    #
    # for
    # K: Minimal length of the input tuple
    # N: Number of brackets in the pattern

    def __init__(self, pattern, strings, partiallyIndexedMappings):
        assert(len(strings) == len(partiallyIndexedMappings) + 1)
        self.pattern = pattern
        self.strings = strings
        self.indexedMappings = fillImplicitIndexes(partiallyIndexedMappings)
        # inputIndexToBracketPoss: list<int|None>
        #     length = N
        inputIndexToBracketPoss = calcInputIndexToBracketPoss(
            [index for index, mapping in self.indexedMappings])
        # Build the domain for each input position. For positions
        # shared among more than one bracket take the set of
        # common elements. If the position is not used by any
        # bracket assume `[None]' domain.
        self.domains = [
            (sorted(reduce(lambda d1, d2: d1 & d2,
                           [self.indexedMappings[bracketPos][1].keys()
                            for bracketPos in bracketPositions]))
            if bracketPositions != []
            else [None])
            for bracketPositions in inputIndexToBracketPoss]

    def show(self):
        print(f"self.pattern:")
        pprint.pp(self.pattern)
        print(f"self.strings:")
        pprint.pp(self.strings)
        print(f"self.indexedMappings:")
        pprint.pp(self.indexedMappings)
        print("self.domains:")
        pprint.pp(self.domains)

    # 5 layers of data, 4 types of transition.
    # Pattern: 'ab_[1|4-6:5-7]/cd_[0|7-9:8-10]/ef__[0-3]'
    # Input:   (5, 8)
    #
    # (5, 8)
    # rawPatternInputToPatternInput
    # (5, 8, None)
    #
    # (5, 8, x)
    # patternInputToBracketsInput
    # (8, 5, x)
    #
    # (8, 5, x)
    # bracketsInputToBracketsOutput
    # (9, 6, x)
    #
    # (9, 6, 0)
    # bracketsOutputToStringOutput
    # 'ab_9/cd_6/ef_0'

    # tuple<(int|None)*> → tuple<(int|None){s}>
    # where s ≥ N
    def rawPatternInputToPatternInput(self, indxs):
        return self.rawInputToInput(indxs, len(self.domains))

    # tuple<(int|None)*> → tuple<(int|None){s}>
    # where s ≥ K
    def rawBracketInputToBracketInput(self, indxs):
        return self.rawInputToInput(indxs, len(self.indexedMappings))

    # tuple<(int|None)*> → tuple<(int|None){s}>
    # where s ≥ n
    def rawInputToInput(self, indxs, n):
        return indxs + tuple([None] * max(n - len(indxs), 0))


    # tuple<int{N}> → tuple<int{K}>
    def patternInputToBracketsInput(self, indxs):
        return tuple([indxs[inputIndex] for inputIndex, mapping
                      in self.indexedMappings])


    # tuple<(int|None){N}> → iterator<tuple<int{N}>>
    def expandPatternInput(self, indxs):
        return self.expandInput(indxs, self.domains)

    # tuple<(int|None){K}> → iterator<tuple<int{K}>>
    def expandBracketsInput(self, indxs):
        return self.expandInput(
            indxs,
            [self.domains[bracketIndex] for bracketIndex, mapping
             in self.indexedMappings])

    # tuple<(int|None){S}> → iterator<tuple<int{S}>>
    def expandInput(self, indxs, domains):
        return it.product(*[
            [index] if index is not None else domain
            for index, domain
            in zip(indxs, it.chain(domains, it.cycle([None])))])


    # tuple<(int|None)*> → list<tuple<int{K}>>
    def bracketsInputsFromRawPatternInput(self, rawPatternInput):
        return it.starmap(
            self.patternInputToBracketsInput,
            zip(self.expandPatternInput(self.rawPatternInputToPatternInput(
                rawPatternInput))))

    # tuple<(int|None)*> → list<tuple<int{K}>>
    def bracketsInputsFromRawBracketsInput(self, rawBracketInput):
        return self.expandBracketsInput(
            self.rawBracketInputToBracketInput(rawBracketInput))

    # bool → (tuple<(int|None)*> → list<tuple<int{K}>>)
    def getInputTransform(self, bracketInputOpt: bool):
        return (self.bracketsInputsFromRawBracketsInput
                if bracketInputOpt else
                self.bracketsInputsFromRawPatternInput)

    # tuple<int{K}> → tuple<int{K}>
    def bracketsInputToBracketsOutput(self, indxs):
        return tuple([
            m[i] for i, m
            in zip(indxs,
                   [mapping for index, mapping
                    in self.indexedMappings])])

    # iterator<tuple<int{K}>> → iterator<tuple<int{K}>>
    def bracketsInputsToBracketsOutputs(self, bracketsInputs):
        return it.starmap(
            self.bracketsInputToBracketsOutput, zip(bracketsInputs))

    # tuple<int{K}> → str
    def bracketsOutputToStringOutput(self, indxs):
        return "".join(mit.interleave_longest(
            self.strings, [str(ei) for ei in indxs]))

    # iterator<tuple<int{K}>> → iterator<str>
    def bracketsOutputsToStringOutputs(self, bracketOutputs):
        return it.starmap(
            self.bracketsOutputToStringOutput, zip(bracketOutputs))

    # bool → (iterator<tuple<int{K}>> → iterator<str>
    #         | iterator<tuple<int{K}>> → iterator<tuple<int{K}>>)
    def getOutputTransform(self, bracketOutputOpt):
        return ((lambda x: x)
                if bracketOutputOpt
                else self.bracketsOutputsToStringOutputs)

    # tuple<(int|None)*> → iterator<(str|tuple<int{K}>)>
    def values(self, args = (), bracketInput = False, bracketOutput = False):
        return self.getOutputTransform(bracketOutput)(
            self.bracketsInputsToBracketsOutputs(
                self.getInputTransform(bracketInput)(args)))

    # tuple<(int|None)*> → iterator<pair<tuple<int{N}|int{K}>,(str|tuple<int{K}>)>>
    def pairs(self, args = (), bracketInput = False, bracketOutput = False):
        # Arguments iterator must be dulicated if we want it to
        # feed both the arguments and the values sequences
        args1, args2 = it.tee(
            self.expandBracketsInput(
                self.rawBracketInputToBracketInput(args))
            if bracketInput else
            self.expandPatternInput(
                self.rawPatternInputToPatternInput(args)))
        return zip(
            args1,
            self.getOutputTransform(bracketOutput)(
                self.bracketsInputsToBracketsOutputs(
                    args2
                    if bracketInput else
                    it.starmap(self.patternInputToBracketsInput,
                               zip(args2)))))

    def invert(self):
        return InvDeviceIdPattern(self.pattern, self)

    # tuple<(int|None)*> → str | tuple<int{K}>
    def evalAt(self, args, bracketInput = False, bracketOutput = False):
        ys = list(it.islice(self.values(args, bracketInput, bracketOutput), 1))
        if len(ys) == 0:
            raise Exception(f"No values found for the argument ‘{args}’")
        elif len(ys) > 1:
            raise Exception(f"Multiple values found for the argument ‘{args}’")
        else:
            return ys[0]

def dictInv(d):
    """Return a dictionary which for every value in 'd' lists all the
keys that map to it"""
    result = {}
    for k, v in d.items():
        if v in result:
            result[v] += [k]
        else:
            result[v] = [k]
    return result

def getRegex(strings):
    return "".join(mit.interleave_longest(
        strings, it.repeat("([0-9]+)", len(strings) - 1)))


# Pattern: '[1|0-7]__[0|0-8]__[3|0-9]__[1|0-7]':
# `bracketsPatternInputPositions': [1, 0, 3, 1]
# result: [[1], [0,3], [], [2]]
def calcPatternPositionsToBracketPositons(bracketsPatternInputPositions):
    # For `i' being the index of a number in the input tuple, the
    # `result[i]' is the list of indexes the brackets in a
    # pattern where it should be evaluated.
    n = max(bracketsPatternInputPositions) + 1
    result = [[]] * n
    for k, i in enumerate(bracketsPatternInputPositions):
        if result[i] == []:
            result[i] = [k]
        else:
            result[i] += [k]
    return result

class InvDeviceIdPattern:
    def __init__(self, pattern, ids: DeviceIdPattern):
        self.pattern = pattern
        self.ids = ids
        self.regex = "^" + getRegex(ids.strings) + "$"
        self.patternInPosToBracketPos = calcPatternPositionsToBracketPositons(
            [index for index, mapping in ids.indexedMappings])
        self.invMappings = [
            dictInv({ x : tuple([ids.indexedMappings[bracketPos][1][x]
                                 for bracketPos in bracketPositions])
                      for x in ids.domains[i] })
            for i, bracketPositions
            in enumerate(self.patternInPosToBracketPos)]

    def show(self):
        print("self.pattern:")
        pprint.pp(self.pattern)
        print("self.ids.strings:")
        pprint.pp(self.ids.strings)
        print("self.regex:")
        pprint.pp(self.regex)
        print("self.patternInPosToBracketPos:")
        pprint.pp(self.patternInPosToBracketPos)
        print("self.invMappings:")
        pprint.pp(self.invMappings)

    def patternOutputToBracketsOutput(self, string):
        m = re.match(self.regex, string)
        if m:
            result = tuple([int(elem) for elem in m.groups()])
            assert(len(result) == len(self.ids.strings) - 1)
            return result
        else:
            raise Exception(
                f"The string ‘{string}’ doesn't match "
                f"the pattern ‘{self.pattern}’")

    def bracketOutputToPatternInput(self, indxs):
        return tuple([
            self.invMappings[i][tuple([indxs[k] for k in bracketPoss])]
            for i, bracketPoss
            in enumerate(self.patternInPosToBracketPos)])

    def bracketOutputToPatternInputs(self, indxs):
        return it.product(*self.bracketOutputToPatternInput(indxs))

    def getOutputTransform(self, bracketOutputOpt: bool):
        if bracketOutputOpt:
            raise Exception("To implement")
        else:
            return self.patternOutputToBracketsOutput

    def patternInputsToBracketsInputs(self, indxs):
        return it.starmap(self.ids.patternInputToBracketsInput,
                          zip(indxs))

    def getInputTransform(self, bracketInputOpt: bool):
        return (self.patternInputsToBracketsInputs
                if bracketInputOpt else
                (lambda x: x))

    def values(self, args, bracketInput = False, bracketOutput = False):
        return self.getInputTransform(bracketInput)(
            self.bracketOutputToPatternInputs(
                self.getOutputTransform(bracketOutput)(args)))


def toStr(v):
    if type(v) is tuple:
        return " ".join(["_" if elem is None else str(elem)
                         for elem in v])
    else:
        return str(v)

def parse(string):
    return string_map__getIds(string, parser.parse(string))

def main():
    args = readArgs()

    try:
        ids = parse(args.expr)
    except Exception as exc:
        print(f"Error during parsing:")
        print(exc)
        return 1

    if args.regex:
        print(getRegex(ids.strings))
    else:
        if args.invert:
            if len(args.arg) < 1:
                raise Exception(f"A single string 'arg' argument required")
            else:
                invIds = ids.invert()

                if args.map:
                    raise Exception("To implement")
                else:
                    values = invIds.values(
                        args.arg[0], args.brackets_in, args.brackets_out)

        else:
            rawInput = tuple([
                None if a == "_" else int(a)
                for a in args.arg])

            if args.map:
                pairs = ids.pairs(
                    rawInput, args.brackets_in, args.brackets_out)
            else:
                values = ids.values(
                    rawInput, args.brackets_in, args.brackets_out)

        if args.map:
            # expect 'pairs' to be defined
            if args.python_format:
                pprint.pp(dict(pairs))
            else:
                for arg, value in pairs:
                    print(f"{toStr(arg)}:\t{toStr(value)}")
        else:
            # expect 'values' to be defined
            if args.python_format:
                pprint.pp(list(values))
            else:
                for v in values:
                    print(toStr(v))

def readArgs():
    parser = argparse.ArgumentParser(
        description = (""))
    parser.add_argument(
        "--brackets-in",
        action = "store_true",
        help = (f"Use brackets input (integer tuples) "
                "instead of pattern input (integer tuples)"))
    parser.add_argument(
        "--brackets-out",
        action = "store_true",
        help = (f"Use brackets output (integer tuples) "
                "instead of pattern output (string)"))
    parser.add_argument(
        "--map",
        action = "store_true",
        help = (f"Print the full mapping (arguments and values)"))
    parser.add_argument(
        "--invert",
        action = "store_true",
        help = (f"Invert the function used"))
    parser.add_argument(
        "--python-format",
        action = "store_true",
        help = (f"If set print the output in a format "
                "parse'able by python interpreter. "
                "This implies loading the whole output "
                "into memory before"))
    parser.add_argument(
        "--regex",
        action = "store_true",
        help = (f"Print a regular expression matching the "
                "string values of the given pattern. "
                "All other options (and arg sequence) are ignored."))
    parser.add_argument(
        "expr",
        type = str,
        action = "store",
        help = (f"Id pattern to use as a function definition"))
    parser.add_argument(
        "arg",
        type = str,
        nargs = "*",
        action = "store",
        help = (
            f"Arguments to evaluate the function at. "
            "Can be from counter-domain if ‘--invert’ option given. "
            "Currying is used in evaluation of the result - "
            "the arguments not provided at the end are assumed to be "
            "running along the whole domain of the respective axis. "
            "Not providing any arguments will therefore print the function's "
            "whole image."))
    return parser.parse_args()

if __name__ == '__main__':
    main()
