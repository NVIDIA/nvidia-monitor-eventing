# Copyright (c) 2024, NVIDIA CORPORATION.  All rights reserved.
#
#  NVIDIA CORPORATION and its licensors retain all intellectual property
#  and proprietary rights in and to this software, related documentation
#  and any modifications thereto.  Any use, reproduction, disclosure or
#  distribution of this software and related documentation without an express
#  license agreement from NVIDIA CORPORATION is strictly prohibited.

"""
Common constants and functions
"""

import re


## where modules live
MODULES_DIR   = ""
# where templates as source for code generation live
TEMPLATES_DIR = ""

#EVENT_LOG_URI="redfish/v1/Systems/system/LogServices/EventLog/Entries"
EVENT_LOG_URI="redfish/v1/Systems/HGX_Baseboard_0/LogServices/EventLog/Entries"
DEVICE_CHASSIS_URI="redfish/v1/Chassis"
PHOSPHOR_LOG_BUSCTL    = "busctl get-property xyz.openbmc_project.Logging /xyz/openbmc_project/logging/entry"
PHOSPHOR_LOG_INTERFACE = "xyz.openbmc_project.Logging.Entry"
PHOSPHOR_LOG_ADDITIOANAL_DATA= "AdditionalData"

LOGGING_SERVICE = "xyz.openbmc_project.Logging"
LOGGING_OBJECT  = "/xyz/openbmc_project/logging"

# Constants used for Event Logging inserting in bustcl commands
REDFISH_MESSAGE_ID    = "REDFISH_MESSAGE_ID"
REDFISH_MESSAGE_ARGS  = "REDFISH_MESSAGE_ARGS"


# Not all fields from Json files are handled, two lists below:
JSON_MANDATORY_FIELDS  = ["redfish", "severity", "event"]
JSON_ADDITIONAL_FIELDS = ["resolution", "accessor", "event_trigger"]


# Keys used in dicts
KEY_MESSAGE_ID = "MessageId"
KEY_SEVERITY = 'Severity'

KEY_ACCESSOR = "Accessor"
KEY_ACCESSOR_TYPE      = KEY_ACCESSOR + ".Type"
KEY_ACCESSOR_OBJECT    = KEY_ACCESSOR + ".Object"
KEY_ACCESSOR_INTERFACE = KEY_ACCESSOR + ".Interface"
KEY_ACCESSOR_PROPERTY  = KEY_ACCESSOR + ".Property"
KEY_ACCESSOR_CHECK     = KEY_ACCESSOR + ".Check"
KEY_ACCESSOR_EXECUTABLE =  KEY_ACCESSOR + ".Executable"
KEY_ACCESSOR_ARGUMENTS =  KEY_ACCESSOR + ".Arguments"

KEY_ACCESSOR_CHECK_BITMASK = KEY_ACCESSOR_CHECK + ".Bitmask"
KEY_ACCESSOR_CHECK_LOOKUP  = KEY_ACCESSOR_CHECK + ".Lookup"
KEY_ACCESSOR_CHECK_EQUAL   = KEY_ACCESSOR_CHECK + ".Equal"
KEY_ACCESSOR_CHECK_NOTEQUAL = KEY_ACCESSOR_CHECK + ".Not_equal"

KEY_ACCESSOR_DEP           = KEY_ACCESSOR + ".Dependency"
KEY_ACCESSOR_DEP_TYPE      = KEY_ACCESSOR_DEP + ".Type"
KEY_ACCESSOR_DEP_PROPERTY  = KEY_ACCESSOR_DEP + ".Property"
KEY_ACCESSOR_DEP_EXECUTABLE =  KEY_ACCESSOR_DEP + ".Executable"
KEY_ACCESSOR_DEP_ARGUMENTS =  KEY_ACCESSOR_DEP + ".Arguments"

KEY_DEP_ACCESSOR_CHECK         = KEY_ACCESSOR_DEP + ".Check"
KEY_ACCESSOR_DEP_CHECK_BITMASK = KEY_DEP_ACCESSOR_CHECK + ".Bitmask"
KEY_ACCESSOR_DEP_CHECK_LOOKUP  = KEY_DEP_ACCESSOR_CHECK + ".Lookup"
KEY_ACCESSOR_DEP_CHECK_EQUAL   = KEY_DEP_ACCESSOR_CHECK + ".Equal"
KEY_ACCESSOR_DEP_CHECK_NOTEQUAL = KEY_DEP_ACCESSOR_CHECK + ".Not_equal"

