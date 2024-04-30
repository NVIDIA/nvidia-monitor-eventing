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
scrName=${scrFullpath#${scrDir}/}

USAGE="Usage:

  ${scrName} commands-list time-output [repeat-count]


Run SMBPBI wrappers and measure their execution time.

'commands-list'
      A CSV file containing commands to be run. Columns:
      1. Executable
      2. Arguments
'time-output'
      A CSV file to write the performance test results to. Columns:
      1. .accessor.executable
      2. .accessor.arguments
      3. cpu_percent
      4. elapsed_real
      5. total_system_s
      6. total_user_s
      7. context_switches_invol
      8. context_switches_vol
      9. exit_code
      10. output
'repeat-count'
      How many times to repeat the commands-list. Default: 1
"

ERR_USAGE="Wrong number of arguments.

${USAGE}"

csv_sanitize() {
    local value=$1
    escapedValue=${value//${quoteChar}/${quoteChar}${quoteChar}}
    echo "${quoteChar}${escapedValue}${quoteChar}"
    return 0
}

tmpFile=tmp.txt
defaultSep=';'
quoteChar="'"

dispatchOpts="1"
while [[ ${dispatchOpts} ]]; do
    if [[ "$1" == "--help" || "$1" == "-h" ]]; then
        HELP=$1
        shift
    elif [[ "$1" == "--sep" ]]; then
        sep=$2
        shift
        shift
    else
        dispatchOpts=""
    fi
done

if [[ -z ${sep} ]]; then
    sep=${defaultSep}
fi

if [[ ${HELP} ]]; then
    echo "${USAGE}"
    exit 0
fi

if [[ $# -lt 1 ]]; then
    echo "No mandatory argument #1 provided. Exiting"
    echo "${USAGE}"
    exit 1
else
    commandLines=$1
fi

if [[ $# -lt 2 ]]; then
    echo "No mandatory argument #2 provided. Exiting"
    echo "${USAGE}"
    exit 1
else
    outputFile=$2
fi

repeatCountDefault="1"
if [[ $# -lt 3 ]]; then
    echo "No argument #3 provided. Using default value '$repeatCountDefault'"
    repeatCount=$repeatCountDefault
else
    repeatCount=$3
fi


header=".accessor.executable\
${sep}.accessor.arguments\
${sep}cpu_percent\
${sep}elapsed_real\
${sep}total_system_s\
${sep}total_user_s\
${sep}context_switches_invol\
${sep}context_switches_vol\
${sep}exit_code\
${sep}output"

# from `man time':
# `C'
#       Name and command line arguments of the command being timed.
# `P'
#       Percentage of the CPU that this job got.  This is just user +
#       system times divided by the total running time. It also prints a
#       percentage sign.
# `E'
#       Elapsed real (wall clock) time used by the process, in
#       [hours:]minutes:seconds.
# `S'
#       Total number of CPU-seconds used by the system on behalf of the
#       process (in kernel mode), in seconds.
# `U'
#       Total number of CPU-seconds that the process used directly (in
#       user mode), in seconds.
# `c'
#       Number of times the process was context-switched involuntarily
#       (because the time slice expired).
# `w'
#       Number of times that the program was context-switched voluntarily,
#       for instance while waiting for an I/O operation to complete.
# `x'
#       Exit status of the command.

timeColumns="%P${sep}%E${sep}%S${sep}%U${sep}%c${sep}%w${sep}%x"

echo "${header}" > ${outputFile}

for ((i=1; i <= repeatCount; i++))
do

    while IFS="${sep}" read -r wrapper arguments;
    do

        cmd="${wrapper} ${arguments}"
        echo -n "Running (${i}/${repeatCount}) '${cmd}'... "

        output=$( TIME=${timeColumns} time -o ${tmpFile} ${cmd} 2>&1 )
        # 'tail' removes the "Command exited with non-zero status X"
        # crap included in the first line by 'time' in case
        # ${command} exited with non-zero which cannot be silenced
        # with '--quiet' in BusyBox.
        timeInfo=$( tail -n 1 ${tmpFile} )
        exitCode=$( echo "${timeInfo//${sep}/ }" | cut -d " " -f 8 )
        echo "exit code: ${exitCode}"

        wrapperS=$( csv_sanitize "${wrapper}")
        argumentsS=$( csv_sanitize "${arguments}")
        outputS=$( csv_sanitize "${output}")
        echo "${wrapperS}${sep}${argumentsS}${sep}${timeInfo}${sep}${outputS}" >> ${outputFile}

        # echo "${wrapper}${sep}${arguments}${sep}${timeInfo}${sep}'${output}'" >> ${outputFile}

    done < "${commandLines}"

done

rm -f ${tmpFile}
