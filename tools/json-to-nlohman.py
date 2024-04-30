# Copyright (c) 2024, NVIDIA CORPORATION.  All rights reserved.
#
#  NVIDIA CORPORATION and its licensors retain all intellectual property
#  and proprietary rights in and to this software, related documentation
#  and any modifications thereto.  Any use, reproduction, disclosure or
#  distribution of this software and related documentation without an express
#  license agreement from NVIDIA CORPORATION is strictly prohibited.

import os
import pprint
import argparse
import pprint
import json
import sys
import re

global args

def escape(jsonVal):
    return re.sub(r'"', r'\"', jsonVal)

# | object        | dict  |
# | array         | list  |
# | string        | str   |
# | number (int)  | int   |
# | number (real) | float |
# | true          | True  |
# | false         | False |
# | null          | None  |

# json_type → tuple<json_type>
# for
# json_type = dict | list | str | int | float | bool | NoneType
def transform(js):
    if isinstance(js, dict):
        return transform([
            [k, v] for k, v in js.items() ])
    elif isinstance(js, list):
        return ("{" + (", " if len(js) <= 3 else ",\n").join([
            transform(el) for el in js]) + "}")
    elif isinstance(js, str):
        return f'"{escape(js)}"'
    elif isinstance(js, int):
        return f"{js}"
    elif isinstance(js, float):
        return f"{js}"
    elif isinstance(js, bool):
        return "true" if js else "false"
    elif isinstance(js, type(None)):
        return "nullptr"

def printElem(variable, keysPath, js):
    if isinstance(js, dict):
        for k, v in js.items():
            printElem(variable, keysPath + [escape(k)], v)
    elif isinstance(js, list):
        return printAssign(
            variable, keysPath,
            transform(js) if js != [] else "json::array()")
    elif isinstance(js, str):
        return printAssign(variable, keysPath, f'"{escape(js)}"')
    elif isinstance(js, int):
        return printAssign(variable, keysPath, js)
    elif isinstance(js, float):
        return printAssign(variable, keysPath, js)
    elif isinstance(js, bool):
        return printAssign(variable, keysPath, "true" if js else "false")
    elif isinstance(js, type(None)):
        return printAssign(variable, keysPath, "nullptr")

def printAssign(variable, keysPath, value):
    print(variable + "".join([f'["{key}"]' for key in keysPath]) +
          f" = {value};")


def readArgs():
    parser = argparse.ArgumentParser(
        description = (
            "Print the json file in the nlohman's json initializer format " +
            "(https://github.com/nlohmann/json#creating-json-objects-from-json-literals)"))
    parser.add_argument(
        "json",
        type = str,
        action = "store",
        nargs = "?",
        default = "-",
        help = (f"Json file path to read or '-' for stdin. " +
                f"Default: '-'."))
    default_var = None
    parser.add_argument(
        "--var",
        type = str,
        action = "store",
        default = default_var,
        help = (f"The name of the json variable holding the json object. " +
                "Specifying this argument causes the imperative construction format " +
                "(operations to perform on the given variable). "
                "Omitting it will result in declarative construction " +
                "(value to be assiged to a json-type variable). "
                f" Default: '{default_var}'"))
    return parser.parse_args()

def openInput(filename, **rest):
    return (sys.stdin if filename == "-"
            else open(filename, "r", **rest))

def main():
    args = readArgs()
    # with open(args.json, "r") as jsonFile:
    with openInput(args.json) as jsonFile:
        js = json.load(jsonFile)
        if args.var != None:
            printElem(args.var, [], js)
        else:
            print(transform(js))
    return 0

if __name__ == "__main__":
    main()
