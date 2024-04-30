#!/usr/bin/python3

"""
Event Inject tests
"""

# --------------------------------------------------------------------------
# This script injects events using event_injector script and verify
# log creation by BMCWeb
#
# Copyright (c) 2021, NVIDIA CORPORATION.  All rights reserved.
#
# NVIDIA CORPORATION and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA CORPORATION is strictly prohibited.
import sys
import tempfile
import os
import shutil
import json

from getpass import getpass
from shlex import split
from time import sleep
import argparse
from requests import get
from requests.auth import HTTPBasicAuth
import paramiko

from termcolor import colored

import urllib3
urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)

## add modules sub directory from
modules_dir = os.path.dirname(__file__)
modules_dir_path = f"{modules_dir}/modules" if len(modules_dir) > 0 else "./modules"
sys.path.append(modules_dir_path)
import com ## common constants and and functions
from event_logging_script  import EventLogsInjectorScript
from event_accessor_script import EventAccessorInjectorScript



# Global variables used to access BMCWEB
BMCWEB_IP=""
BMCWEB_PORT=None

# Global variables used to access QEMU
QEMU_IP=""
QEMU_USER=""
QEMU_PASS=""
QEMU_PORT=None

JSON_FILE="event_info.json"
DAT_FILE="dat.json"
JSON_REMOTE="/usr/share/mon_evt"
JSON_REMOTE_PATHNAME=f"{JSON_REMOTE}/{JSON_FILE}"
JSON_REMOTE_FILE=f"<QEMU_USER>@<QEMU_IP>:<QEMU_PORT>{JSON_REMOTE_PATHNAME}/{JSON_FILE}"
DAT_REMOTE_FILE=f"{JSON_REMOTE}/{DAT_FILE}"
GENERATE_SCRIPT_ONLY = False

# Global variables to access event injector script
EVENT_INJ_SCRIPT_NAME="event_injector.bash"
EVENT_INJ_SCRIPT_PATH=f"/home/root/{EVENT_INJ_SCRIPT_NAME}"
EVENT_INJ_TMP_SCRIPT_PATH=f"/tmp/mon_evt/bin/{EVENT_INJ_SCRIPT_NAME}"
EVENT_INJ_SCRIPT_ARGS=""
JSON_EVENTS_FILE = None

TEST_MODE_EVENTS_LOGGING=1
TEST_MODE_CHANGE_DEVICE_STATUS=2
TEST_MODE=TEST_MODE_EVENTS_LOGGING
NOT_GENERATE_MESSAGE_ARGS = False

RUN_SCRIPT  = True  ## will not run depending on some options
CLEAR_LOGS  = True  ## clear logs at beginning not necessary to fetch initial logs

CLEAR_LOGS_COMMAND = "busctl call xyz.openbmc_project.Logging /xyz/openbmc_project/logging xyz.openbmc_project.Collection.DeleteAll DeleteAll"

com.MODULES_DIR   = modules_dir_path
com.TEMPLATES_DIR = com.MODULES_DIR + "/" + "templates"

