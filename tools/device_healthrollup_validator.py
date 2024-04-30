#!/usr/local/bin/python3

# Copyright (c) 2024, NVIDIA CORPORATION.  All rights reserved.
#
# NVIDIA CORPORATION and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA CORPORATION is strictly prohibited.

import argparse
import json
import sys
import requests
from requests.auth import HTTPBasicAuth
import paramiko
import time
import urllib3
urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)

parser = argparse.ArgumentParser(
        description="Device Health Rollup Validator",
        usage='./device_healthrollup_validator.py [help]',
        )

parser.add_argument("-a", "--association_tree", action="store", dest="profile", type=str, nargs="?", default="association_tree.json")
parser.add_argument("-i", "--ip", action="store", dest="ip", type=str, nargs="?")
parser.add_argument("-u", "--user", action="store", dest="user", type=str, nargs="?", default="root")
parser.add_argument("-p", "--password", action="store", dest="password", type=str, nargs="?", default="0penBmc")
parser.add_argument("-q", "--qemu_port", action="store", dest="port", type=int, nargs="?", default=2222)
parser.add_argument("-r", "--redfish_port", action="store", dest="redfish_port", type=int, nargs="?", default=2443)
parser.add_argument("-e", "--editor_script", action="store", dest="editor", type=str, nargs="?", default="device_association_editor")
parser.add_argument("-d", "--dump", action="store_true", dest="dump")
parser.add_argument("-v", "--verbose", action="store_true", dest="verbose")

if len(sys.argv) < 2:
    parser.print_help()
    sys.exit(0)

args = parser.parse_args()

dat = None
try:
    profile = open(args.profile)
    dat = json.load(profile)
except Exception as e:
    print(e)

if args.dump:
    print("IP: {}".format(args.ip))
    print("Username: {}".format(args.user))
    print("Password: {}".format(args.password))
    print("Port: {}".format(args.port))
    def print_dat():
        if dat is None:
            print("No DAT provided")
            return
        print("\n########## Device Association Tree ##########\n")
        for device in dat:
            print("device: {}".format(str(device)))
            for child in dat[device]:
                print("\tchild: {}".format(str(child)))
    print_dat()

def qemu_ssh_handle(ip, user, pwd, port):
    try:
        handle = paramiko.SSHClient()
        handle.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        handle.connect(ip, username=user, port=port, password=pwd, timeout=10, look_for_keys=False)
    except Exception as e:
        raise Exception(str(e))
    return handle

def get_redfish_object(ip, user, pwd, port, device):
    redfish_path = "redfish/v1/Chassis/" + device
    path = "https://{}:{}/{}".format(ip, port, redfish_path)
    response = requests.get(path, verify=False, auth=HTTPBasicAuth(user, pwd), timeout=10)
    return response.json()

# traverses DAT
# for each pair of (upstream, downstream):
#   edit health status
#   check if healthrollup of upstream changed
#   revert the health status to what it was before
def traverse_dat(ip, user, pwd, qemu_port, bmcweb_port):
    if dat is None:
        return
    handle = None
    try:
        handle = qemu_ssh_handle(ip, user, pwd, qemu_port)
    except Exception as e:
        print("Could not establish QEMU connection. Will not edit device associations")
        print(str(e) + "\n")
        return

    def execute_fn(handle, cmd):
        stdin, stdout, stderr = handle.exec_command(cmd, get_pty=True)
        result = "\n{} {}\n".format(" ".join(stdout.readlines()), " ".join(stderr.readlines()))
        return result

    n = 1
    passes, fails, skipped = 0, 0, 0
    statuses = ["Critical", "Warning"]
    ptr = 0
    for device in dat:
        downstream = dat[device]
        for dev in downstream:
            expected = statuses[ptr % len(statuses)]
            print("Test #{}".format(n))
            print("Editing association for target {} and associated device {}".format(device, dev))
            print("Setting health status to {}".format(expected))
            n += 1
            cmd_fmt = "./{} -f {} -t {} -a {}".format(args.editor, args.profile, device, dev)
            cmd = "{} -h {}".format(cmd_fmt, expected)
            revert_cmd = "{} -h {}".format(cmd_fmt, "OK")
            result = execute_fn(handle, cmd)
            if args.verbose:
                print(result)

            retry_count = 3
            failure = False
            while retry_count > 0:
                try:
                    resp = get_redfish_object(ip, user, pwd, bmcweb_port, device)
                    if "Status" in resp and "HealthRollup" in resp["Status"]:
                        break
                except Exception as e:
                    print("Failed to get redfish object for device {}: {}\n".format(device, str(e)))
                    if retry_count == 1:
                        failure = True
                retry_count -= 1
                time.sleep(10)

            if failure:
                skipped += 1
                execute_fn(handle, revert_cmd)
                continue

            healthrollup = resp["Status"]["HealthRollup"]
            print("Health rollup for upstream device {}: {}".format(device, healthrollup))
            print("Expected: {}".format(expected))

            if healthrollup.lower() == expected.lower():
                print("Result: PASS")
                passes += 1
            else:
                print("Result: FAIL")
                fails += 1
            print()

            revert_result = execute_fn(handle, revert_cmd)
            if args.verbose:
                print("Reverting previous edit command")
                print(revert_result)

            ptr += 1

        print()
    return passes, fails, skipped


passes, fails, skipped = traverse_dat(args.ip, args.user, args.password, args.port, args.redfish_port)
print("\nSummary:")
print("{} tests passed".format(passes))
print("{} tests failed".format(fails))
print("{} tests skipped\n".format(skipped))

# exit code of script is # of failures
sys.exit(fails)

