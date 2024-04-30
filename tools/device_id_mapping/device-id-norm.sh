#!/usr/bin/env bash

# Copyright (c) 2024, NVIDIA CORPORATION.  All rights reserved.
#
#  NVIDIA CORPORATION and its licensors retain all intellectual property
#  and proprietary rights in and to this software, related documentation
#  and any modifications thereto.  Any use, reproduction, disclosure or
#  distribution of this software and related documentation without an express
#  license agreement from NVIDIA CORPORATION is strictly prohibited.


appName="device-id-norm.sh"

PROFILE_FILE_DIR=.
PROFILE_FILE_NAME="device_id_map.csv"

[ -z "$PROFILE_FILE" ] && PROFILE_FILE=${PROFILE_FILE_DIR}/${PROFILE_FILE_NAME}

USAGE="Usage:

  ${appName} DEVICE_ID
  ${appName} --ext DEVICE_ID
  ${appName} (-h|--help)

Details:

  ${appName} [--debug] DEVICE_ID

      Print the normalized form of the DEVICE_ID and return 0 if
      the DEVICE_ID was recognized.

      Print the DEVICE_ID as it was given and return non-zero if
      the DEVICE_ID was NOT recognized.

      With '--debug' option include extensive logging in the
      stdout.

  ${appName} [--debug] --ext DEVICE_ID

      If DEVICE_ID was recognized, print a line of the form:

        NORM_DEVICE_ID STEM INDEXES

      where NORM_DEVICE_ID is the normalized name of DEVICE_ID,
      STEM is NORM_DEVICE_ID with all the indexes removed and
      INDEXES is a space-separated sequence of indexes found in
      NORM_DEVICE_ID. For example

        $ ./${appName} NVSwitch_3
        NVSwitch3 NVSwitch 3

      This can be used to easily obtain the information about
      device identificator in testpoint wrapper scripts, like

        deviceIdNormOut=\$( ${appName} \${deviceId} )

        read deviceFullId deviceStem deviceIndexes <<EOF
        \${deviceIdNormOut}
        EOF

      The details about what constitutes an index and what stem
      are defined in the device map file (see below).

      If the DEVICE_ID was not recognized print the DEVICE_ID as
      it was given and return non-zero.

      With '--debug' option include extensive logging in the
      stdout.

  ${appName} (-h|--help)
"

ERR_USAGE="Wrong number of arguments.

${USAGE}"

dispatchOpts="1"
while [[ ${dispatchOpts} ]]; do
    if [[ "$1" == "--help" || "$1" == "-h" ]]; then
        HELP=$1
        shift
    elif [[ "$1" == "--ext" ]]; then
        EXTENDED=$1
        shift
    else
        dispatchOpts=""
    fi
done

deviceId=$1
sedCmd="sed --quiet"

if [[ ${HELP} ]]; then
    echo "${USAGE}"
    exit 0
fi

deviceId=$1

if [[ -z ${deviceId} ]]; then
    echo "${ERR_USAGE}"
    exit 1
fi

if [[ ${EXTENDED} ]]; then
    sedOutput=$( ${sedCmd} --e "s/^${deviceId};\(.*\)\$/\1/p" ${PROFILE_FILE} )
else
    sedOutput=$( ${sedCmd} --e "s/^${deviceId};\([^ ]\+\).*/\1/p" ${PROFILE_FILE} )
fi

if [[ ${sedOutput} ]]; then
    echo ${sedOutput}
    exit 0
else
    echo ${deviceId}
    exit 1
fi