available_options = [
    {
        'args': [ '-p', '--passwd'],
        'kwargs': {
            'type': str,
            'default': None,
            'help': '''Specify Password to be used for QEMU and BMCWeb login.'''
        }
    },
    {
        'args': [ '-m', '--mode'],
        'kwargs': {
            'type': int,
            'default': 2,
            'help': '''[optional] Test Mode to perform; 1=insert event logs; 2=change device status, requires AML daemon
                    (which should insert event logs)
                    '''
        }
    },
    {
        'args': [ '--bmcweb-ip'],
        'kwargs': {
            'type': str,
            'default': '127.0.0.1',
            'help': '''[optional] Specify IP address to connect to BMCWeb.'''
        }
    },
    {
        'args': [ '--bmcweb-port'],
        'kwargs': {
            'type': int,
            'default': 2080,
            'help': '''[optional] Specify Port to connect to BMCWeb.'''
        }
    },
    {
        'args': [ '--qemu-ip'],
        'kwargs': {
            'type': str,
            'default': '127.0.0.1',
            'help': '''[optional] Specify IP address to connect to QEMU.'''
        }
    },
    {
        'args': [ '--qemu-port'],
        'kwargs': {
            'type': int,
            'default': 2222,
            'help': '''[optional] Specify Port to connect to QEMU.'''
        }
    },
    {
        'args': [ '-u', '--user' ],
        'kwargs': {
            'type': str,
            'default': 'root',
            'help': '''[optional] Specify Username to be used for QEMU and BMCWeb login.'''
        }
    },
    {
        'args': [ '-j', '--json' ],
        'kwargs': {
            'type': str,
            'default': JSON_REMOTE_FILE,
            'help': '''[optional] A json file with events to be injected.'''
        }
    },
    {
        'args': [ '-l', '--list-events-only' ],
        'kwargs': {
            'action': 'store_true',
            'help': '''[optional] Prints the event list and exit.'''
        }
    },
    {
        'args': [ '-g', '--generate-script-only' ],
        'kwargs': {
            'action': 'store_true',
            'help': '''[optional] Just generates the bash Injector script on /tmp and exits.'''
         }
    },
    {
        'args': [ '--list-single-injection-commands-only' ],
        'kwargs': {
            'action': 'store_true',
            'help': '''[optional] Just prints the commands for having a single-injection for all events.'''
         }
    },
    {
        'args': [ '-d', '--device' ],
        'kwargs': {
            'type': str,
            'default': '',
            'help': '''[optional] Perform injections only for a specific device, see also ----list-events-only and --event.'''
        }
    },
    {
        'args': [ '-e', '--event' ],
        'kwargs': {
            'type': str,
            'default': '',
            'help': '''[optional] Perform injections only for a specific event, see also ----list-events-only and --device.'''
        }
    },
    {
        'args': [ '-i', '--device-index' ],
        'kwargs': {
            'type': str,
            'default': '',
            'help': '''[optional] Perform injections only for a specific device INDEX since the EVENT has range (use zero if it has not range), see also ----list-events-only and --event.'''
        }
    },
    {
        'args': [ '--keep-existent-logs' ],
        'kwargs': {
            'action': 'store_true',
            'help': '''[optional] If this option is specified, Phosphor Log entries will NOT be cleared before injecting failures.'''
        }
    },
    {
        'args': [ '--second-range-limit'],
        'kwargs': {
            'type': int,
            'default': 1000,
            'help': '''[optional] Specify a limit to expand the second range, the default value is large enougth to expand all, example /NVSwitch_[0-3]/Ports/NVLink_[0-39] generates 160 items'''
        }
    },
    {
        'args': [ '--unique-event' ],
        'kwargs': {
            'action': 'store_true',
            'help': '''[optional] Performs injections only for the first occurrence from each event (does not walk through ranges)'''
        }
    },
    {
        'args': [ '--dry-run' ],
        'kwargs': {
            'action': 'store_true',
            'help': '''[optional] If this option is specified, no event will be injected.'''
         }
    },
]


def avoid_unused_variable(variable):
    """
    It is used just to avoid pyling messages 'unused-variable', it does nothing
    """
    local_var = variable
    variable  = local_var


def init_arg_parser():
    """
    Initialize argument parser
    """
    msg  = "Event Injection Test Automation: mode=1 by \'inserting event logs\' "
    msg += ";  mode=2 by \'changing devices status (test for complete flow)\'"
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter,\
                                    description=msg)
    for opt in available_options:
        parser.add_argument(*opt['args'], **opt['kwargs'])
    return parser


