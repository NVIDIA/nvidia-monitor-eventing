# RAS Catalog Spec2Code Tool Design Document

## Overview

The rascat_spec2code tool converts Server RAS (Reliability, Availability, Serviceability) Catalog specifications and FPGA register table definitions into standardized formats for device monitoring and event handling in NVIDIA server platforms.

## Input Sources

### Server RAS Catalog (.xlsx)
- **Source**: Internal Google Sheets document
- **Required Sheets**:
  - Error List: Contains error definitions and configurations
  - Devices: Contains device mapping information

### FPGA Register Table (.xlsx)
- Register definitions and interrupt configurations
- Key fields:
  - Reg Name
  - Address Start/End
  - Bits
  - Int for HMC
  - Default

## Command-Line Methods

### to_event_info_json
Converts Server RAS Catalog to event info JSON format.

```bash
./rascat_spec2code Server-RAS-Catalog.xlsx to_event_info_json <platform>
```

#### Parameters
- `platform`: Target platform name (e.g., "Blackwell-HGX-8-GPU", "GH200 NVL", "GB200 NVL")

#### Processing
1. Validates platform compatibility
2. Processes device patterns and mappings
3. Converts catalog entries to JSON format
4. Applies platform-specific configurations

#### Output Format
```json
{
  "Device": [
    {
      "event": "<device_name> <event_name>",
      "device_type": "<device_type>",
      "error_id": "<error_id>",
      "managed": "<managed_flag>",
      "category": ["<category>"],
      "event_trigger": {
        "type": "DBUS",
        "object": "/xyz/openbmc_project/GpioStatusHandler",
        "interface": "xyz.openbmc_project.GpioStatus",
        "property": "<gpio_name>",
        "check": {"equal": "false"}
      },
      "severity": "<severity>",
      "resolution": "<resolution>",
      "debounce": {
        "type": "<type>",
        "duration": "<duration>"
      },
      "redfish": {
        "message_id": "<message_id>",
        "origin_of_condition": "<condition_origin>",
        "message_args": {
          "patterns": ["<pattern1>", "<pattern2>"],
          "parameters": []
        }
      }
    }
  ]
}
```

### fpga_alert_rascat
Converts FPGA register table alerts to RAS catalog entries.

```bash
./rascat_spec2code Server-RAS-Catalog.xlsx fpga_alert_rascat <fpga_regtbl_file> --sheet "<sheet_name>"
```

#### Parameters
- `fpga_regtbl_file`: FPGA register table Excel file
- `--sheet`: Sheet name in Excel file (default: "register table")

#### FPGA Register Table Format
The FPGA register table must contain the following columns:
- Reg Name
- Address Start/End
- Bits
- Int for HMC
- Default

(FPGA Register Table for short: **FRTL** in the following sections)

#### Server RAS Catalog Format
The Server RAS Catalog columns are not hardcoded, it will learn the columns from the Server RAS Catalog Excel file.
Populate the necessary columns accordingly (see Processing section for details), and leave the rest of the columns empty. But the columns order will be the same as the Server RAS Catalog Excel file.
(Server RAS Catalog for short: **SRASCAT** in the following sections)