KEY_EVENT_TRIGGER = "Event_trigger"
KEY_EVENT_TRIGGER_TYPE      = KEY_EVENT_TRIGGER + ".Type"
KEY_EVENT_TRIGGER_OBJECT    = KEY_EVENT_TRIGGER + ".Object"
KEY_EVENT_TRIGGER_INTERFACE = KEY_EVENT_TRIGGER + ".Interface"
KEY_EVENT_TRIGGER_PROPERTY  = KEY_EVENT_TRIGGER + ".Property"
KEY_EVENT_TRIGGER_CHECK     = KEY_EVENT_TRIGGER + ".Check"
KEY_EVENT_TRIGGER_CHECK_BITMASK = KEY_EVENT_TRIGGER_CHECK + ".Bitmask"
KEY_EVENT_TRIGGER_CHECK_LOOKUP  = KEY_EVENT_TRIGGER_CHECK + ".Lookup"

## current device being handled
DEVICE_HW = ""

# common constants, it is used for both KEY_ACCESSOR AND KEY_EVENT_TRIGGER
ACCESSOR_TYPE_DBUS      = "DBUS"
ACCESSOR_TYPE_DBUS_CALL = "DBUS CALL"
ACCESSOR_TYPE_DEVICE_CORE_API = "DeviceCoreAPI"
ACCESSOR_TYPE_CMDLINE = "CMDLINE"

LOGGING_ENTRY_STR       = "xyz.openbmc_project.Logging.Entry"
LOGGING_ENTRY_DOT_STR   = f"{LOGGING_ENTRY_STR}."


## this the index of the device defined in the input json file such as
##   "GPU": [ .. ]  -> GPU0, GPU1, ...  index= 0, 1, ...
CURRENT_JSON_DEVICE_INDEX = 0

# index in busctl_info
INDEX_DEVICE_NAME     = 0
INDEX_EVENT           = 1


# BUSCTL command index for Event Logging inserting -> Corresponding field position
BUSCTL_MSG_IDX            = 7   # "GPU0 OverT"
BUSCTL_SEVERITY_IDX       = 8   # "xyz.openbmc_project.Logging.Entry.Level.Critical"
BUSCTL_ADDITIONALDATA_IDX = 10  # where additinal data starts


# Keys used for comparing 'injected events' and 'redfish data'
#   if an element is a list: the first element is the key and the second is an alias
MANDATORY_EVENT_KEYS = [ KEY_SEVERITY, 'Resolution', [KEY_MESSAGE_ID, REDFISH_MESSAGE_ID] ]
OPTIONAL_EVENT_KEYS  = [ ['MessageArgs', REDFISH_MESSAGE_ARGS] ]

# Flags used for comparing 'injected events' and 'redfish data'
MANDATORY_FLAG       = True
OPTIONAL_FLAG        = False

SEVERITYDBUSTOREDFISH = {
                            'critical'  : [
                                        "xyz.openbmc_project.Logging.Entry.Level.Alert",
                                        "xyz.openbmc_project.Logging.Entry.Level.Critical",
                                        "xyz.openbmc_project.Logging.Entry.Level.Emergency",
                                        "xyz.openbmc_project.Logging.Entry.Level.Error"],
                            'ok'        : [
                                            "xyz.openbmc_project.Logging.Entry.Level.Debug",
                                            "xyz.openbmc_project.Logging.Entry.Level.Informational",
                                            "xyz.openbmc_project.Logging.Entry.Level.Notice"],
                            'warning'   : ["xyz.openbmc_project.Logging.Entry.Level.Warning"]
                        }

## avoid huge range expansion such as:
## /xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_[0-3]/Ports/NVLink_[0-39] , in this case the second range will be limited by 2
DOUBLE_EXPANSION_LIMIT = 5

# Used to say that a previous range must be repeated and not double expanded agains the first one
RANGE_REPEATER_INDICATOR = '()'

# Data to be used on custom Events
#------------------------

LIST_EVENTS = False  # True list Devices and exit
LIST_SINGLE_INJECTION_COMMANDS = False ## True when generating the single-injection commands

# when not empty means the device we want to perform only: GPU,
SINGLE_DEVICE = ""
SINGLE_EVENT = ""         # performs just that event name
SINGLE_DEVICE_INDEX = ""  # performs just that device index
CUSTOM_EVENT  = False     # True when one of SINGLE customization above is not empty

range_expansion_list = [] ## global variable used in expand_range with names
range_start_end_list = [] ## global list of integer list with start-end for each range