def parse_arguments():
    """
    Initialize argument parser.
    Set all the Global variables based on the arguments
    Returns True if arguments are OK
    """
    parser = init_arg_parser()

    global QEMU_USER, QEMU_PASS, QEMU_IP, QEMU_PORT, BMCWEB_IP, BMCWEB_PORT, \
           JSON_EVENTS_FILE, TEST_MODE, EVENT_INJ_SCRIPT_ARGS, RUN_SCRIPT, CLEAR_LOGS, GENERATE_SCRIPT_ONLY

    args, remaining = parser.parse_known_args()
    avoid_unused_variable(remaining)

    TEST_MODE=args.mode

    if TEST_MODE < TEST_MODE_EVENTS_LOGGING or TEST_MODE > TEST_MODE_CHANGE_DEVICE_STATUS:
        print(f"Mode option -m|--mode {TEST_MODE} out of range, use -h to see valid mode options")
        return False

    # QEMU Access info
    QEMU_USER= args.user
    QEMU_PASS=args.passwd
    QEMU_IP=args.qemu_ip
    QEMU_PORT=args.qemu_port

    # BMCWeb access info
    BMCWEB_IP=args.bmcweb_ip
    BMCWEB_PORT=args.bmcweb_port

    JSON_EVENTS_FILE=args.json

    GENERATE_SCRIPT_ONLY = args.generate_script_only
    if GENERATE_SCRIPT_ONLY is True:
       RUN_SCRIPT = False

    com.LIST_SINGLE_INJECTION_COMMANDS = args.list_single_injection_commands_only
    com.LIST_EVENTS = args.list_events_only
    if com.LIST_EVENTS is True or com.LIST_SINGLE_INJECTION_COMMANDS is True:
        RUN_SCRIPT = False

    com.SINGLE_DEVICE = args.device
    com.SINGLE_EVENT  = args.event
    com.SINGLE_DEVICE_INDEX = args.device_index

    if len(com.SINGLE_DEVICE) > 0 or len(com.SINGLE_EVENT) or \
       len(com.SINGLE_DEVICE_INDEX) :
          com.CUSTOM_EVENT = True

    # Check for dry run
    if args.dry_run:
        EVENT_INJ_SCRIPT_ARGS = f"{EVENT_INJ_SCRIPT_ARGS} --dry-run"

    CLEAR_LOGS = not args.keep_existent_logs
    if args.keep_existent_logs != True:
         EVENT_INJ_SCRIPT_ARGS += " --keep-existent-logs"

    if args.passwd is None and (RUN_SCRIPT is True or JSON_EVENTS_FILE == JSON_REMOTE_FILE):
         getpass(prompt='Password: ')

    if args.second_range_limit > 0:
        com.DOUBLE_EXPANSION_LIMIT = args.second_range_limit ## otherwise leave default value in com.py

    if args.unique_event is True:
        EVENT_INJ_SCRIPT_ARGS += " --unique-event"

    return True


