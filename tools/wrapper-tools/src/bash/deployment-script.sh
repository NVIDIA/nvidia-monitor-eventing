#!/usr/bin/env bash

# Copyright (c) 2024, NVIDIA CORPORATION.  All rights reserved.
#
#  NVIDIA CORPORATION and its licensors retain all intellectual property
#  and proprietary rights in and to this software, related documentation
#  and any modifications thereto.  Any use, reproduction, disclosure or
#  distribution of this software and related documentation without an express
#  license agreement from NVIDIA CORPORATION is strictly prohibited.


scrFullpath=$(realpath $0)
scrDir=$(dirname ${scrFullpath})

mockupsDir="${scrDir}/wrapper-mockups"
wrappersDir=/usr/bin

mockupWrapperPath() {
    local wrapperName=$1
    echo "${mockupsDir}/${wrapperName}"
}

originalWrapperPath() {
    local wrapperName=$1
    echo "${wrappersDir}/${wrapperName}.original"
}

executionWrapperPath() {
    local wrapperName=$1
    echo "${wrappersDir}/${wrapperName}"
}

setWrapperOriginal() {
    local wrapperName=$1
    local original=$( originalWrapperPath ${wrapperName} )
    local execution=$( executionWrapperPath ${wrapperName} )
    if [[ -f "${original}" ]]; then
        cp -f ${original} ${execution}
        rm -f ${original}
        return 0
    else
        echo "No '${originl}' found. \
Either '${execution}' is already in its original form or \
the original state of '${execution}' cannot be recovered."
        return 1
    fi
}

setWrapperMockup() {
    local wrapperName=$1
    local original=$( originalWrapperPath ${wrapperName} )
    local mockup=$( mockupWrapperPath ${wrapperName} )
    local execution=$( executionWrapperPath ${wrapperName} )
    if [[ -f "${mockup}" ]]; then
        if [[ ! -f "${original}" ]]; then
            cp -f ${execution} ${original}
            echo "Original wrapper '${execution}' saved in '${original}'"
        fi
        cp -f ${mockup} ${execution}
        return 0
    else
        echo "No '${mockup}' found. \
Cannot set the '${execution}' wrapper to its mocked state."
        return 1
    fi
}

USAGE="Usage:

  deployment-script.sh [action]
  deployment-script.sh (-h | --help)

action:
    mockup    Install the mockup wrappers
    original  Revert wrappers back to original
"

ERR_USAGE="Wrong number of arguments.

${USAGE}"

if [[ "$1" = "--help" || "$1" = "-h" ]]; then
    echo "${USAGE}"
    exit 0
fi

actionDefault="mockup"
if [[ $# -lt 1 ]]; then
    echo "No argument #1 provided. Using default value '$actionDefault'"
    action=$actionDefault
else
    action=$1
fi

(
    cd ${mockupsDir}
    for wrapper in *; do
        echo "Installing ${action} wrapper '${wrapper}'"
        if [[ "${action}" == "mockup" ]]; then
            setWrapperMockup ${wrapper}
        elif [[ "${action}" == "original" ]]; then
            setWrapperOriginal ${wrapper}
        else
            >&2 echo "Unrecognized action: '${action}'"
            exit 1
        fi
    done
)
