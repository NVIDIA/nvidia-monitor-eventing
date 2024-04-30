#!/usr/bin/env python

# Copyright (c) 2024, NVIDIA CORPORATION.  All rights reserved.
#
#  NVIDIA CORPORATION and its licensors retain all intellectual property
#  and proprietary rights in and to this software, related documentation
#  and any modifications thereto.  Any use, reproduction, disclosure or
#  distribution of this software and related documentation without an express
#  license agreement from NVIDIA CORPORATION is strictly prohibited.


import pandas as pd
import os
import pathlib
import re
import common
import pprint
import argparse
import logging
import pprint

logging.basicConfig(force = True, level=logging.DEBUG)

def readArgs():
    parser = argparse.ArgumentParser(
        description = ("Join wrapper tests results into 1 table"))
    common.addCsvOptions(parser)
    common.addInFileOption(parser, "all_tests")
    common.addOutFileOption(parser, "failing_wrappers")
    common.addOutFileOption(parser, "working_wrappers")
    return parser.parse_args()

# Row selection ###############################################################

def nonWorkingWrappers(df):
    return df.loc[df["exit_code"] != 0]

def noResults(cols):
    return [c for c in cols if c not in ["exit_code", "output"]]

def workingWrappers(df):
    return df.loc[df["exit_code"] == 0][
        noResults(df.columns)]

# Data aggregation ############################################################

callsResultsColumns = [
    "date",
    "machine",
    "comment",
    ".accessor.executable",
    ".accessor.arguments",
    "exit_code",
    "output"]

# Return a table conflating the calls with the same results
def callsResultsCount(df):
    return df.groupby(callsResultsColumns)[callsResultsColumns[0]].count().rename(
        "count")

perfStatsColumns = [
    "cpu_percent_unit",
    "elapsed_real_s",
    "total_system_s",
    "total_user_s",
    "context_switches_invol",
    "context_switches_vol"
]

def performanceStats(df):
    res = df.groupby(noResults(callsResultsColumns))[
        perfStatsColumns].agg(["mean", "count"]).round(5)[
            [
                (      "cpu_percent_unit",  "mean"),
                (        "elapsed_real_s",  "mean"),
                (        "total_system_s",  "mean"),
                (          "total_user_s",  "mean"),
                ("context_switches_invol",  "mean"),
                (  "context_switches_vol",  "mean"),
                (  "context_switches_vol", "count")
            ]
        ]
    res.columns=[
        "mean_cpu_percent_unit",
        "mean_elapsed_real_s",
        "mean_total_system_s",
        "mean_total_user_s",
        "mean_context_switches_invol",
        "mean_context_switches_vol",
        "count"
    ]
    return res

# Tables ######################################################################

def main():
    args = readArgs()

    with common.openInput(args.all_tests) as allTestsStream:
        allDf = pd.read_csv(allTestsStream,
                            sep = args.sep,
                            quotechar = args.quotechar,
                            encoding = args.encoding)
        with common.openOutput(args.failing_wrappers) as failingWrappersStream:
            callsResultsCount(nonWorkingWrappers(
                allDf)).to_csv(path_or_buf = failingWrappersStream,
                               sep = args.sep,
                               quotechar = args.quotechar,
                               encoding = args.encoding,
                               index = True)
        with common.openOutput(args.working_wrappers) as workingWrappersStrem:
            performanceStats(workingWrappers(
                allDf)).to_csv(path_or_buf = workingWrappersStrem,
                               sep = args.sep,
                               quotechar = args.quotechar,
                               encoding = args.encoding,
                               index = True)
    return 0

if __name__ == "__main__":
    main()