class InjectTest:
    """
    This class is used to:
        1.  Get all the logs already present using BMCWeb redfish interface, using API call.
        2.  Inject events into D-Bus using "event_injector" script.
        3.  Get all the logs generated after Step 2 and
              verify all the logs with corresponding events.
    """

    def __init__(self):
        """
        self.initial_log_count          -   Number of logs before event injection
        self.total_events               -   Total number of events
        self.events_injected_count      -   Number of events injected
        self.events_injected            -   Information of all the events
        self.final_log_count            -   Number of logs after event injection
        self.log_cache                  -   Cache of Log Entry API
        """
        self.initial_log_count=0
        self.total_events=0
        self.events_injected_count=0
        self.events_injected = {}
        self.final_log_count=0
        self._ssh_cmd = None
        self.log_cache=None
        self.unverified_events = 0


    def clear_logs(self):
       """
       Clear Logs at beginning
       """
       self.create_ssh_session()
       try:
         # Execute the command to invoke event_injector script
         channel = self._ssh_cmd.get_transport().open_session()
         channel.exec_command(CLEAR_LOGS_COMMAND)
         channel.set_combine_stderr(True)

       except Exception as error:
            msg = f"Exception occurred while running command {CLEAR_LOGS_COMMAND}"
            raise Exception(f"{msg} over ssh: {str(error)}") from error


    def get_asserted_device(self, entryId):
       """
       """
       real_device = ""
       pipes_extract = " | cut -f3-550 -d' ' | awk -F 'namespace='  '{print $2}' | tr -d '\"' "
       cmd = f"{com.PHOSPHOR_LOG_BUSCTL}/{entryId} {com.PHOSPHOR_LOG_INTERFACE} {com.PHOSPHOR_LOG_ADDITIOANAL_DATA} {pipes_extract}"
       try:
            # Execute the command to invoke event_injector script
            channel = self._ssh_cmd.get_transport().open_session()
            channel.exec_command(cmd)
            channel.set_combine_stderr(True)
            while True:
                out = channel.recv(256).decode("utf-8")
                real_device += out
                if channel.exit_status_ready():
                    break
       except Exception as error:
            msg = f"Exception occurred while running command {cmd}"
            raise Exception(f"{msg} over ssh: {str(error)}") from error

        # Check stdout return code
       if channel.recv_exit_status():
            msg  = f"Exception occurred while running command {cmd} over ssh:"
            msg += f"\n{''.join(real_device)}"
            raise Exception(msg)

       return real_device.rstrip()


    def get_device_chassis_health_rollup(self, device):
       """

       """
       health_rollup = ""
       event_log_entries_api= f"http://{BMCWEB_IP}:{BMCWEB_PORT}/{com.DEVICE_CHASSIS_URI}/{device}"
       try: # Call the API
            response = get(event_log_entries_api, verify=False,
                              auth = HTTPBasicAuth(QEMU_USER, QEMU_PASS))
            if response.status_code == 200:
               health_rollup = response.json()
       except Exception as error:
            msg = f"Exception occurred while converting response to json: {str(error)}"
            raise Exception(f"{msg}\n Response: {response.text}") from error

       return health_rollup


    def current_log_count(self, cache=False):
        """
        Uses redfish API to get the Log entries.
        Uses key 'Member@odata.count' to get the current count.
        Cache the redfish API response if argument "cache" is set to "True"
        """
        event_log_entries_api= f"http://{BMCWEB_IP}:{BMCWEB_PORT}/{com.EVENT_LOG_URI}"
        try: # Call the API
            response = get(event_log_entries_api, verify=False,
                                    auth = HTTPBasicAuth(QEMU_USER, QEMU_PASS))
        except Exception as error:
            msg = f"Exception occurred while making API({event_log_entries_api})"
            raise Exception(f"{msg} call: {str(error)}") from error
        try:
            # Convert the API response into JSON format
            response_json=response.json()
        except Exception as error:
            msg = f"Exception occurred while converting response to json: {str(error)}"
            raise Exception(f"{msg}\n Response: {response.text}") from error
        # Get the 'Members@odata.count' key to read the log count
        key_members = "Members"
        members = response_json.get(key_members, None)
        if members is None:
            raise Exception(f"Exception occurred while reading key: '{members}'")
        # Cache the API response if argument is set to True
        if cache:
            try:
                self.log_cache=response_json.copy()
            except Exception as error:
                msg = f"Exception occurred while copying API response json: {str(error)}"
                raise Exception(msg) from error
        log_count = 0
        for member in members:
            log_count = max(log_count, int(member['Id']))

        return log_count


    def collect_logs_before_injections(self):
        """
        Fetch current number of logs.
        """
        if RUN_SCRIPT is True:
            print("\nFetching current logs....")
            try:
                  self.initial_log_count = self.current_log_count()
            except Exception as error:
                  raise error


    def close_ssh_session(self):
       """
       Closes the ssh session if it is open
       """
       if self._ssh_cmd is not None:
          self._ssh_cmd.close()


    def create_ssh_session(self):
        """
        Creates a ssh session
        """
        if self._ssh_cmd is None:
           try:
              self._ssh_cmd = paramiko.SSHClient()
              # Set missing host key policy
              self._ssh_cmd.set_missing_host_key_policy(paramiko.AutoAddPolicy())
              # Connect the SSH client to QEMU
              self._ssh_cmd.connect(QEMU_IP, username=QEMU_USER, port=QEMU_PORT, \
                              password=QEMU_PASS, timeout=60)
           except Exception as error:
              msg = f"Exception occurred while connecting to {BMCWEB_IP} {QEMU_PORT}"
              raise Exception(f"{msg}: {str(error)}") from error


    def sftp_script_to_emulator(self, source_script):
        """
        Just copies the script to emulator
        """
        try:
            sftp =  self._ssh_cmd.open_sftp()
            msg = f"Copying {source_script} to {QEMU_USER}@{BMCWEB_IP}:{EVENT_INJ_SCRIPT_PATH}"
            print(f"...\n\n{msg} using sftp....")
            sftp.put(source_script, EVENT_INJ_SCRIPT_PATH)
            try:
                stat_return = sftp.lstat(EVENT_INJ_TMP_SCRIPT_PATH);
                msg = f"Copying {source_script} to {QEMU_USER}@{BMCWEB_IP}:{EVENT_INJ_TMP_SCRIPT_PATH}"
                print(f"...\n\n{msg} using sftp....")
                sftp.put(source_script, EVENT_INJ_TMP_SCRIPT_PATH)
            except Exception:
                pass
            sftp.close()
        except Exception as error:
            msg = f"Exception occurred while copying {source_script} using sftp"
            raise Exception(f"{msg}: {str(error)}") from error


    def execute_remote_script_and_get_stdout(self):
        """
        Runs the remote script and gets its output
        Returns its output
        """
        event_inject_cmd = f"/bin/bash {EVENT_INJ_SCRIPT_PATH} {EVENT_INJ_SCRIPT_ARGS}"
        result_stdout=""
        print("...\n\nInjecting new events....\n...")
        try:
            # Execute the command to invoke event_injector script
            channel = self._ssh_cmd.get_transport().open_session()
            channel.exec_command(event_inject_cmd)
            channel.set_combine_stderr(True)
            while True:
                if channel.exit_status_ready():
                    break
                out = channel.recv(256).decode("utf-8")
                print (out, end='')
                result_stdout += out
        except Exception as error:
            msg = f"Exception occurred while running command {event_inject_cmd}"
            raise Exception(f"{msg} over ssh: {str(error)}") from error

        # Check stdout return code
        if channel.recv_exit_status():
            msg  = f"Exception occurred while running command {event_inject_cmd} over ssh:"
            msg += f"\n{''.join(result_stdout)}"
            raise Exception(msg)
        return result_stdout.splitlines()

    def parse_event_logging_output_get_events_injected(self, result_stdout):
        """
        1. reads stdout and inserts data in the dict self.events_injected
        2. self.events_injected keys are new 'events Ids' supposed as created
           these 'events Ids' are greater than the number of logs found before injection
        3. sets self.total_events          -> lines rom stdout containing a 'busctl' commands

        self.events_injected looks like:

        "GPU0 OverT": {
            "msg_id" : "ResourceEvent.1.0.ResourceErrorsDetected,
            "severity": "xyz.openbmc_project.Logging.Entry.Level.Critical",
            "msg_args": "["GPU0_Temp", "Overheat"]",
            "resolution": "Contact NVIDIA Support"
        },
        """
        current_log_number = self.initial_log_count # number of logs found before injection
        for event in result_stdout:
            # Proceed only if it is a busctl command
            if "busctl" not in event:
                continue
            # Split the busctl command and cache all the necessary fields/values
            try:
                event_split = split(str(event))
                current_log_number+=1
                if current_log_number in self.events_injected:
                    print(f"Duplicate event: {current_log_number}")
                else:
                    self.events_injected[current_log_number] = []
                # Create a temp dictionary to fill in the info
                temp_dict = {}
                temp_str = event_split[com.BUSCTL_MSG_IDX]
                temp_str.replace('\"', "");
                words = temp_str.split(' ')
                temp_dict['device'] = words.pop(0)
                temp_dict['event']  = ' '.join(words)
                temp_dict[com.KEY_SEVERITY]  = event_split[com.BUSCTL_SEVERITY_IDX]
                additional_data_idx = com.BUSCTL_ADDITIONALDATA_IDX
                while additional_data_idx < len(event_split):
                    key = com.is_event_mandatory_key(event_split[additional_data_idx])
                    if key is None:
                        key = com.is_event_optional_key(event_split[additional_data_idx])
                    if key is not None:
                        temp_dict[key] = event_split[additional_data_idx + 1]
                    additional_data_idx += 2
                # Increment total number of events
                self.total_events+=1
                temp_temp_dict = temp_dict.copy()
                self.events_injected[current_log_number] = temp_temp_dict
            except Exception as error:
                msg = f"Unable to get field from busctl command {event}"
                raise Exception(f"{msg}:  {str(error)}") from error


    def get_events_list_from_json_file(self, result_stdout, event_logs_script):
        """
        This function is similar to parse_event_logging_output_get_events_injected()
        Inserts data in the dict self.events_injected, to be compared to redfish data
        Sets self.total_events
        """
        current_log_number = self.initial_log_count # number of logs found before injection
        self.supposed_injected_events = event_logs_script.get_accessor_dbus_expected_events_list()
        self.total_events = len(self.supposed_injected_events);
        for line in result_stdout:
            # failed    -> "[debug: 63/100] failed EVENT='VR Fault DVDD' : 'busctl command failed'
            # successful-> "[debug: 61/100] injected EVENT='PCIe link error' : 'created EventLogId [ 28 ]"
            if "[debug:" in line and "injected" in line:
                    words = line.split(' ')
                    index_event= int(words[1].split('/')[0]) - 1
                    event_id=0
                    counter = 3
                    while counter < len(words):
                        if words[counter] == "EventLogId":
                            event_id = int(words[counter+2])
                            break
                        counter += 1
                    self.events_injected[event_id] = self.supposed_injected_events[index_event]
                    current_log_number += 1
        # difference are those not injected
        self.unverified_events =  self.total_events - len(self.events_injected)


    def copy_file_to_tmp_dir(self, src_filename, destination_name):
        """
        A file that already exists will be copied into /tmp as /tmp/{destination_name}
        """
        tmp_filename = tempfile.gettempdir() + "/" + destination_name
        if os.path.exists(tmp_filename):
            os.unlink(tmp_filename)
        print(f"...\n\nSaving used {destination_name} file as {tmp_filename}\n...")
        shutil.copyfile(src_filename, tmp_filename)
        os.chmod(tmp_filename, 0o666);

    def generate_script_from_json(self):
        """
        Generates the script, uses the Json file passed in command line or gets
         from QEMU using sftp
        """
        global JSON_EVENTS_FILE, JSON_REMOTE_FILE, DAT_REMOTE_FILE
        json_filename = JSON_EVENTS_FILE
        json_fd_temp = 0
        is_remote = 0
        remote_file = JSON_REMOTE_PATHNAME
        if JSON_EVENTS_FILE == JSON_REMOTE_FILE:
            try:
                self.create_ssh_session()
                is_remote = 1
                JSON_EVENTS_FILE=f"{QEMU_USER}@{QEMU_IP}:{QEMU_PORT}{JSON_REMOTE_PATHNAME}"
                sftp =  self._ssh_cmd.open_sftp()
                # scpo event_info.json into current machine
                json_fd_temp, json_file_temp = tempfile.mkstemp()
                os.close(json_fd_temp)
                json_fd_temp = open(json_file_temp, 'w', encoding="utf-8")
                sftp.get(JSON_REMOTE_PATHNAME, json_file_temp);

                ## scp dat.json into current machine
                remote_file = DAT_REMOTE_FILE
                dat_fd_temp, dat_file_temp = tempfile.mkstemp()
                os.close(dat_fd_temp)
                dat_fd_temp = open(dat_file_temp, 'w', encoding="utf-8")
                sftp.get(DAT_REMOTE_FILE, dat_file_temp);

                sftp.close()
                json_filename = json_file_temp
                ## copy temp dat.json to /tmp/dat.json
                self.copy_file_to_tmp_dir(dat_file_temp, DAT_FILE)

            except Exception as error:
                msg = f"Unable to get jon file from {remote_file} "
                raise Exception(f"{msg}:  {str(error)}") from error

        msg  = f"Parsing Json file {JSON_EVENTS_FILE} and generating event injector "
        msg += f"bash script for mode={TEST_MODE}...."
        print(f"...\n\n{msg}\n")

        event_logs_script = EventLogsInjectorScript(json_filename, NOT_GENERATE_MESSAGE_ARGS) \
            if TEST_MODE == TEST_MODE_EVENTS_LOGGING else \
                EventAccessorInjectorScript(json_filename)
        event_logs_script.generate_script_from_json()

        ## always create a copy of event_info.json in /tmp, useful to export the file when using a remote HMC
        self.copy_file_to_tmp_dir(json_filename, JSON_FILE)


        if json_fd_temp != 0 and is_remote != 0:
            os.unlink(json_filename)

        if GENERATE_SCRIPT_ONLY is True:
            self.close_ssh_session()
            source_script = event_logs_script.script_file()
            destination = tempfile.gettempdir() + "/" + EVENT_INJ_SCRIPT_NAME
            print(f"...\n\nGenerated Bash Injector Script as {destination}\n...")
            shutil.copyfile(source_script, destination)

        return event_logs_script


    def inject_events(self, event_logs_script):
        """
        *  Inject events using event_injector script over SSH
        *  Check stdout, stderr for any errors/exceptions
        *  Read all the busctl commands injected by the script and
            cache useful data in dictionaries
        """
        try:
            # Create the SSH client, iif it is not created yet
            self.create_ssh_session()
            self.sftp_script_to_emulator(event_logs_script.script_file())
            result_stdout = self.execute_remote_script_and_get_stdout()
                # reads the last output line which should have a 'Successful Injections: 23'
            try:
                index=-1
                while index > -4:  # try a couple on lines at bottom
                    if 'Successful Injections:' in result_stdout[index]:
                        str_injected = result_stdout[index].strip().split(':')[-1].strip()
                        self.events_injected_count = int(str_injected)
                        break
                    index += -1
            except Exception as error:
                msg=f"Exception occurred while reading successful injections count: {str(error)}"
                raise Exception(msg) from error
            if TEST_MODE == TEST_MODE_EVENTS_LOGGING:
                self.parse_event_logging_output_get_events_injected(result_stdout)
            elif TEST_MODE == TEST_MODE_CHANGE_DEVICE_STATUS:
                self.get_events_list_from_json_file(result_stdout, event_logs_script)
        except Exception as error:
            raise error


    def collect_logs_after_injection_and_verify(self):
        """
        *  Gather all the logs generated after event injection
        *  Compare the logs information with the injected_events information
             cached in the inject_events() method
        """
        print("Fetching new logs....\n...")

        # If no event is injected, exit
        if self.events_injected_count == 0:
            raise Exception("No events injected, exiting!!")

        try: # Gather all the logs using BMCWeb interface
            self.final_log_count = self.current_log_count(cache=True)
        except Exception as error:
            raise error

        out = ""
        # Iterate over all the log entries
        print(f"\nComparing injections with redfish logs, fields: {com.MANDATORY_EVENT_KEYS} {com.OPTIONAL_EVENT_KEYS}\n...\n")
        for member in self.log_cache.get('Members', []):
            try:
                # Check if log entry number is greater than the initial logs count before injection
                if int(member['Id']) in self.events_injected:
                    log_id = int(member['Id'])
                    temp_dict = self.events_injected[log_id]
                    real_device = self.get_asserted_device(member['Id'])
                    # Compare all the fields
                    if 'MessageArgs' in member:
                         print("entryId=%03d device=%-14s event='%-50s' MessageArgs=%s" % (log_id, real_device, temp_dict['event'], member['MessageArgs']))
                    else:
                        print("entryId=%03d device=%-14s event='%-50s'" % (log_id, real_device, temp_dict['event']))
                    if com.CUSTOM_EVENT is True:
                        print(json.dumps(member, indent=4))
                        print()
                        health_rollup = self.get_device_chassis_health_rollup(real_device)
                        if len(health_rollup) == 0 and real_device.startswith("HGX_") is False:
                           attempty = "HGX_" + real_device
                           health_rollup = self.get_device_chassis_health_rollup(attempty)
                        if len(health_rollup) > 0:
                           print(json.dumps(health_rollup, indent=4))
                           print()

                    mandatory_fields_match = com.compare_mandatory_event_fields(temp_dict, member)
                    optional_fields_match = com.compare_optional_event_fields(temp_dict, member)
                    if mandatory_fields_match is True and optional_fields_match is True:
                        del self.events_injected[log_id]
                    else:
                        log_id = 0 # just a debug break point
            except Exception as error:
                message = "Exception occurred while matching keys for event id"
                out = f"{message} :{log_id}: {str(error)}\n"
                raise error

        # If the dictionary still have some info,
        # that means we did not find logs for some events
        if len(self.events_injected) > 0:
            # Append the info in the output string and raise exception
            out = f"{out}\nUnable to verify log generation for following events:\n"
            event_key_list = list(self.events_injected)
            for key in event_key_list:
                if len(self.events_injected[key]) > 0:
                    out = f"{out}\t* {key}\n"
                    self.unverified_events += 1
            raise Exception(f"Exception occurred: {out}")


    def print_summary(self):
        """
        *  Print the summary of the entire process
        Example:
        ====================
        SUMMARY:
            Total events: 6
            Events injected:  5
            Unverified events:  1
        """

        print("\n====================")
        print("SUMMARY:")
        print(f"\tTotal events: {self.total_events}")

        color = 'red' if self.events_injected_count < self.total_events else 'green'
        print("\tEvents injected: ", colored(f"{self.events_injected_count}", color))

        color = 'red' if self.unverified_events > 0 else 'green'
        print("\tUnverified events: ", colored(f"{self.unverified_events}", color))

        self.close_ssh_session()
        return self.unverified_events


