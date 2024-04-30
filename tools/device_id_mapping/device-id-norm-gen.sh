#!/usr/bin/env bash

# Copyright (c) 2024, NVIDIA CORPORATION.  All rights reserved.
#
#  NVIDIA CORPORATION and its licensors retain all intellectual property
#  and proprietary rights in and to this software, related documentation
#  and any modifications thereto.  Any use, reproduction, disclosure or
#  distribution of this software and related documentation without an express
#  license agreement from NVIDIA CORPORATION is strictly prohibited.


fullAppName=$( realpath $0 )
dirPath=$( dirname $fullAppName )
appName=${fullAppName#${dirPath}/}

placeholder='#'
indexSep=';'
defaultDeviceMapFile="device_id_map_gen.csv"

# Careful with changing this line, undergoes substitution in openbmc recipe
PROFILE_FILE_DIR=${dirPath}
PROFILE_FILE_NAME=${defaultDeviceMapFile}

[ -z "$PROFILE_FILE" ] && PROFILE_FILE=${PROFILE_FILE_DIR}/${PROFILE_FILE_NAME}
deviceMapPath="${PROFILE_FILE}"

USAGE="Usage:

  ${appName} [--debug] DEVICE_ID
  ${appName} [--debug] --ext DEVICE_ID
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

      Print this help.


Device map config file

The script uses '${defaultDeviceMapFile}' from the directory in
which the script is located as the configuration file, by
default. To change it define the 'PROFILE_FILE' variable
accordingly, e.g

  PROFILE_FILE=some_other.csv ${appName} ...


Each row in the csv configuration file defines a rule to match
and transformation of the DEVICE_ID into a new name. It follows
the format of

  DEVICE_ID_MATCH, CONDITION, TRANSFORMS, NORM_DEVICE_ID_PATTERN

where:

  DEVICE_ID_MATCH

      A regular expression the DEVICE_ID must be matching for the
      rule to be applied. The DEVICE_ID must match in entirety,
      from the beginnig to the end. Groups in parentheses can be
      used, the matched contents of which will go into variables
      'x', 'x1', 'x2', ..., accordingly.

  CONDITION

      An arbitrary bash expression which would be accepted by the
      '(( ... ))' form. It must return non-zero value for the
      rule to be applied. Variables 'x', 'x1', 'x2', ... can be
      used to access the consecutive matching groups in
      DEVICE_ID_MATCH regex. As a special case an empty CONDITION
      is treated as true, for convenience.

  TRANSFORMS

      A '${indexSep}'-separated list of arbitrary bash
      expressions which would be accepted by the '(( ... ))'
      form. Variables 'x', 'x1', 'x2', ... can be used to access
      the consecutive matching groups in DEVICE_ID_MATCH regex.

  DEVICE_ID_NORM

      A string defining the normalized form of the device id,
      with each '${placeholder}' character being substituted with
      a corresponding value from TRANSFORMS results.

For example, the row

  GPU_SXM_([0-9]+),1 <= x && x <= 4,x+3,GPU#

defines the following mapping:

  GPU_SXM_1 -> GPU4
  GPU_SXM_2 -> GPU5
  GPU_SXM_3 -> GPU6
  GPU_SXM_4 -> GPU7

The rules are applied in succession until the first match, then
the program exits. If no rule matched the DEVICE_ID is considered
unrecognized.
"

ERR_USAGE="Wrong number of arguments.

${USAGE}"

printDebug() {
    local message=$1
    if test "${DEBUG}"; then
        echo "DEBUG: ${message}"
    fi
}

evalCondition() {
    local environment=$1
    local condition=$2

    if [[ ${condition} ]]; then
        printDebug "Checking condition '${condition}' for environment '${environment}'..."
        if ! bash -c "${environment} bash -c 'exit \$(( \"${condition}\" ))'"; then
            printDebug "  ...true"
            return 0
        else
            printDebug "  ...false"
            return 1
        fi
    else
        printDebug "Condition not specified. Assuming it's true"
        return 0
    fi
}

dispatchOpts="1"
while [[ ${dispatchOpts} ]]; do
    if [[ "$1" == "--help" || "$1" == "-h" ]]; then
        HELP=$1
        shift
    elif [[ "$1" == "--debug" || "$1" == "-d" ]]; then
        DEBUG=$1
        shift
    elif [[ "$1" == "--ext" ]]; then
        EXTENDED=$1
        shift
    else
        dispatchOpts=""
    fi
done

if [[ ${HELP} ]]; then
    echo "${USAGE}"
    exit 0
fi

DEVICE_ID=$1

if [[ -z ${DEVICE_ID} ]]; then
    echo "${ERR_USAGE}"
    exit 1
fi

mappedDevice=
printDebug "Opening device id mapping file '${deviceMapPath}'"
while IFS=',' read -r regexPattern condition joinedIndexTransforms targetDeviceIdPattern
do

    printDebug "Row elements:"
    printDebug "  regexPattern          = '${regexPattern}'"
    printDebug "  condition             = '${condition}'"
    printDebug "  joinedIndexTransforms = '${joinedIndexTransforms}'"
    printDebug "  targetDeviceIdPattern = '${targetDeviceIdPattern}'"

    actualRegex="^${regexPattern}\$"
    printDebug "Matching '${DEVICE_ID}' against regex '${actualRegex}'..."
    [[ ${DEVICE_ID} =~ ${actualRegex} ]]

    if [[ ${BASH_REMATCH[0]} ]]; then

        printDebug "  ...match"
        N=${#BASH_REMATCH[@]}

        # 'environment' defines variables
        # x  : first regex match group
        # x1 : second regex match group
        # x2 : third regex match group
        # ...
        # up to Nth group.
        environment=
        for ((i=0; i < N-1; i++))
        do
            val=${BASH_REMATCH[i+1]}
            printDebug "Match group $(( i+1 )): '${val}'"
            if ((i == 0)); then
                var="x"
            else
                var="x${i}"
            fi
            environment="${environment} ${var}=${val}"
        done

        if evalCondition "${environment}" "${condition}"; then

            IFS="${indexSep}" read -a indexTransforms <<EOF
${joinedIndexTransforms}
EOF

            targetDeviceId=${targetDeviceIdPattern}
            joinedIndexes=
            for elem in ${indexTransforms[@]}
            do
                printDebug "Evaluating '${elem}' in environment '${environment}'..."
                resIndex=$( bash -c "echo \$( ${environment} bash -c 'echo \$(( ${elem} ))' )" )
                printDebug "  ...result: '${resIndex}'"
                printDebug "Substituting for the next '${placeholder}' in pattern '${targetDeviceIdPattern}'..."
                targetDeviceId=${targetDeviceId/"${placeholder}"/${resIndex}}
                printDebug "  ...result: '${targetDeviceId}'"
                joinedIndexes="${joinedIndexes} ${resIndex}"
            done

            targetDeviceStem=${targetDeviceIdPattern/"${placeholder}"/}

            printDebug "Normalized device id: '${targetDeviceId}'"
            printDebug "Device id stem:       '${targetDeviceStem}'"
            printDebug "Device id indexes:    '${joinedIndexes}'"

            break
        fi

    else
        printDebug "  ...no match"
    fi

done < "${deviceMapPath}"

if [[ ${targetDeviceId} ]]; then
    if [[ ${EXTENDED} ]]; then
        echo ${targetDeviceId} ${targetDeviceStem}${joinedIndexes}
    else
        echo ${targetDeviceId}
    fi
    exit 0
else
    printDebug "No mapping found in '${deviceMapPath}' for device id '${DEVICE_ID}'"
    echo ${DEVICE_ID}
    exit 1
fi
