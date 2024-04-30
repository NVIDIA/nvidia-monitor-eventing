#!/usr/bin/env python3

# Copyright (c) 2024, NVIDIA CORPORATION.  All rights reserved.
#
#  NVIDIA CORPORATION and its licensors retain all intellectual property
#  and proprietary rights in and to this software, related documentation
#  and any modifications thereto.  Any use, reproduction, disclosure or
#  distribution of this software and related documentation without an express
#  license agreement from NVIDIA CORPORATION is strictly prohibited.

import json
import csv

failTpCounter = 0
totalTpCounter = 0
statsDict = {}


mapDatToReportLayer = {
    "power_rail": "power-rail-status",
    "erot_control": "erot-control-status",
    "pin_status": "pin-status",
    "interface_status": "interface-status",
    "protocol_status": "protocol-status",
    "firmware_status": "firmware-status",
    "data_dump": "data-dump"
}


def show_help(usage):
    print(usage)


def update_stat_key(is_failed, read_failed, _key, _dict):
    if _key not in _dict:
        _dict[_key] = {}
        _dict[_key]["total"] = 0
        _dict[_key]["fail"] = 0
        _dict[_key]["read_fail"] = 0
        _dict[_key]["pass"] = 0

    _dict[_key]["total"] += 1

    if read_failed:
        _dict[_key]["read_fail"] += 1
    elif is_failed:
        _dict[_key]["fail"] += 1
    else:
        _dict[_key]["pass"] += 1


def handle_commandline_stats(is_failed, read_failed, executable_name):
    global statsDict
    name = "CMDLINE_" + executable_name
    update_stat_key(is_failed, read_failed, name, statsDict)


def handle_stats(datTp):
    global failTpCounter
    global totalTpCounter
    global statsDict
    is_failed = False
    read_failed = False

    if "result" in datTp and datTp["result"] == "Fail":
        is_failed = True

    if datTp["value"] == "Error - TP read failed.":
        read_failed = True

    totalTpCounter += 1
    if is_failed or read_failed:
        failTpCounter += 1

    accType = datTp["accessor"]["type"]

    update_stat_key(is_failed, read_failed, accType, statsDict)

    if datTp["accessor"]["type"] == "CMDLINE":
        handle_commandline_stats(
            is_failed, read_failed, datTp["accessor"]["executable"])


def handle_tp(datTp, repTp):
    # merge testpoint configuration (dat) with actual result (report) by adding
    # needed fields. After that - handle statistics for a summary.
    datTp["value"] = repTp["value"]
    if "result" in repTp:
        datTp["result"] = repTp["result"]
    handle_stats(datTp)


def handle_layer(datLayer, repLayer):
    # find matching testpoints in the layer and finally process them
    for datTp in datLayer:
        for repTp in repLayer:
            if datTp["name"] == repTp["name"]:
                handle_tp(datTp, repTp)
                break


def handle_device(datDev, repDev):
    # find corresponding test layers in dat and report
    for datLayer, reportLayer in mapDatToReportLayer.items():
        if datLayer in datDev:  # its possible that some devices miss 'data dump' layer
            handle_layer(datDev[datLayer], repDev[reportLayer]["test-points"])


def merge_dat_and_report(datJsonPath, reportJsonPath, mergedJsonPath):
    print("opening dat.json path: ", datJsonPath)
    print("opening selftest_report.json path: ", reportJsonPath)
    fdat = open(datJsonPath, "rt")
    frep = open(reportJsonPath, "rt")
    print("creating merged json path: ", mergedJsonPath)
    fm = open(mergedJsonPath, "wt")

    jdat = json.loads(fdat.read())
    jrep = json.loads(frep.read())
    fdat.close()
    frep.close()

    # iterate over dat.json configured devices, find each corresponding
    # device in selftest report and process them
    for x in jdat:
        for y in jrep["tests"]:
            if y["device-name"] == x:
                handle_device(jdat[x], y)
                break  # break if found, theyre unique

    jmerged = jdat
    fm.write(json.dumps(jmerged, indent=2))
    fm.close()


def print_summary():
    print("fail counter = ", failTpCounter)
    print("total counter = ", totalTpCounter)
    print("stats dict = ", statsDict)


def write_summary_to_csv(_dict, csv_name):
    header = ['tp accessor type', 'total cnt',
              'fail cnt', 'read fail cnt', 'pass cnt']
    data = []

    for accType, accStats in _dict.items():
        data.append([accType, accStats["total"], accStats["fail"],
                    accStats["read_fail"], accStats["pass"]])

    with open(csv_name, 'w', newline='') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(header)
        writer.writerows(data)


def main():
    datJsonPath = "dat.json"
    reportJsonPath = "selftest_report.json"
    mergedJsonPath = "merged.json"
    csvFilePath = "summary.csv"
    usage = "\
        usage: if you need to change input files then edit the script, input argument not supported.\n\
        This script expects DAT file (" + datJsonPath + ") and selftest report (" + reportJsonPath + ") in the same directory as an input.\n\
        Output of the script is created new file " + mergedJsonPath + " also in the same directory which merges \n\
        dat configuration and report results in one file for convenient review.\n\
        Also csv file with testpoints summary is created (" + csvFilePath + ")\n\
        note: provided report file must have been created using provided dat to ensure consistency"
    # change default path is needed in code, no args parsing provided atm

    show_help(usage)
    merge_dat_and_report(datJsonPath, reportJsonPath, mergedJsonPath)
    print_summary()
    write_summary_to_csv(statsDict, csvFilePath)

    return 0


if __name__ == "__main__":
    main()
