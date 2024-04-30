#!/usr/bin/env xonsh

import pandas as pd
import sys
import re
import logging
import json

logging.basicConfig(force = True, level=logging.WARNING)

def openInput(filename, **rest):
    return (sys.stdin if filename == "-"
            else open(filename, "r", **rest))

def openOutput(filename, **rest):
    return (sys.stdout if filename == "-"
            else open(filename, "w", **rest))

def getDeviceGroupRegex(devices):
    names = (list(devices.keys())
             + [elem for x in list(devices.values()) for elem in x])
    return "|".join(names)

def getDeviceTypeRegex(deviceRegex):
    # "GPU"
    # "GPU[0-7]"
    return f"({deviceRegex})( *\\[[^\\]]+\\])?"

# OpCode (like "b2h") surrounded by word boundaries
def getOpCodeRegex(opCodeGpioMapping):
    return "\\b" + "|".join(opCodeGpioMapping.keys()) + "\\b"

# ...
#    "PCIeSwitch0" : ["PCIe Switch"]
#    ^-----------^   ^-------------^
#   Canonical name  List of synonyms
# ...
devices = {
    "System" : [],
    "GPU" : [],
    "NVSwitch" : [],
    "PCIeSwitch0" : ["PCIe Switch"],
    "Retimer" : [],
    "HSC" : [],
    "InletSensor" : [],
    "Baseboard" : [],
    "FPGA" : [],
    "HMC" : [],
    "LS10" : []}

DEVICE_GROUP_REG = getDeviceGroupRegex(devices)
DEVICE_TYPE_REG = getDeviceTypeRegex(DEVICE_GROUP_REG)

def scenarioParse(s):
    """Find and extract the device type using 'DEVICE_TYPE_REG'
regular expression. Remove it from 's' and treat all the rest as
a fault name.
    """
    # "VR fault - GPU[0-7]"
    # "NVSwitch[0-3] VR fault"
    m = re.search(DEVICE_TYPE_REG, s, re.IGNORECASE)
    return ((m[1],
             m[0],
            (s[:m.start(0)] + s[m.end(0)+1:]).strip(
                " \t\r\n-,.;'"))
            if m else None)

def ensureEntry(jsonObj, *keys):
    obj = jsonObj
    for key in keys:
        if key not in obj:
            obj[key] = {}
        obj = obj[key]

# "TODO: Accessor definition [type: Object]
#        ^-----------------^        ^----^
#              what                jsonType
#
#    (could not be deduced from cell; cell value: 'any other WP?';
#    ^-----------------------------^               ^-----------^
#             comment                                 cell
#
#    row: 53, col: 'Fault Scenario')"
#         ^^        ^------------^
#     where[0]         where[1]

def todoMessage(what,
                jsonType,
                /,
                comment = None,
                cell = None,
                where = None):
    base = f"TODO: {what} [type: {jsonType}]"
    info = "; ".join(filter(
        lambda x: x is not None,
        [comment,
         f"cell value: '{cell}'" if cell else None,
         f"row: {where[0]}, col: '{where[1]}'" if where else None]))
    return (f"{base} ({info})" if info != "" else base)

def failedCellMsg(cell, row, col):
    return {
        "comment": "could not be deduced from the cell",
        "cell": cell,
        "where": (row, col)
    }

def emptyCellMsg(row, col):
    return {
        "comment": "could not be deduced from an empty cell",
        "cell": None,
        "where": (row, col)
    }


vfSummaryFilePathIn  = $ARG1
vfOpCodePathIn       = $ARG2
eventInfoJsonPathOut = $ARG3

# Keys must be in lower case
opCodeGpioMapping = {
    "1ah": "I2C3_ALERT",
    "b2h": "I2C4_ALERT",
    "21h": "IC23_ALERT",
    "b3h": "I2C4_ALERT"}

OP_CODE_REG = getOpCodeRegex(opCodeGpioMapping)