def get_logging_entry_level(level):
    """
    Just formats a entry level
    """
    if level.lower() == "ok":
        full_level = f"{LOGGING_ENTRY_STR}.Level.Notice"
    else:
        level_str = level[0].upper()
        level_str += level[1:]
        full_level = f"{LOGGING_ENTRY_STR}.Level.{level_str}"
    return full_level


def replace_occurrence(string, occurrence):
    """
    replaces any sequence of "()" before a range specification in string by occurrence
    example:
           print (replace_occurrence("test_()_more_()", "1"))
           print (replace_occurrence("test_()_more_()_not[1-2]_()", "1"))
    prints:
          test_1_more_1
          test_1_more_1_not[1-2]_()
    """
    my_string = string
    occurrence_defined =  my_string.find(RANGE_REPEATER_INDICATOR)
    if occurrence_defined != -1:
        open_bracket = my_string.find('[')
        if open_bracket != -1:
            occurrence_defined =  my_string.find(RANGE_REPEATER_INDICATOR, 0, open_bracket)
        while occurrence_defined != -1:
            my_string = my_string[:occurrence_defined] + occurrence +  my_string[occurrence_defined + 2 : ]
            open_bracket = my_string.find('[')
            if open_bracket != -1:
                occurrence_defined =  my_string.find(RANGE_REPEATER_INDICATOR, 0, open_bracket)
            else:
                occurrence_defined =  my_string.find(RANGE_REPEATER_INDICATOR)
    return my_string


def get_range(name):
   """
   return the range string such as [0-8] or an empty string when it does not not exist
   """
   range_str = ""
   open_bracket = name.find('[')
   if open_bracket != -1:
      close_bracket = name.find(']', open_bracket)
      if close_bracket != -1:
         range_str = name[open_bracket: close_bracket + 1]
   return range_str



def replace_last_range_if_repeated(current_string, value):
   """
   takes the last range from range_expansion_list and checks if the same range
   is present in 'current_string' but having an index to inform to not double
   expand it, then and if so replaces that range with index by value.
   Example:

     current_string = "test[1|[1-8]/more"
     range_expansion_list has two elements and the latest is "[1-8]", meaning a
        that had already been replace by 'value' once.

     returns a string with all ocurrences of "[1|[1-8]" replaced by 'value'

   """
   last_range_index = len(range_expansion_list) -1
   range_repeated_begin = f"[{last_range_index}|"
   last_range =   range_expansion_list[last_range_index]
   range_repeated = last_range.replace("[", range_repeated_begin, 1)
   replaced = current_string.replace(range_repeated, value)
   return replaced



def expand_range(name, limit=0, occurrence=0):
    """
    expand_range(range_string) returns a list from strings with/without range specification at end

    The 'limit' parameter can be used to avoid big expansions

    expand a name such as:

        expand_range("name[1-5]")         -> ['name1', 'name2', 'name3', 'name4', 'name5']
        expand_range("unique")            -> ['unique']
        expand_range("other[0, 1 , 3-5]") -> ['other0', 'other1', 'other3', 'other4', 'other5']
        expand_range("chars[0, 1 , g-k]") ->
              ['chars0', 'chars1', 'charsg', 'charsh', 'charsi', 'charsj', 'charsk']

    """
    if limit == 0:
       range_expansion_list.clear()
       range_start_end_list.clear()

    list_names = []
    open_bracket = name.find('[')
    range_str = get_range(name)
    if len(range_str) > 0:
       range_expansion_list.append(range_str)
       values_range = range_str.split('-')
       value = 0
       ## check if exists [0|value-final_value] -> 0=index_range
       index_range = range_str.find("|")
       if index_range != -1:
          value = int(values_range[0][3:])
       else:
          value = int(values_range[0][1:])
       final_value = int(values_range[1][:-1])

       ranges_size = len(range_start_end_list)
       if occurrence == 0 or \
          (ranges_size > 0 and occurrence == range_start_end_list[ranges_size -1][0]):
            final_value_used = final_value
            if limit > 0 and final_value_used > limit:
                    final_value_used = limit
            list_range = [value, final_value_used]
            range_start_end_list.append(list_range)

       while value <= final_value:
          ## index_range DOES NOT exist,  search for other referencing that one
          if index_range == -1:
             replaced = name.replace(range_str, str(value), 1)
             replaced = replace_last_range_if_repeated(replaced, str(value))
          else:
             ## replace all ocurrences
             replaced = name.replace(range_str, str(value))
          replaced = replace_occurrence(replaced, str(value))
          open_bracket = replaced.find('[', open_bracket)
          if open_bracket != -1:
             sub_list = expand_range(replaced, DOUBLE_EXPANSION_LIMIT, value)
             # No longer changing the order as we may have range specication
             # if len(sub_list) > 0:
             #    first_sub_list = sub_list.pop(0)
             #    list_names.insert(value, first_sub_list)
             list_names.extend(sub_list)
          else:
             list_names.append(replaced)
          value += 1
          if value == limit:
             break
    else:
         list_names.append(name)
    return list_names


