#!/bin/bash

# Copyright (c) 2024, NVIDIA CORPORATION.  All rights reserved.
#
#  NVIDIA CORPORATION and its licensors retain all intellectual property
#  and proprietary rights in and to this software, related documentation
#  and any modifications thereto.  Any use, reproduction, disclosure or
#  distribution of this software and related documentation without an express
#  license agreement from NVIDIA CORPORATION is strictly prohibited.

SPREADSHEETS=spreadsheets.txt

USAGE="Usage:

    ./spreadsheets-install.sh

Pushes the Google Apps script in the current folder to all the
projects listed in the '${SPREADSHEETS}' file (one per line)
using 'clasp' tool. Make sure you are logged in with

    clasp login

before running this script.

Options:
    -h, --help
        Print this message and exit.
"

INFO_USAGE="Install script in Google Apps Script projects.

${USAGE}"

ERR_USAGE="Incorrect call.

${USAGE}"

if [[ $# -gt 0 ]]; then
    if [[ $# -eq 1 && ( "$1" = "--help" || "$1" = "-h" ) ]]; then
        echo "${INFO_USAGE}"
        exit 0
    else
        echo "${ERR_USAGE}"
        exit 1
    fi
fi


SED_SUBST='s|^https\?://script\.google\.com.*\?/projects/\([^/]\+\).*$|\1|p'

while read -r LINE;
do
    echo "Processing project: '${LINE}'"
    ID=$( echo "${LINE}" | sed -n -e "${SED_SUBST}" )
    if [[ "${ID}" = "" ]]; then
        echo "WARNING: Link not recognised."
        UPDATED=""
    else
        echo "Creating temporary '.clasp.json' file..."
        echo "{\"scriptId\":\"${ID}\",\"rootDir\":\"$(pwd)\"}" > .clasp.json
        echo "Pushing script to the Apps Script project..."

        if ! clasp push;
        then
            echo "WARNING: 'clasp push' returned $? code"
            UPDATED=""
        else
            UPDATED="yes"
        fi
    fi
    if [[ "${UPDATED}" ]]; then
        echo "Project successfully updated."
    else
        echo "Project NOT updated."
    fi
    echo
done < "${SPREADSHEETS}"