referenceCol      = "SR #"
faultScenarioCol  = "Fault Scenario"
recoveryCol       = "Recovery"
severityCol       = "Severity"
messageIdCol      = "MessageId"
messageArgsCol    = "MessageArgs"
deducedJsonCol    = "Deduced JSON"
oobApiCol         = "GPUOOB Property/OOB API"
opCodeCol         = "OpCode"
additionalDataCol = "AdditionalData"

def readTable(fileName):
    result = None
    with openInput(fileName, encoding='utf-8') as f:
        result = pd.read_csv(f, sep = "	", quotechar = "'")
    return result

vfSum = readTable(vfSummaryFilePathIn)
vfOpCode = readTable(vfOpCodePathIn)

vf = pd.merge(vfSum, vfOpCode, how = "left", on = referenceCol,
              suffixes = ["_summary", "_opcode"])

n = vf.shape[0]
eventInfoJson = {}

accessorDefinition = "accessor object (see 'https://gitlab-collab-01.nvidia.com/viking-team/nvidia-monitor-eventing/-/tree/vulcan_faults/examples#accessor-format')"

for (i, row) in vf.iterrows():
    gsheetsIndex = i + 2
    deducedJson = {}

    deducedJson["remove_this_property"] = (
        f"Json object auto-generated from Vulcan Faults table, row {gsheetsIndex}, " +
        f"{referenceCol} = '{row[referenceCol]}'")

    deviceGroup = None
    # .event
    # .device_type
    scenarioRaw = row[f"{faultScenarioCol}_summary"]
    if not pd.isnull(scenarioRaw):
        scenario = scenarioRaw.strip()
        # "GPU[0-7] VR Fault" →
        # "GPU", "GPU[0-7]", "VR Fault"
        parse = scenarioParse(scenario)
        if parse:
            deviceGroup, deviceType, faultName = parse
            deducedJson["event"] = faultName
            deducedJson["device_type"] = deviceType
            logging.debug(f"'{scenario}' → '{deviceType}': '{faultName}'")
        else:
            deducedJson["event"] = todoMessage(
                "Event name",
                "string",
                **failedCellMsg(scenario, gsheetsIndex, faultScenarioCol))
            deducedJson["device_type"] = todoMessage(
                "Device specification",
                "string",
                **failedCellMsg(scenario, gsheetsIndex, faultScenarioCol))
            logging.warning(f"Unrecognized scenario: '{scenario}'")
    else:
        deducedJson["event"] = todoMessage(
            "Event name",
            "string",
            **emptyCellMsg(gsheetsIndex, faultScenarioCol))
        deducedJson["device_type"] = todoMessage(
            "Device specification",
            "string",
            **emptyCellMsg(gsheetsIndex, faultScenarioCol))
        logging.warning(f"Row {i}: missing ‘{faultScenarioCol}’")

    # .event_trigger
    if not pd.isnull(row[opCodeCol]):
        res = re.search(OP_CODE_REG, row[opCodeCol], re.IGNORECASE)
        if res:
            deducedJson["event_trigger"] = {
                "type": "DBUS",
                "object": "/xyz/openbmc_project/GpioStatusHandler",
                "interface": "xyz.openbmc_project.GpioStatus",
                "property": opCodeGpioMapping[res[0].lower()],
                "check": {
                    "equal": "false"
                }
            }
        else:
            deducedJson["event_trigger"] = todoMessage(
                "Accessor definition", accessorDefinition)
    else:
        deducedJson["event_trigger"] = todoMessage(
                "Accessor definition", accessorDefinition)

    # .accessor
    if not pd.isnull(row[oobApiCol]):
        deducedJson["accessor"] = {
            "type": "DeviceCoreAPI",
            "property": row[oobApiCol].strip(),
            "check": todoMessage(
                "Value checking definition", "object")
        }
    else:
        deducedJson["accessor"] = todoMessage(
            "Accessor definition", accessorDefinition,
            **emptyCellMsg(gsheetsIndex, oobApiCol))

    # .severity
    if not pd.isnull(row[severityCol]):
        deducedJson["severity"] = row[severityCol].strip()
    else:
        deducedJson["severity"] = todoMessage(
            "Severity level", "string",
            **emptyCellMsg(gsheetsIndex, severityCol))

    # .resolution
    if not pd.isnull(row[recoveryCol]):
        deducedJson["resolution"] = row[recoveryCol].strip()
    else:
        deducedJson["resolution"] = todoMessage(
            "Resolution description", "string",
            **emptyCellMsg(gsheetsIndex, recoveryCol))

    # .trigger_count
    # Purposefuly erroneous
    deducedJson["trigger_count"] = todoMessage(
        "Trigger count", "integer >= 0")

    # .event_counter_reset
    deducedJson["event_counter_reset"] = {
        "type": todoMessage("type", "string"),
        "metadata": todoMessage("metadata", "string")
    }

    # .redfish
    deducedJson["redfish"] = {}
    # .redfish.message_id
    if not pd.isnull(row[messageIdCol]):
        deducedJson["redfish"]["message_id"] = row[messageIdCol]
    else:
        deducedJson["redfish"]["message_id"] = todoMessage(
            "Message Id", "string",
            **emptyCellMsg(gsheetsIndex, messageIdCol))

    # .redfish.message_args.patterns
    # .redfish.message_args.parameters
    if not pd.isnull(row[messageArgsCol]):
        ensureEntry(deducedJson, "redfish", "message_args")
        try:
            parsedMessageArgsCol = json.loads(row[messageArgsCol])
            deducedJson["redfish"]["message_args"]["patterns"] = parsedMessageArgsCol
            deducedJson["redfish"]["message_args"]["parameters"] = []
            paramsJoined = "".join(parsedMessageArgsCol)
            for placeholder in re.findall("{([^}]*)}", paramsJoined):
                if placeholder.endswith("Id"):
                    paramAccessor = {
                        "type": "DIRECT",
                        "field": "CurrentDeviceName"
                    }
                else:
                    paramAccessor = todoMessage(
                        "Value accessor", accessorDefinition,
                        **failedCellMsg(
                            row[messageArgsCol],
                            gsheetsIndex, messageArgsCol))
                deducedJson["redfish"]["message_args"]["parameters"] += [paramAccessor]

        except json.JSONDecodeError as e:
            logging.warning(f"Unable to parse “{messageArgsCol}” column value " +
                            f"‘{row[messageArgsCol]}’ as a json value")
    else:
        ensureEntry(deducedJson, "redfish", "message_args")
        deducedJson["redfish"]["message_args"]["patterns"] = todoMessage(
            "Message patterns", "array of strings",
            **emptyCellMsg(gsheetsIndex, messageArgsCol))
        deducedJson["redfish"]["message_args"]["parameters"] = todoMessage(
            "Pattern placeholder values", "array of objects",
            **emptyCellMsg(gsheetsIndex, messageArgsCol))

    # .telemetries
    if not pd.isnull(row[additionalDataCol]):
        deducedJson["telemetries"] = [
            todoMessage(f"'{elem.strip()}'",
                        "object",
                        where = (gsheetsIndex, additionalDataCol))
            for elem in row[additionalDataCol].split("\n")
            if elem.strip() != ""]
    else:
        deducedJson["telemetries"] = []

    row[deducedJsonCol] = json.dumps(deducedJson)
    group = deviceGroup if deviceGroup else "Uncategorized"
    if group not in eventInfoJson.keys():
        eventInfoJson[group] = []
    eventInfoJson[group] += [deducedJson]

with open(eventInfoJsonPathOut, "w") as eventInfoJsonFile:
    json.dump(eventInfoJson, eventInfoJsonFile, indent = 2)

# vf.to_csv(vfSummaryFilePathOut, index = False,
#           encoding = 'utf-8',
#           quotechar = "'",
#           sep = "\t")