def lists_are_equal(list1, list2):
    """
    Compare two lists
    """
    if len(list1) != len(list2):
        return False
    return sorted(list1) == sorted(list2)


def is_event_mandatory_key(key):
    """
    Returns true if a such key belongs to MANDATORY_EVENT_KEYS list
    """
    if key.startswith(LOGGING_ENTRY_DOT_STR):
        key = key[len(LOGGING_ENTRY_DOT_STR):]
    for mandatory_key in MANDATORY_EVENT_KEYS:
        if isinstance(mandatory_key, list):
            if key in mandatory_key:
                return mandatory_key[0]
        elif key == mandatory_key:
            return mandatory_key
    return None


def is_event_optional_key(key):
    """
    Returns true if a such key belongs to OPTIONAL_EVENT_KEYS list
    """
    if key.startswith(LOGGING_ENTRY_DOT_STR):
        key = key[len(LOGGING_ENTRY_DOT_STR):]
    for optional_key in OPTIONAL_EVENT_KEYS:
        if isinstance(optional_key, list):
            if key in optional_key:
                return optional_key[0]
        elif key == optional_key:
            return optional_key
    return None


def __compare_event_data_and_redfish_data(key_list, mandatory_flag, injected_dict, redfish_dict):
    """
    Generic Private function to compare Event data between 'inject events' and
       'events collected' from redifsh
    The key_list can be either MANDATORY_EVENT_KEYS or OPTIONAL_EVENT_KEYS
    """
    ret = True
    try:
        for key in key_list:
            # injected_dict can use the alias key in MANDATORY_EVENT_KEYS or OPTIONAL_EVENT_KEYS
            injected_key = key
            if isinstance(key, list):
                # injected_dict is using the alias key
                injected_key = key[1] if key[1] in injected_dict else key[0]
                key = key[0]
            exist_both_keys = injected_key in injected_dict and key in redfish_dict
            if exist_both_keys is False:
                if mandatory_flag is True:
                    print(f"\tError: Key '{key}' does not exist in redfish entry")
                    ret =  False  # both dicts must have the key value
                continue          # ok if the key information misses in one or both dicts
            # performs the comparing
            if key == KEY_SEVERITY: # special Severity cheking
                sub_severity_key = redfish_dict[key].lower()
                if injected_dict[injected_key] not in SEVERITYDBUSTOREDFISH[sub_severity_key]:
                    ret = False
                continue  ## Severity field OK
            ## filds comparing
            if isinstance(redfish_dict[key], list):
                injected_list_value = re.split(r',\s*', injected_dict[injected_key])
                if lists_are_equal(redfish_dict[key], injected_list_value):
                    continue
                print(f"\tError: the content of the lists of Key '{key}' does not match, redfish={redfish_dict[key]} injected={injected_list_value}")
                ret = False
            value_injected = injected_dict[injected_key]
            value_redfish = redfish_dict[key]
            ## compare strings without spaces, this works around some event creation problems
            if isinstance(value_injected, str):
                value_injected = ''.join(value_injected.split())
            if isinstance(value_redfish, str):
                value_redfish = ''.join(value_redfish.split())
            if  value_injected != value_redfish:
                print(f"\tError: the value of Key '{key}' does not match, redfish={value_redfish} injected={value_injected}")
                ret = False
    except Exception as error:
        raise error
    return ret


def compare_mandatory_event_fields(injected_dict, redfish_dict):
    """
    Compare mandatory Event Data Fields between 'inject events' and events collected  from redifsh
    """
    return __compare_event_data_and_redfish_data(MANDATORY_EVENT_KEYS, MANDATORY_FLAG,
                                               injected_dict, redfish_dict)


def compare_optional_event_fields(injected_dict, redfish_dict):
    """
    Compare optional Event Data Fields between 'inject events' and events collected  from redifsh
    """
    return __compare_event_data_and_redfish_data(OPTIONAL_EVENT_KEYS, OPTIONAL_FLAG,
                                               injected_dict, redfish_dict)
