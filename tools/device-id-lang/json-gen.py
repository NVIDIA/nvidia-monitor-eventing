# Copyright (c) 2024, NVIDIA CORPORATION.  All rights reserved.
#
#  NVIDIA CORPORATION and its licensors retain all intellectual property
#  and proprietary rights in and to this software, related documentation
#  and any modifications thereto.  Any use, reproduction, disclosure or
#  distribution of this software and related documentation without an express
#  license agreement from NVIDIA CORPORATION is strictly prohibited.

import ids
import argparse
import pprint
import json
import sys

def getDevIdObj(string):
    try:
        return ids.parse(string)
    except Exception as exc:
        print(f"Error parsing pattern ‘{string}’")
        print(exc)
        return None

def transform_attrValue(js, context):
    res = transform(js, context)
    if len(res) == 1:
        return res[0]
    else:
        return list(res)

def transform_dict(js, context):
    result = {}
    for keyPattern, val in js.items():
        pairs = getDevIdObj(keyPattern).pairs(context)
        for keyContext, keyInstance in pairs:
            result[keyInstance] = transform_attrValue(
                val, keyContext)
    return (result,)

def transform_list(js, context):
    return ([x
             for tup in [transform(e, context) for e in js]
             for x in tup],)

def transform_str(js, context):
    return tuple(list(getDevIdObj(js).values(context)))

def transform_int(js, context):
    return (js,)

def transform_float(js, context):
    return (js,)

def transform_bool(js, context):
    return (js,)

def transform_None(js, context):
    return (js,)


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
def transform(js, context):
    if isinstance(js, dict):
        return transform_dict(js, context)
    elif isinstance(js, list):
        return transform_list(js, context)
    elif isinstance(js, str):
        return transform_str(js, context)
    elif isinstance(js, int):
        return transform_int(js, context)
    elif isinstance(js, float):
        return transform_float(js, context)
    elif isinstance(js, bool):
        return transform_bool(js, context)
    elif isinstance(js, type(None)):
        return transform_None(js, context)

def readArgs():
    parser = argparse.ArgumentParser(
        description = ("Expand json file based on the ids syntax"))
    # parser.add_argument(
    #     "json",
    #     type = str,
    #     action = "store",
    #     help = (f"Json file to process"))
    parser.add_argument(
        "json",
        type = str,
        action = "store",
        nargs = "?",
        default = "-",
        help = (f"Json file path to read or '-' for stdin. " +
                f"Default: '-'."))
    return parser.parse_args()

def openInput(filename, **rest):
    return (sys.stdin if filename == "-"
            else open(filename, "r", **rest))

def main():
    args = readArgs()
    # with open(args.json, "r") as jsonFile:
    with openInput(args.json) as jsonFile:
        js = json.load(jsonFile)
        results = transform(js, ())
        for result in results:
            # print("result:")
            # pprint.pp(result)
            json.dump(result, sys.stdout, indent = 4)
            print()
    return 0

if __name__ == "__main__":
    main()
