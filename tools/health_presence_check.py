#!/usr/bin/env python

# Copyright (c) 2024, NVIDIA CORPORATION.  All rights reserved.
#
#  NVIDIA CORPORATION and its licensors retain all intellectual property
#  and proprietary rights in and to this software, related documentation
#  and any modifications thereto.  Any use, reproduction, disclosure or
#  distribution of this software and related documentation without an express
#  license agreement from NVIDIA CORPORATION is strictly prohibited.


import paramiko
import string
import sys

if __name__ == "__main__":
    if len(sys.argv) < 6:
        print("Too few arguments specified, quitting")
        print("Usage:")
        print("python health_presence_check.py <hostname> <port> <username> <password> <device association tree file> [<output>]")
        sys.exit()

    if len(sys.argv) > 7:
        print("Too many arguments specified, quitting")
        print("Usage:")
        print("python health_presence_check.py <hostname> <port> <username> <password> <device association tree file> [<output>]")
        sys.exit()

    address = sys.argv[1]

    try:
        port = int(sys.argv[2])
    except BaseException as err:
        print(f'Port (second argument) must be an integer')
        sys.exit()

    username = sys.argv[3]
    password = sys.argv[4]
    dev_assoc_tree_filename = sys.argv[5]

    devices = set()
    try:
        dev_assoc_tree_file = open(dev_assoc_tree_filename, "r")
        dev_assoc_tree_data = dev_assoc_tree_file.readlines()
        allowed_characters = set(string.ascii_lowercase + string.ascii_uppercase + string.digits + '_')
        for line in dev_assoc_tree_data:
            parts = line.partition('"') # remove the tree-related decorations
            for p in parts:
                if len(p) > 1:
                    if set(p) <= allowed_characters:
                        devices.add(p)
                    elif parts[0] != line:
                        dev_assoc_tree_data.append(p)
    except Exception:
        print(f'Failed to open and parse device association tree file')
        sys.exit()

    if len(devices) == 0:
        print(f'No valid entries in device association tree file')
        sys.exit()

    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    try:
        client.connect(hostname=address, port=port, username=username, password=password)
    except Exception:
        print(f'Connection failed')
        sys.exit()

    objects_to_check = set()

    ssh_stdin, ssh_stdout, ssh_stderr = client.exec_command('busctl tree xyz.openbmc_project.GpuMgr')
    tree = ssh_stdout.readlines()
    for line in tree:
        cleaned_line = line.partition('/') # remove the tree-related decorations
        object = cleaned_line[1] + cleaned_line[2]
        if any(dev in object for dev in devices):
            objects_to_check.add(object.replace('\n', ''))

    if len(objects_to_check) == 0:
        print(f'No objects provided by xyz.openbmc_project.GpuMgr')
        print(f'busctl tree command returned:')
        for line in tree:
            print(line.replace('\n', ''))
        sys.exit()

    # if no output filename is specified, use stdout
    output = sys.stdout
    if len(sys.argv) == 7:
        output_filename = sys.argv[6]
        if len(output_filename) != 0:
            try:
                output = open(output_filename, "w")
            except Exception:
                print(f'Failed to open output file')
                sys.exit()

    output.write('{\n')

    # look for xyz.openbmc_project.State.Decorator.Health
    health_interface = 'xyz.openbmc_project.State.Decorator.Health'
    sorted_objects_to_check = sorted(objects_to_check)
    for obj in sorted_objects_to_check:
        ssh_stdin, ssh_stdout, ssh_stderr = client.exec_command('busctl introspect xyz.openbmc_project.GpuMgr ' + obj)
        introspect = ssh_stdout.readlines()
        output.write('\t{ "' + obj + '", ' + str(int(any(health_interface in line for line in introspect))) + ' },\n')

    output.write('}\n')
