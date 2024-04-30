# Copyright (c) 2024, NVIDIA CORPORATION.  All rights reserved.
#
#  NVIDIA CORPORATION and its licensors retain all intellectual property
#  and proprietary rights in and to this software, related documentation
#  and any modifications thereto.  Any use, reproduction, disclosure or
#  distribution of this software and related documentation without an express
#  license agreement from NVIDIA CORPORATION is strictly prohibited.

import ids
import pandas as pd
import pprint
import argparse

def readArgs():
    parser = argparse.ArgumentParser(
        description = ("Map devices id"))
    parser.add_argument(
        "config",
        type = str,
        action = "store",
        help = (f"Config file"))
    parser.add_argument(
        "--join",
        action = "store_true",
        help = (f"Join all the id maps into one"))
    parser.add_argument(
        "--no-table",
        action = "store_true",
        help = (f"Don't print the device id mapping table"))
    return parser.parse_args()

def getDevIdObj(string):
    try:
        return ids.getDevIdObj(string)
    except Exception as exc:
        print(f"Error parsing pattern ‘{string}’")
        print(exc)
        return None

def main():
    args = readArgs()
    configFile = pd.read_csv(args.config, sep = ';', quotechar = '"')
    if not args.no_table:
        print()
        print(configFile)
        print()
    maps = []
    for (i, row) in configFile.iterrows():
        internalDevId = getDevIdObj(row[0])
        if internalDevId is not None:
            externalDevId = getDevIdObj(row[1])
            if externalDevId is not None:
                idMap = { interId: externalDevId.evalAt(index)
                          for index, interId in internalDevId.pairs(()) }
                maps += [idMap]
    if args.join:
        pprint.pp(ids.dictSum(maps))
    else:
        for idMap in maps:
            pprint.pp(idMap)
            print()

    return 0

if __name__ == "__main__":
    main()
