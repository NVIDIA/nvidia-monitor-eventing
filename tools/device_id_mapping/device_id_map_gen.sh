#!/usr/bin/bash

# Copyright (c) 2024, NVIDIA CORPORATION.  All rights reserved.
#
#  NVIDIA CORPORATION and its licensors retain all intellectual property
#  and proprietary rights in and to this software, related documentation
#  and any modifications thereto.  Any use, reproduction, disclosure or
#  distribution of this software and related documentation without an express
#  license agreement from NVIDIA CORPORATION is strictly prohibited.


# ./generate-lookup.sh './device-id-norm-gen.sh --ext' devices.list device_id_map_gen.csv

scrFullpath=$(realpath $0)
scrDir=$(dirname ${scrFullpath})
scrName=${scrFullpath#${scrDir}/}

USAGE="Usage:

  ${scrName} conversion_script devices_list lookup_table_output

Generate device id lookup table"

ERR_USAGE="Wrong number of arguments.

${USAGE}"

dispatchOpts="1"
while [[ ${dispatchOpts} ]]; do
    if [[ "$1" == "--help" || "$1" == "-h" ]]; then
        HELP=$1
        shift
    else
        dispatchOpts=""
    fi
done

if [[ ${HELP} ]]; then
    echo "${USAGE}"
    exit 0
fi

if [[ $# -lt 1 ]]; then
    echo "No mandatory argument #1 provided. Exiting"
    echo "${USAGE}"
    exit 1
else
    conversionScript=$1
fi

if [[ $# -lt 2 ]]; then
    echo "No mandatory argument #2 provided. Exiting"
    echo "${USAGE}"
    exit 1
else
    devicesListFile=$2
fi

# if [[ $# -lt 3 ]]; then
#     echo "No mandatory argument #3 provided. Exiting"
#     echo "${USAGE}"
#     exit 1
# else
#     lookupTable=$3
# fi

# echo -n > ${lookupTable}
# while read -r deviceId;
# do
#     echo -n ${deviceId} >> ${lookupTable}
#     echo -n ";" >> ${lookupTable}
#     ${conversionScript} ${deviceId} >> ${lookupTable}
# done < "${devicesListFile}"

while read -r deviceId;
do
    echo -n ${deviceId}
    echo -n ";"
    ${conversionScript} ${deviceId}
done < "${devicesListFile}"
