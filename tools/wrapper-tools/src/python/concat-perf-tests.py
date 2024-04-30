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

logging.basicConfig(force = True, level=logging.DEBUG)

# → (date, hardware)
def extractInfo(fullFilePath :pathlib.Path):
    m = re.match("([0-9]{4}-[0-9]{2}-[0-9]{2})__(.*?)(__(.*))?$",
                 fullFilePath.stem)
    if m:
        return (m[1], m[2], m[4] if m[4] is not None else "-")
    else:
        raise Exception(
            f"Path ‘{fullFilePath}’ doesn't match the expected pattern " +
            "yyyy-mm-dd__hardware[__other-info].csv")

def readArgs():
    parser = argparse.ArgumentParser(
        description = ("Join wrapper tests results into 1 table"))
    common.addCsvOptions(parser)
    parser.add_argument(
        "results_dir",
        type = str,
        action = "store",
        help = (f"Directory containing the .csv results"))
    common.addOutFileOption(parser, "all_tests")
    return parser.parse_args()

columns = [".accessor.executable",
           ".accessor.arguments",
           "cpu_percent",
           "elapsed_real",
           "total_system_s",
           "total_user_s",
           "context_switches_invol",
           "context_switches_vol",
           "exit_code",
           "output"]

columnsFinal = ["date",
                "machine",
                "comment",
                ".accessor.executable",
                ".accessor.arguments",
                "cpu_percent",
                "cpu_percent_unit",
                "elapsed_real",
                "elapsed_real_s",
                "total_system_s",
                "total_user_s",
                "context_switches_invol",
                "context_switches_vol",
                "exit_code",
                "output"]

# time: string returned by `time' command, in the format like "0m 0.08s".
def parseTimeToSec(time):
    m = re.match("([.,0-9]+)m ([.,0-9]+)s", time)
    return float(m[1]) * 60 + float(m[2]) if m else math.nan

def parsePercentToUnit(string):
    m = re.match("([.,0-9]+)%", string)
    return float(m[1]) / 100 if m else math.nan

def main():
    args = readArgs()
    resultsDir = pathlib.Path(args.results_dir)
    if not resultsDir.exists():
        raise Exception(f"The results directory ‘{resultsDir}’ doesn't exist")
    csvFiles = resultsDir.glob("*.csv")
    logging.info(f"Concatenating files (dir: {args.results_dir}):")
    result = pd.DataFrame(columns = columns)
    for csvFile in csvFiles:
        date, device, comment = extractInfo(csvFile)
        logging.info(f"{csvFile.name}")
        df = pd.read_csv(csvFile,
                         sep = args.sep,
                         quotechar = args.quotechar)
        newColumns = pd.DataFrame(data = {
            "date": pd.Series(data=[date] * df.shape[0]),
            "machine": pd.Series(data=[device] * df.shape[0]),
            "comment": pd.Series(data=[comment] * df.shape[0]),
            "elapsed_real_s": df["elapsed_real"].map(parseTimeToSec),
            "cpu_percent_unit": df["cpu_percent"].map(parsePercentToUnit)
        })
        result = pd.concat([result, newColumns.join(df[columns])])
    result[columnsFinal].to_csv(args.all_tests,
                                sep = args.sep,
                                quotechar = args.quotechar,
                                index = False)
    return 0

if __name__ == "__main__":
    main()