def main():
    """
    *  Initialize argument parser
    *  Collect logs before injection
    *  Inject events
    *  Collect logs and verify after injection
    *  Print summary
    """

    # Initalize arg parser and add all the arguments
    arguments_ok = parse_arguments()

    main_rc = os.EX_OK if arguments_ok is True else -1

    try:
        if main_rc == os.EX_OK:
            inject_test = InjectTest()
            event_logs_script= None
            if RUN_SCRIPT is True or GENERATE_SCRIPT_ONLY is True or com.LIST_EVENTS is True or \
              com.LIST_SINGLE_INJECTION_COMMANDS is True:
                event_logs_script = inject_test.generate_script_from_json()
            if RUN_SCRIPT is True:
               if CLEAR_LOGS is True:
                  inject_test.clear_logs()
             #  inject_test.collect_logs_before_injections()
               inject_test.inject_events(event_logs_script)

                ### Comparing fields such as MessageArgs and Severity no longer makes sense,
               # if TEST_MODE == TEST_MODE_EVENTS_LOGGING:
               #  sleep(2)
               #  inject_test.collect_logs_after_injection_and_verify()
    except Exception as error:
        print(colored(error, 'red'))
        main_rc = -1
        raise error

    finally:
        # for mode=2 Bash script now prints a summary
        if TEST_MODE == TEST_MODE_EVENTS_LOGGING:
            if main_rc == os.EX_OK and inject_test.print_summary() == 0:
                main_rc = os.EX_OK
            else:
                main_rc = -1

    return main_rc


if __name__ == "__main__":
    sys.exit(main())  # OS status will have 0 status if this test runs OK and passes
