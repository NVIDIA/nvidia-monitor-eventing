# Copyright (c) 2024, NVIDIA CORPORATION.  All rights reserved.
#
#  NVIDIA CORPORATION and its licensors retain all intellectual property
#  and proprietary rights in and to this software, related documentation
#  and any modifications thereto.  Any use, reproduction, disclosure or
#  distribution of this software and related documentation without an express
#  license agreement from NVIDIA CORPORATION is strictly prohibited.

# Generate DAT json which includes only devices from the sub-dat
# of the full 'dat.json' rooted in a given device.

import argparse
import pprint
import json
import sys

def openInput(filename, **rest):
    return (sys.stdin if filename == "-"
            else open(filename, "r", **rest))

def openOutput(filename, **rest):
    return (sys.stdout if filename == "-"
            else open(filename, "w", **rest))

def readArgs():
    parser = argparse.ArgumentParser(
        description = (
            "Generate DAT json which includes "
            "only devices from the sub-dat of "
            "the full 'dat.json' rooted in a given device."
            "Useful for generating mockups for unit tests "
            "for DAT-related code"))
    parser.add_argument(
        "root_dev",
        type = str,
        action = "store",
        help = (f"A device identifier to be the root "
                "of the generated dat"))
    parser.add_argument(
        "dat_in",
        type = str,
        action = "store",
        nargs = "?",
        default = "-",
        help = (f"File path to read or '-' for stdin. " +
                f"Default: '-'."))
    parser.add_argument(
        "dat_out",
        type = str,
        action = "store",
        nargs = "?",
        default = "-",
        help = (f"File path to write, or '-' for stdout. " +
                f"Default: '-'"))
    parser.add_argument(
        "--testlayers",
        action = "store_true",
        help = (f"Interpret DAT based on the associations"
                "defined in the test layers instead of the"
                '"associations" property'))
    return parser.parse_args()

def getChildrenAssociation(devEntry):
    return devEntry["association"]

def getChildrenTestLayers(devEntry):
    return [child
            for layer in ["power_rail",
                          "erot_control",
                          "pin_status",
                          "interface_status",
                          "protocol_status",
                          "firmware_status"]
            for child in getChildrenTestLayer(devEntry[layer]) ]

def getChildrenTestLayer(layerEntry):
    return [acc["accessor"]["device_name"]
            for acc in layerEntry
            if acc["accessor"]["type"] == "DEVICE"]

def fixAssociations(devEntry, allDevices):
    return {
        "association" : list(set(devEntry["association"]) &
                              set(allDevices)),
        **{ k : v for (k, v) in devEntry.items() if k != "association"}
    }

def subDat(fullDat, rootDev, childrenFunc):
    fringe = [rootDev]
    subDatDevs = set(fringe)
    while fringe:
        dev = fringe.pop()
        fringe += childrenFunc(fullDat[dev])
        subDatDevs |= set([dev])
    result = {}
    for subDatDev in subDatDevs:
        result[subDatDev] = fixAssociations(fullDat[subDatDev], subDatDevs)
    return result

def main():
    args = readArgs()
    with openInput(args.dat_in) as dat_in:
        with openOutput(args.dat_out) as dat_out:
            dat = json.load(dat_in)
            childrenFunc = (getChildrenTestLayers
                            if args.testlayers else
                            getChildrenAssociation)
            json.dump(subDat(dat, args.root_dev, childrenFunc),
                      dat_out, sort_keys = False, indent = 2)

    return 0

if __name__ == "__main__":
    exit(main())
