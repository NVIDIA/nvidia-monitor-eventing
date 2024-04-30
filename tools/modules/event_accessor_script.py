# Copyright (c) 2024, NVIDIA CORPORATION.  All rights reserved.
#
#  NVIDIA CORPORATION and its licensors retain all intellectual property
#  and proprietary rights in and to this software, related documentation
#  and any modifications thereto.  Any use, reproduction, disclosure or
#  distribution of this software and related documentation without an express
#  license agreement from NVIDIA CORPORATION is strictly prohibited.

"""
Module for handling devices from Json input file which as 'accessor' information not empty
So far accessor.type = DBUS are handled
"""
import random

import com ## common constants and and functions

from injector_script_base import InjectorScriptBase

TEMPLATE_NAME = "event_accessor_script.bash"

class EventAccessorInjectorScript(InjectorScriptBase):
    """
    This class parses a Json Event file, then:
       1 Generates a bash script with busctl commands
    """

    def __init__(self, json_file):
        super().__init__(json_file)
        self._busctl_cmd_counter = 0
        self._accessor_dbus_expected_events_list = []
        self._event_unique_list = {} ## a unique device+event list
        self._max_commands = 0 # if greater than zero tells the maximum commands
        self._unique_event = False ## during processing, indentifies if the event is the first occurrence
        self._seed = 2
        """
        Keeps commands in a list just to have the total counter
        Then comands are generated later
        """
        self._commands_information_list = []


    def create_script_file(self):
        """
        override parent method
        """
        template_file = com.TEMPLATES_DIR + '/' + TEMPLATE_NAME
        template = open(template_file, mode='r')
        content=""
        ok = False
        if template != None:
            content = template.read()
            template.close()
            if len(content) != 0:
               ok = True
        if ok is False:
               raise Exception(f"template file '{template_file}' not found or empty")

        super().priv_create_script_file(content)

    def close_script_file(self):
        """
        override parent method, does nothing, close will be called from super()
        """


    def get_accessor_dbus_expected_events_list(self):
        """
        Returns a list of event data which has accessor.type = DBUS
        """
        return self._accessor_dbus_expected_events_list


    def __join_event_trigger_and_accessor(self):
        """
        move event_trigger information into accessor
        set accessor.dependency.type       from accessor.type
            accessor.dependency.property   from accessor.property
        """
        accessor_key = f"{com.KEY_ACCESSOR}."
        for key in list(self._additional_data.keys()):
            if key.startswith(accessor_key):
                dep_field = key.split(accessor_key)[1]
                dep_key = com.KEY_ACCESSOR_DEP + '.' + dep_field
                self._additional_data[dep_key] = self._additional_data[key]
                del self._additional_data[key]

        event_trigger_key = f"{com.KEY_EVENT_TRIGGER}."
        for key in list(self._additional_data.keys()):
            if key.startswith(event_trigger_key):
                accessor_field = key.split(event_trigger_key)[1]
                accessor_key   = com.KEY_ACCESSOR + '.' + accessor_field;
                self._additional_data[accessor_key] = self._additional_data[key]
                del self._additional_data[key]


    def create_device_index_string_list(self):
        """
        Creates a list of device_index strings such as ["0", "1" ..] or [ "0,0", "0,1"] for double ranges
        It is based on com.range_start_end_list
        """
        # case 1 com.range_start_end_list is empty
        size = len(com.range_start_end_list)
        device_index_list = []
        if size == 0:
            device_index_list.append("0")
        elif size == 1:
            for number in range(com.range_start_end_list[0][0], com.range_start_end_list[0][1] + 1):
                device_index_list.append(f"{number}")
        elif size == 2:
            middle_start = com.range_start_end_list[1][0]
            middle_end = com.range_start_end_list[1][1] + 1
            for first_index in range(com.range_start_end_list[0][0], com.range_start_end_list[0][1] + 1):
                for second_index in range(middle_start, middle_end):
                     device_index_list.append(f"{first_index},{second_index}")
        return device_index_list

    def __get_accessor_type(self):
        """
        Returns None or com.ACCESSOR_TYPE_DBUS or ACCESSOR_TYPE_DBUS_CALL
        """
        ret = "none"
        if com.KEY_EVENT_TRIGGER_TYPE in self._additional_data:
            self.__join_event_trigger_and_accessor()
        if com.KEY_ACCESSOR_TYPE in self._additional_data:
            accessor_type = self._additional_data[com.KEY_ACCESSOR_TYPE]
            ret = accessor_type
            if accessor_type.upper() == com.ACCESSOR_TYPE_DBUS:
                try:
                    if len(self._additional_data[com.KEY_ACCESSOR_OBJECT]) > 0 and \
                        len(self._additional_data[com.KEY_ACCESSOR_INTERFACE])  > 0 and \
                        len(self._additional_data[com.KEY_ACCESSOR_PROPERTY])   > 0:
                        ret = com.ACCESSOR_TYPE_DBUS
                except Exception as exc:
                    raise Exception(f"No information {com.ACCESSOR_TYPE_DBUS}") from exc
            elif accessor_type.upper() == com.ACCESSOR_TYPE_DBUS_CALL:
                if com.KEY_ACCESSOR_PROPERTY in self._additional_data and \
                    len(self._additional_data[com.KEY_ACCESSOR_PROPERTY]) > 0:
                        ret = com.ACCESSOR_TYPE_DBUS_CALL
                else:
                    # so far consider this as DUBS to generate wrong set-property command
                    ret = com.ACCESSOR_TYPE_DBUS
        return ret


    def __store_command_information(self, information, device_index):
        """
        Stores command information in the list self._commands_information_list
        This information is used to generate 'busctl 'commands later
        """
        if len(com.SINGLE_DEVICE_INDEX) > 0:
           if device_index != com.SINGLE_DEVICE_INDEX:
              return False

        information["device"] = com.DEVICE_HW
        information["event"]  = self._busctl_info[com.INDEX_EVENT]

        if information["range"] != "":
            information["device_index"] = device_index
        else:
            information["device_index"] = 0

        if com.KEY_ACCESSOR_INTERFACE in self._additional_data:
            information[com.KEY_ACCESSOR_INTERFACE] = \
                self._additional_data[com.KEY_ACCESSOR_INTERFACE]
        if com.KEY_ACCESSOR_PROPERTY in self._additional_data:
            information[com.KEY_ACCESSOR_PROPERTY] = \
                self._additional_data[com.KEY_ACCESSOR_PROPERTY]

        ## handling single quote and quotes, making scape characters in strings
        copy_information = {}
        for info_key in information.keys():
           value = str(information[info_key])
           value = value.replace('"', '\\"')
           value = value.replace("'", "\\'")
           copy_information[info_key] = value

        self._commands_information_list.append(copy_information)

        return True


    def __write_busctl_commands_into_script(self, cmd_info):
        """
        Saves busctl commands in the script
        """
        cmd =  f"\n METHOD=\"{cmd_info['method']}\" METHOD_VALUE=\"{cmd_info['method_value']}\" "
        cmd += f"ACCESSOR_TYPE=\"{cmd_info['accessor_type']}\" "
        cmd += f" \\\n\tINJECTION=\"{cmd_info['injection']}\" INJECTION_PARAM=\"{cmd_info['injection_param']}\" "
        cmd += f"INJECTION_CHECK=\"{cmd_info['injection_check']}\" INJECTION_CHECK_VALUE=\"{cmd_info['injection_check_value']}\""
        if len(self._busctl_info) > com.INDEX_EVENT:
            cmd += f" \\\n\tDEVICE=\"{cmd_info['device']}\" DEVICE_RANGE=\"{cmd_info['range']}\" DEVICE_INDEX=\"{cmd_info['device_index']}\" EVENT=\"{cmd_info['event']}\" "

        cmd += f" \\\n\tproperty_change {cmd_info['device_item']} "
        if com.KEY_ACCESSOR_INTERFACE in cmd_info and com.KEY_ACCESSOR_PROPERTY in cmd_info:
            cmd += f"{cmd_info[com.KEY_ACCESSOR_INTERFACE]} "
            cmd += f"{cmd_info[com.KEY_ACCESSOR_PROPERTY]}\n"
        else:
            cmd += "\n"
        super().write(cmd)


    def __store_in_expected_events_list(self, device):
        """
        Stores event data to be later compared with RedFish data
        """
        event_data = self._additional_data.copy()
        super().remove_accessor_fields(event_data)
        event_data['event'] = self._busctl_info[com.INDEX_EVENT]
        event_data['device'] = com.DEVICE_HW
        self._accessor_dbus_expected_events_list.append(event_data)


    def set_injection_information(self, information):
        """
        """
        information['injection'] = "DBUS"
        information['injection_param'] = ""
        information['injection_check'] = ""
        information['injection_check_value'] = ""

        if com.KEY_ACCESSOR_DEP_TYPE in self._additional_data and \
           self._additional_data[com.KEY_ACCESSOR_DEP_TYPE] == com.ACCESSOR_TYPE_DEVICE_CORE_API:
            information['injection'] =  com.ACCESSOR_TYPE_DEVICE_CORE_API
            information['injection_param'] = self._additional_data[com.KEY_ACCESSOR_DEP_PROPERTY]
            if com.KEY_ACCESSOR_DEP_CHECK_LOOKUP in self._additional_data:
               information['injection_check'] = "lookup"
               information['injection_check_value'] = self._additional_data[com.KEY_ACCESSOR_DEP_CHECK_LOOKUP]
            elif com.KEY_ACCESSOR_DEP_CHECK_EQUAL  in self._additional_data:
               information['injection_check'] = "equal"
               information['injection_check_value'] = self._additional_data[com.KEY_ACCESSOR_DEP_CHECK_EQUAL]
            return

        if com.KEY_ACCESSOR_TYPE in self._additional_data and \
           self._additional_data[com.KEY_ACCESSOR_TYPE] == com.ACCESSOR_TYPE_DEVICE_CORE_API:
            information['injection'] =  com.ACCESSOR_TYPE_DEVICE_CORE_API
            information['injection_param'] = self._additional_data[com.KEY_ACCESSOR_PROPERTY]
            return

        if com.KEY_ACCESSOR_DEP_TYPE in self._additional_data and \
           self._additional_data[com.KEY_ACCESSOR_DEP_TYPE] == com.ACCESSOR_TYPE_CMDLINE:
            information['injection'] =  com.ACCESSOR_TYPE_CMDLINE
            information['injection_param'] = self._additional_data[com.KEY_ACCESSOR_DEP_EXECUTABLE] + \
                                             " " +  self._additional_data[com.KEY_ACCESSOR_DEP_ARGUMENTS]
            return

        if com.KEY_ACCESSOR_TYPE in self._additional_data and \
           self._additional_data[com.KEY_ACCESSOR_TYPE] == com.ACCESSOR_TYPE_CMDLINE:
            information['injection'] =  com.ACCESSOR_TYPE_CMDLINE
            information['injection_param'] = self._additional_data[com.KEY_ACCESSOR_EXECUTABLE] + \
                                             " " +  self._additional_data[com.KEY_ACCESSOR_ARGUMENTS]
            return


    def store_busctl_commands_as_dbus_accessor(self, device, accessor_type, information, objects_range):
        """
        Generates commands in the script regardless the accessor data
        """
        method="add"
        value="1"

        if com.KEY_ACCESSOR_CHECK_EQUAL in self._additional_data:
            method="equal"
            value = self._additional_data[com.KEY_ACCESSOR_CHECK_EQUAL]
        elif com.KEY_ACCESSOR_CHECK_NOTEQUAL in self._additional_data:
            method="not_equal"
            value = self._additional_data[com.KEY_ACCESSOR_CHECK_NOTEQUAL]
        elif com.KEY_ACCESSOR_CHECK_BITMASK in self._additional_data:
            method="bitmask"
            value = self._additional_data[com.KEY_ACCESSOR_CHECK_BITMASK]
            #------ do change bitmask
            #if com.KEY_ACCESSOR_DEP_TYPE not in self._additional_data:
                #method="bitmask"
                #value = self._additional_data[com.KEY_ACCESSOR_CHECK_BITMASK]
            #elif com.KEY_ACCESSOR_DEP_PROPERTY in self._additional_data:
                #method = "bitmask_dbus_call"
                #value =  f"{com.CURRENT_JSON_DEVICE_INDEX}:{self._additional_data[com.KEY_ACCESSOR_DEP_PROPERTY]}"
            #-----------
        elif com.KEY_ACCESSOR_CHECK_LOOKUP in self._additional_data:
            method="lookup"
            value=self._additional_data[com.KEY_ACCESSOR_CHECK_LOOKUP]

        self.set_injection_information(information)
        information["device_item"] = device
        information["method"] = method
        information["method_value"] = value
        information["accessor_type"] = accessor_type
        device_index = 0 # this index regards to object path expansion
        device_index_str = self.create_device_index_string_list()
        if len(objects_range) > 0:
            for device_item in objects_range:
                information["device_item"] = device_item
                if self.__store_command_information(information, device_index_str[device_index]) is True:
                   self.__store_in_expected_events_list(device_item)
                device_index = device_index + 1
        else:
            if self.__store_command_information(information, device_index_str[device_index]) is True:
               self.__store_in_expected_events_list(device)

    def generate_busctl_command_from_json_dict(self, device, data):
        """
        Redefines parent method
        Generates commands in the script if the accessor_type is not empty and is handled
        """
        super().parse_json_dict_data(device, data)
        should_generate_commands = True
        event = data["event"]
        event_key = com.DEVICE_HW + event
        self._unique_event = False
        if event_key not in self._event_unique_list:
            self._event_unique_list[event_key] = 1
            self._unique_event = True

        information = {}
        information["range"] = ""
        objects_range = []
        if com.KEY_ACCESSOR_OBJECT in self._additional_data:
            objects_range = com.expand_range(self._additional_data[com.KEY_ACCESSOR_OBJECT])
        elif com.KEY_EVENT_TRIGGER_OBJECT in self._additional_data:
             objects_range = com.expand_range(self._additional_data[com.KEY_EVENT_TRIGGER_OBJECT])

        ## Objects may have range and device_type not, but also device_type may have range and Objects not
        ignore_data = ""
        if len(objects_range) == 1:
            ignore_data = com.expand_range(data['device_type'])
        for single_range in com.range_start_end_list:
            if information["range"] != "":
                information["range"] += "/"
            information["range"] += f"[{single_range[0]}-{single_range[1]}]"

        if com.LIST_EVENTS is True or com.LIST_SINGLE_INJECTION_COMMANDS is True:
            should_generate_commands = False
            if self._unique_event is True:
                if com.LIST_EVENTS is True:
                    print(f"{com.DEVICE_HW}{information['range']} \"{event}\"")
                else:
                    self.print_test_single_injection(event, com.range_start_end_list)
        else:
            if len(com.SINGLE_DEVICE) > 0  and com.DEVICE_HW != com.SINGLE_DEVICE:
               should_generate_commands = False
            elif len(com.SINGLE_EVENT) > 0  and event != com.SINGLE_EVENT:
               should_generate_commands = False

        if should_generate_commands is True:
            accessor_type = self.__get_accessor_type()
            ## previous information["range"] may be used only in case it was for print commands
            ## lets check it again and clear if Objects path do not have range
            if  len(ignore_data) > 0:
                information["range"] = ""
            self.store_busctl_commands_as_dbus_accessor(device, accessor_type, information, objects_range)
        else:
            event_data = self._additional_data.copy()
            super().remove_accessor_fields(event_data)

        ## Block commented, removed skipping, everything is performed
        #--------------------------------------------------------------
        #if  accessor_type == com.ACCESSOR_TYPE_DBUS:
            #if self._additional_data[com.KEY_ACCESSOR_OBJECT].upper() == "TBD" or \
                  #self._additional_data[com.KEY_ACCESSOR_INTERFACE].upper() == "TBD" or \
                  #self._additional_data[com.KEY_ACCESSOR_PROPERTY].upper() == "TBD":
                #method="skip"
                #value="any DBUS accessor field = TBD"
                #self.__store_command_information(device, method, value)

            #else:
                #self.store_busctl_commands_as_dbus_accessor(device, accessor_type)
        #else:
            #method ="skip"
            #value = f"accessor.type = {accessor_type}"
            #self.__store_command_information(device, method, value, accessor_type)
        #--------------------------------------------------------------
        super().remove_accessor_fields()


    def print_test_single_injection(self, event, range_list):
        """
        Prints a command such as './test_AML.sh inject  -m -d PCIeRetimer -e "I2C EEPROM Error" -i 0'
        """
        index = "0"
        counter = 0
        for single_range in range_list:
            random.seed(self._seed)
            self._seed += 17
            number = random.randint(single_range[0], single_range[1])
            if counter > 0:
                index += "," + str(number)
            else:
                index = str(number)
            counter += 1;
        type_accessor = ""
        if com.KEY_EVENT_TRIGGER_TYPE in self._additional_data:
            type_accessor += f" event_trigger.type={self._additional_data[com.KEY_EVENT_TRIGGER_TYPE]}"
        if com.KEY_ACCESSOR_TYPE in self._additional_data:
            type_accessor += f" accessor.type={self._additional_data[com.KEY_ACCESSOR_TYPE]}"

        print(f"./test_AML.sh inject -m -d {com.DEVICE_HW} -e \"{event}\" -i {index}\t\t\t# {type_accessor}")
        print(f"./inject-event.sh  {com.DEVICE_HW} {index} \"{event}\" --markdown  \t\t\t# {type_accessor}")

    def generate_script_from_json(self):
        """
        Redefines from super

        Calls generate_script_from_json() from super which does not generate
          the commands using its empy close_script_file()
        Writes the total of commands to be executed, it allows having a percentage
        Then writes all the commands
        """
        super().generate_script_from_json()
        total_commands = len(self._commands_information_list)
        unique_events = len(self._event_unique_list)
        if total_commands > 0:
            if self._max_commands > 0 and  total_commands > self._max_commands:
                total_commands = self._max_commands
            commands_number = f"\ngl_total_comands={total_commands}\ngl_unique_events={unique_events}\n"
            commands_number += "[ $gl_cmd_line_unique_event -eq 1 ] && gl_total_comands=$gl_unique_events\n"
            super().write(commands_number)
            for cmd_info in self._commands_information_list:
                self.__write_busctl_commands_into_script(cmd_info)
                total_commands -= 1
                if total_commands == 0:
                    break

            bottom ="\n\nfinish_test ## release/clear everything\n\nexit $gl_rc"
            super().priv_close_script_file(bottom)
            self._event_unique_list.clear()
            self._commands_information_list.clear()



