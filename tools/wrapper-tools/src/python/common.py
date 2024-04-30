# Copyright (c) 2024, NVIDIA CORPORATION.  All rights reserved.
#
#  NVIDIA CORPORATION and its licensors retain all intellectual property
#  and proprietary rights in and to this software, related documentation
#  and any modifications thereto.  Any use, reproduction, disclosure or
#  distribution of this software and related documentation without an express
#  license agreement from NVIDIA CORPORATION is strictly prohibited.

import sys
import argparse

def addCsvOptions(parser):
    defaultEncoding = "utf-8"
    parser.add_argument(
        "--encoding",
        type = str,
        action = "store",
        default = defaultEncoding,
        help = (f"Encoding of the csv file" +
                f"Default: '{defaultEncoding}'"))
    defaultQuotechar = '"'
    parser.add_argument(
        "--quotechar",
        type = str,
        action = "store",
        default = defaultQuotechar,
        help = (f"The character to quote fields in csv file" +
                f"Default: '{defaultQuotechar}'"))
    defaultSep = ","
    parser.add_argument(
        "--sep",
        type = str,
        action = "store",
        default = defaultSep,
        help = (f"Separator of columns for csv file. " +
                f"Default: '{defaultSep}'"))
    defaultColumns = None
    parser.add_argument(
        "--columns",
        type = str,
        action = "store",
        nargs = "*",
        default = defaultColumns,
        help = (f"List of columns to reduce the output to. " +
                f" Default: all"))
    parser.add_argument(
        "--no_header",
        action = "store_true",
        help = (f"Whether to omit the header"))

def addInFileOption(parser, fileInArgName):
    parser.add_argument(
        fileInArgName,
        type = str,
        action = "store",
        nargs = "?",
        default = "-",
        help = (f"File path to read, or '-' for stdin. " +
                f"Default: '-'"))

def addOutFileOption(parser, fileOutArgName = "file_out"):
    parser.add_argument(
        fileOutArgName,
        type = str,
        action = "store",
        nargs = "?",
        default = "-",
        help = (f"File path to write, or '-' for stdout. " +
                f"Default: '-'"))

def csvFileProc(description = "",
                fileInArgName = "file_in",
                fileOutArgName = "file_out"):
    parser = argparse.ArgumentParser(
        description = description)
    addCsvOptions(parser)
    addInFileOption(parser, fileInArgName)
    addOutFileOption(parser, fileOutArgName)
    return parser.parse_args()

def openInput(filename, **rest):
    return (sys.stdin if filename == "-"
            else open(filename, "r", **rest))

def openOutput(filename, **rest):
    return (sys.stdout if filename == "-"
            else open(filename, "w", **rest))
