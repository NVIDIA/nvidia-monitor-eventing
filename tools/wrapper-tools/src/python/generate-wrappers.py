#!/usr/bin/env python

# Copyright (c) 2024, NVIDIA CORPORATION.  All rights reserved.
#
#  NVIDIA CORPORATION and its licensors retain all intellectual property
#  and proprietary rights in and to this software, related documentation
#  and any modifications thereto.  Any use, reproduction, disclosure or
#  distribution of this software and related documentation without an express
#  license agreement from NVIDIA CORPORATION is strictly prohibited.


import pandas as pd
import re
from pprint import pprint
import shlex
import argparse
from enum import Enum, auto, unique
import logging
import pathlib
import common

logging.basicConfig(force = True, level=logging.DEBUG)

def nanToEmptyStr(value):
    return "" if pd.isna(value) else value

def nanToZero(value):
    return 0 if pd.isna(value) else value

class WrapperMockupDefs:

    class Columns(Enum):
        CMD     = (".accessor.executable",
                   "the name of the wrapper")
        PARAMS  = (".accessor.arguments",
                   "the arguments provided to the wrapper, " +
                   "joined with space")
        TIME    = ("time_ms",
                   f"ignored for now")
        OUTPUT  = ("output",
                   f"the standard output of the ‘{CMD[0]}’ for the " +
                   f"given ‘{PARAMS[0]}’")
        RETCODE = ("return_code",
                   f"the exit code of the ‘{CMD[0]}’ for the " +
                   f"given ‘{PARAMS[0]}’")

        def __init__(self, val, description):
            self.val = val
            self.description = description

        def fullDescription(self):
            return (f"‘{self.val}’ ({self.description})"
                    if self.description is not None else f"‘{self.val}’")

        @classmethod
        def columnsDescription(cls):
            return ', '.join([e.fullDescription() for e in list(cls)])

    # Fields:
    # - wrappersMapping
    #   { command_name →
    #       { space_joined_parameters →
    #         (execution_time, string_OUTPUT, exit_code) }
    #   }

    def __init__(self, testpoints):
        super().__init__()
        self.wrappersMapping = {}
        for (i, row) in testpoints.iterrows():
            if row[WrapperMockupDefs.Columns.CMD.val] not in self.wrappersMapping:
                self.wrappersMapping[row[WrapperMockupDefs.Columns.CMD.val]] = {}
            assert(not pd.isna(row[WrapperMockupDefs.Columns.PARAMS.val]))
            args = " ".join(shlex.split(row[WrapperMockupDefs.Columns.PARAMS.val]))
            assert(not pd.isna(row[WrapperMockupDefs.Columns.CMD.val]))
            self.wrappersMapping[row[WrapperMockupDefs.Columns.CMD.val]][args] = (
                # convert miliseconds to seconds
                str(float(nanToZero(row[WrapperMockupDefs.Columns.TIME.val])) / 1000),
                str(nanToEmptyStr(row[WrapperMockupDefs.Columns.OUTPUT.val])),
                str(nanToZero(row[WrapperMockupDefs.Columns.RETCODE.val])))


def bashFormStrValue(value :str):
    return f"'{value}'"

def bashFormStmt(expr):
    return f"{expr};"

def bashFormAssign(variable :str, value :str):
    return bashFormStmt(f"{variable}={value}")

def bashFormSequence(elements :list):
    return '\n'.join(elements)

def bashFormTest(expression :str):
    return f"[[ {expression} ]]"

def bashFormEqual(left :str, right :str):
    return f"{left} == {right}"

def bashFormVarExpand(varName :str):
    return "\"${" + varName + "}\""

def bashFormElif(elifTests, elifBodies):
    assert(len(elifTests) == len(elifBodies))
    return '\n'.join([f"""elif {test}; then
{body}"""
     for (test, body) in zip(elifTests, elifBodies)])

def bashFormIfElifElse(ifElifTests :list, ifElifBodies :list, elseBody :str):
    assert(len(ifElifTests) == len(ifElifBodies))
    if len(ifElifTests) == 0:
        return elseBody
    else:
        return f"""if {ifElifTests[0]}; then
{ifElifBodies[0]}
{bashFormElif(ifElifTests[1:], ifElifBodies[1:])}
else
{elseBody}
fi"""

def bashFormMapping(inputOutputDict, inputVar :str, outputVars :list,
                    defaultValues :list):
    """Return a bash code snippet which sets 'outputVars' variables
according to the dictionary 'inputOutputDict', whose values must
be iterables of length at least 'len(outputVars)', using the
value of 'inputVar' as a key and string-type equivalency '==' for
comparison. Set 'outputVars' to 'defaultValues' if what
'inputVar' holds isn't equal to any key in 'inputOutputDict'.

    """
    def assignments(values):
        return bashFormSequence(
            [bashFormAssign(variable, bashFormStrValue(value))
             for (variable, value) in zip(outputVars, values)])
    ifElseAssignmentss = [
        assignments(values)
        for values in inputOutputDict.values()]
    elseAssignments = assignments(defaultValues)
    return bashFormIfElifElse(
        [bashFormTest(bashFormEqual(bashFormVarExpand(inputVar),
                                    bashFormStrValue(value)))
             for value in inputOutputDict.keys()],
        ifElseAssignmentss,
        elseAssignments)

def bashFormMapScr(wrapperMapping, timeVar :str,
                   outputVar :str, returnCodeVar :str):
    return (
        """#!/usr/bin/env bash

""" + bashFormMapping(wrapperMapping,
                      "@",
                      [timeVar, outputVar, returnCodeVar],
                      [0, "", 1]) + f"""

sleep {bashFormVarExpand(timeVar)};
echo {bashFormVarExpand(outputVar)};
exit {bashFormVarExpand(returnCodeVar)};"""
    )


def parseArgs():
    parser = argparse.ArgumentParser(
        description = ("""Generate SMBPBI wrappers mockups"""))
    parser.add_argument(
        "mockup_definition",
        type = str,
        action = "store",
        help = (f"A path to a TSV file containing definition of the wrapper mockups. " +
                "Expecteed columns: " + WrapperMockupDefs.Columns.columnsDescription()))
    default_dir = "."
    parser.add_argument(
        "dir",
        type = str,
        action = "store",
        nargs = "?",
        default = default_dir,
        help = (f"The directory in which to place the ganerated wrapper mockups. " +
                f"Default: {default_dir}"))
    common.addCsvOptions(parser)
    return parser.parse_args()

def main():
    args = parseArgs()
    logging.info(f"Parsing ‘{args.mockup_definition}’")
    testpoints = pd.read_csv(
        args.mockup_definition,
        quotechar = args.quotechar,
        sep = args.sep)
    wrappersDefs = WrapperMockupDefs(testpoints).wrappersMapping
    for wrapperName in wrappersDefs:
        destWrapper = pathlib.Path(args.dir) / pathlib.Path(wrapperName)
        logging.info(f"Generating wrapper ‘{destWrapper}’")
        content = bashFormMapScr(wrappersDefs[wrapperName],
                                 "time", "result", "returncode")
        with open(destWrapper, "w") as destWrapperFile:
            print(content, file = destWrapperFile)
    return 0

if __name__ == "__main__":
    main()