#### Processing
1. Reads register table definitions from the FPGA register table (FRTL) Excel file
2. Identifies interrupt-capable registers (a.k.a *_INT) by the following criteria:
   1. FRTL."Int for HMC" is not empty
   2. FRTL."Bits" is not empty
   3. FRTL."Address Start" is not empty (if FRTL."Address End" is empty, assume it's the same as FRTL."Address Start")
3. For each *_INT register,
   1. Find its corresponding *_MASK register by looking for the register with the same "Reg Name" (without the "_INT" suffix) but with "_MASK" suffix.
   2. If the *_MASK register default value listed in FRTL."Default" is all 1s for all the bits, skip the *_INT register and go to the next *_INT register.
   3. Otherwise, process the *_INT register as follows:
      1. If the *_INT FRTL."Reg Name" has a device range in format of "[n-m]", remove the range part as the alert_name.
      2. device_name is the portion of the alert_name that is before the first "_", with the following policies:
         1. if device_name is I2C+number, remove the number part.
         2. if device_name is "NVSW" or "QM3", change it to "NVSwitch"
         3. if device_name is "PEXSW", change it to "NVLinkManagementNIC"
         4. if device_name is "CX8", change it to "ConnectX"
      3. SRASCAT."Error ID" = alert_name + "-ERROR"
      4. SRASCAT."Error Name" = alert_name + "Abnormal State Change"
      5. SRASCAT."Impacted Component" = device_name
      6. SRASCAT."Description" = "FPGA interrupt " + alert_name + " happens."
      7. SRASCAT."To HMC Code?" = "yes"
      8. SRASCAT."Error Type" = "FPGA-ALERT"
      9. SRASCAT."Hardware/GPIO Signal" = the translated FRTL."Int for HMC" of the *_INT register as the mapping below,
         1. FRTL."Int for HMC" == "T" => "THERM_OVERT"
         2. FRTL."Int for HMC" == "PF" => "PS_RUN_PWR_FAULT"
         3. Any other value and its a number **n** => "I2C**n**_ALERT"
      10. SRASCAT."Error Telemetry" = "BASEBOARD-REDFISH-EVENT-LOG"
      11. SRASCAT."RF Managed Error" = "no" if alert_name means thermal warning/alert otherwise "yes"
      12. SRASCAT."RF Rollup to Device" = per this mapping of the device_name of this alert_name:
          1. "{GPUDeviceID}" if device_name is about GPU
          2. "{CPUDeviceID}" if device_name is about CPU
          3. "{NVSwitchDeviceID} if device_name is about NVSwitch/NVSW/QM3
          4. "{NVLinkManagementNICID}" if device_name is about PEX/PEXSW
          5. otherwise "{BaseboardID}"
          6. Above policies are only applies to the ones that SRASCAT."RF Managed Error" == "yes". otherwise, leave it empty.
      13. SRASCAT."RF Log Producer" = "EventingService"
      14. SRASCAT."RF Log URI" = "/redfish/v1/Systems/{SystemID}/LogServices/EventLog"
      15. SRASCAT."RF Log Message ID" = "ResourceEvent.1.0.ResourceErrorsDetected"
      16. SRASCAT."RF Log Message Args" = "['{" + device_name + "DeviceID} " + alert_name (without device_name part) + "', 'Abnormal State Change']"
      17. SRASCAT."RF Log Health" = "Warning" if alert_name means thermal warning/alert otherwise "Critical"
      18. SRASCAT."RF Log Resolution" = "AC cycle the system. If problem persists, collect all dumps per troubleshooting guide, isolate the system and evaluate for RMA."
      19. SRASCAT."Resolution ID" = "COLLECT_LOGS"
      20. SRASCAT."RF Log Origin Of Condition" = SRASCAT."RF Rollup to Device"
      21. SRASCAT."Event Name" = SRASCAT."Error Name"
      22. SRASCAT."Device Type" = "{" + device_name + "DeviceID}"
      23. SRASCAT."Layer" = per this mapping below,
          1. "power_rail" if alert_name is about power
          2. "thermal_control" if alert_name is about thermal
          3. "interface_status" if alert_name is about HW interface like I2C, PCIe, etc.
          4. "pin_status" if alert_name is about present/gpio status
          5. "erot-control" if alert_name is about ERoT or IRoT
      24. SRASCAT."Trigger data config" = "[ type | DBUS ]; [ object | /xyz/openbmc_project/GpioStatusHandler ]; [ interface | xyz.openbmc_project.GpioStatus ]; [ property | " + SRASCAT."Hardware/GPIO Signal" + " ]"
      25. SRASCAT."Trigger check" = "[ equal | false ]"
      26. SRASCAT."Accessor data config" = "[ type | CMDLINE ]; [ executable | fpga_regtbl ]; [ arguments | " + alert_name + " " + device_range (per register bits or the device range in *_INT Reg Name, otherwise 0) + " ]"
      27. SRASCAT."Accessor check" = "[ equal | 1 ]"
      28. SRASCAT."Leaky bucket trigger count" = "0"
      29. SRASCAT."Data value is event count" = "FALSE"
      30. SRASCAT."RF Log Namespace" = SRASCAT."RF Rollup to Device"
      31. remaining columns in SRASCAT are left empty (learn columns from SRASCAT file)
   4. Generate RAS catalog entry for this alert in CSV format and print out on the screen
      1. If any commas in the content of any column, they will be handled correctly by the CSV format.
4. Finish and print out the numbers to stderr of,
    1. total number of RAS catalog entries found in FRTL
    2. total number of RAS catalog entries that are interrupt-capable
    3. total number of RAS catalog entries that are fully masked, partial masked, and not masked
    4. total number of RAS catalog entries that are valid interrupt to print out

#### Coding Style
1. Use python 3.x
2. Functionalize as much as possible
3. Reuse existing functions and modules
4. Follow the coding style of the existing code
5. Print debug logs in details if any error happens to stderr

### get_columns
Lists available columns in the Server RAS Catalog.

```bash
./rascat_spec2code Server-RAS-Catalog.xlsx get_columns
```

#### Output
List of column names from Server RAS Catalog.

### get_platforms
Lists supported platform names.

```bash
./rascat_spec2code Server-RAS-Catalog.xlsx get_platforms
```

#### Output
List of supported platform names:
- Blackwell-HGX-8-GPU
- GH200 NVL
- GB200 NVL

### get_device_list
Gets device list for a specific platform.

```bash
./rascat_spec2code Server-RAS-Catalog.xlsx get_device_list <platform>
```

#### Parameters
- `platform`: Target platform name

#### Output
Dictionary mapping device patterns to actual device names.

## Field-to-Field Conversions

### Key Mapping Functions
```python
def json_acc_to_fc_expr(accessor):
    # Converts accessor JSON to field catalog expression

def fc_expr_to_json_acc(expr, check=""):
    # Converts field catalog expression to accessor JSON

def json_dev_name(device_type):
    # Extracts device name from device type

def fc_get_hw_signal(accessor):
    # Extracts hardware signal from accessor configuration
```

### Pattern Substitution
```python
def pattern2device(text):
    # Substitutes device patterns with actual device names
```

## Error Handling

1. Input Validation
   - Excel file format checking
   - Required sheet presence
   - Column validation

2. Platform Compatibility
   - Platform name validation
   - Configuration availability check

3. Register Processing
   - Bit range validation
   - Mask register verification
   - Address range checks

## Dependencies

- Python 3.x
- Required packages:
  - pandas
  - argparse
  - csv
  - json

## Limitations

1. Input Format Requirements
   - Specific Excel sheet formats required
   - Fixed column names and structure

2. Platform Support
   - Limited to predefined platforms
   - Hardcoded platform configurations

3. Processing
   - Single-threaded execution
   - No incremental updates

## Future Enhancements

1. Additional Features
   - Support for new platforms
   - Custom field mappings
   - Batch processing

2. Improvements
   - Enhanced error reporting
   - CI/CD integration
   - Validation automation

3. Performance
   - Parallel processing
   - Incremental updates
   - Memory optimization

## References

- Server RAS Catalog Schema
- FPGA Register Table Format
- Platform Configuration Files
- Related Tools and Utilities

