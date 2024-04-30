#!/usr/bin/env bash

# Copyright (c) 2024, NVIDIA CORPORATION.  All rights reserved.
#
#  NVIDIA CORPORATION and its licensors retain all intellectual property
#  and proprietary rights in and to this software, related documentation
#  and any modifications thereto.  Any use, reproduction, disclosure or
#  distribution of this software and related documentation without an express
#  license agreement from NVIDIA CORPORATION is strictly prohibited.


SCRIPT=./nvlink-status-wrapper.sh

USAGE="Usage:

  nvlink-status-wrapper-test.sh OPTIONS [script_option]

  OPTIONS:
  --csv
  --mock-nvlinks-count N
  --max-gpus N
  --max-nvswitches N

  OPTIOS are read as long as they are recognized, the next one,
if any, is passed directly to the '${SCRIPT}'.
"

ERR_USAGE="Wrong number of arguments.

${USAGE}"

CSV_SEP="	"

############################# Functions ##############################


print() {
    local message=$1
    if [[ -z ${JSON} ]]; then
        echo ${message}
    fi
}

printParamsStart() {
    if [[ ${JSON} ]]; then
        echo "{"
    fi
}

atLeastOneCommandPrinted=""
printCommand() {
    local command=$1
    if [[ ${FORMAT_CSV} ]]; then
        echo -n "${command}${CSV_SEP}"
    elif [[ ${JSON} ]]; then
        if [[ ${atLeastOneCommandPrinted} ]]; then
            echo ","
        fi
        echo -n "\"${command}\" : "
        atLeastOneCommandPrinted=1
    else
        echo "--------------------------------"
        echo "Running '${command}'"
    fi
}

printParamsEnd() {
    if [[ ${JSON} ]]; then
        echo
        echo "}"
    fi
}

printDebug() {
    local message=$1
    if test "${DEBUG}"; then
        echo "DEBUG: ${message}"
    fi
}

printStatus() {
    local status=$1
    if [[ ${FORMAT_CSV} ]]; then
        echo -n "${status}${CSV_SEP}"
    else
        echo "${status}"
    fi
}

printResult() {
    local res=$1
    if [[ ${FORMAT_CSV} ]]; then
        echo "'${res}'${CSV_SEP}"
    elif [[ ${JSON} ]]; then
        echo "${res}"
    else
        echo "${res}"
    fi
}

runTest() {
    local command=$1
    local device=$2
    local device1=$3
    command="${SCRIPT} ${OPTIONS} ${command} ${device} ${device1}"
    printCommand "${command}"
    res=$( ${command} ); local rc=$?

    if [[ -z ${JSON} ]]; then
        if test ${rc} -eq 0; then
            printStatus "PASS"
        else
            printStatus "FAIL"
        fi
    fi

    printResult "${res}"
}

runTests() {
    local command=$1
    local device=$2
    local N=$3

    for ((i=0; i < N ; i++))
    do
        runTest "${command}" "${device}" "NVLink_${i}"
    done

}

########################### Program start ############################

dispatchOpts="1"
while [[ ${dispatchOpts} ]]; do
    if [[ "$1" == "--help" || "$1" == "-h" ]]; then
        HELP=$1
        shift
    elif [[ "$1" == "--mock-nvlinks-count" ]]; then
        NVLINKS_COUNT=$2
        shift
        shift
    elif [[ "$1" == "--max-gpus" ]]; then
        MAX_GPUS=$2
        shift
        shift
    elif [[ "$1" == "--max-nvswitches" ]]; then
        MAX_NVSWITCHES=$2
        shift
        shift
    elif [[ "$1" == "--csv" ]]; then
        FROMAT_CSV=$1
        shift
    elif [[ "$1" == "--json" || "$1" == "-j" ]]; then
        JSON=$1
        shift
    else
        dispatchOpts=""
    fi
done

if [[ ${HELP} ]]; then
    echo "${USAGE}"
    exit 0
fi

OPTIONS=$1

printParamsStart

print "Getting the number of NVLinks..."
if [[ ${NVLINKS_COUNT} ]]; then
    nvlinksNum=${NVLINKS_COUNT}
else
    nvlinksNum=$( ${SCRIPT} nvlink_number )
fi
print "  nvlinksNum = '${nvlinksNum}'"

if [[ -z ${MAX_GPUS} ]]; then
    MAX_GPUS=8
fi

for ((i=1; i <= ${MAX_GPUS}; i++))
do
    runTest  gpu_nvlink_number                               "GPU_SXM_${i}"
    runTests gpu_nvlink_state_v1                             "GPU_SXM_${i}" ${nvlinksNum}
    runTest  gpu_nvlink_state_v1_agg                         "GPU_SXM_${i}"
    runTests gpu_nvlink_bandwidth                            "GPU_SXM_${i}" ${nvlinksNum}
    runTest  gpu_nvlink_bandwidth_agg                        "GPU_SXM_${i}"
    runTests gpu_nvlink_replay_error_count                   "GPU_SXM_${i}" ${nvlinksNum}
    runTest  gpu_nvlink_replay_error_count_agg               "GPU_SXM_${i}"
    runTests gpu_nvlink_recovery_error_count                 "GPU_SXM_${i}" ${nvlinksNum}
    runTest  gpu_nvlink_recovery_error_count_agg             "GPU_SXM_${i}"
    runTests gpu_nvlink_flit_CRC_error_count                 "GPU_SXM_${i}" ${nvlinksNum}
    runTest  gpu_nvlink_flit_CRC_error_count_agg             "GPU_SXM_${i}"
    runTests gpu_nvlink_data_CRC_error_count                 "GPU_SXM_${i}" ${nvlinksNum}
    runTest  gpu_nvlink_data_CRC_error_count_agg             "GPU_SXM_${i}"
    runTests gpu_nvlink_state_v2                             "GPU_SXM_${i}" ${nvlinksNum}
    runTest  gpu_nvlink_state_v2_agg                         "GPU_SXM_${i}"
    runTests gpu_nvlink_sublink_width_tx                     "GPU_SXM_${i}" ${nvlinksNum}
    runTests gpu_nvlink_sublink_width_rx                     "GPU_SXM_${i}" ${nvlinksNum}
    runTests gpu_nvlink_transmitted_data_throughput          "GPU_SXM_${i}" ${nvlinksNum}
    runTests gpu_nvlink_received_data_throughput             "GPU_SXM_${i}" ${nvlinksNum}
    runTests gpu_nvlink_transmitted_data_throughput_protocol "GPU_SXM_${i}" ${nvlinksNum}
    runTests gpu_nvlink_received_data_throughput_protocol    "GPU_SXM_${i}" ${nvlinksNum}
    runTests gpu_nvlink_availability                         "GPU_SXM_${i}" ${nvlinksNum}
done

runTest  pcieretimer_eeprom_wp_status                         PCIeRetimer
runTest  nvswitch_eeprom_wp_status                            NVSwitch
runTest  gpu_spiflash_wp_status                               GPU
runTest  hmc_spiflash_wp_status                               HMC
runTest  pcieswitch_eeprom_wp_status                          PCIeSwitch
runTest  pcieswitch_spi_mode_status                           PCIeSwitch

if [[ -z ${MAX_NVSWITCHES} ]]; then
    MAX_NVSWITCHES=4
fi

for ((i=0; i < ${MAX_NVSWITCHES}; i++))
do
    runTest  nvswitch_nvlink_number                               "NVSwitch_${i}"
    runTests nvswitch_nvlink_state_v1                             "NVSwitch_${i}" ${nvlinksNum}
    runTest  nvswitch_nvlink_state_v1_agg                         "NVSwitch_${i}"
    runTests nvswitch_nvlink_bandwidth                            "NVSwitch_${i}" ${nvlinksNum}
    runTest  nvswitch_nvlink_bandwidth_agg                        "NVSwitch_${i}"
    runTests nvswitch_nvlink_replay_error_count                   "NVSwitch_${i}" ${nvlinksNum}
    runTest  nvswitch_nvlink_replay_error_count_agg               "NVSwitch_${i}"
    runTests nvswitch_nvlink_recovery_error_count                 "NVSwitch_${i}" ${nvlinksNum}
    runTest  nvswitch_nvlink_recovery_error_count_agg             "NVSwitch_${i}"
    runTests nvswitch_nvlink_flit_CRC_error_count                 "NVSwitch_${i}" ${nvlinksNum}
    runTest  nvswitch_nvlink_flit_CRC_error_count_agg             "NVSwitch_${i}"
    runTests nvswitch_nvlink_data_CRC_error_count                 "NVSwitch_${i}" ${nvlinksNum}
    runTest  nvswitch_nvlink_data_CRC_error_count_agg             "NVSwitch_${i}"
    runTests nvswitch_nvlink_state_v2                             "NVSwitch_${i}" ${nvlinksNum}
    runTest  nvswitch_nvlink_state_v2_agg                         "NVSwitch_${i}"
    runTests nvswitch_nvlink_sublink_width_tx                     "NVSwitch_${i}" ${nvlinksNum}
    runTests nvswitch_nvlink_sublink_width_rx                     "NVSwitch_${i}" ${nvlinksNum}
    runTests nvswitch_nvlink_transmitted_data_throughput          "NVSwitch_${i}" ${nvlinksNum}
    runTests nvswitch_nvlink_received_data_throughput             "NVSwitch_${i}" ${nvlinksNum}
    runTests nvswitch_nvlink_transmitted_data_throughput_protocol "NVSwitch_${i}" ${nvlinksNum}
    runTests nvswitch_nvlink_received_data_throughput_protocol    "NVSwitch_${i}" ${nvlinksNum}
    runTests nvswitch_nvlink_training_error_count_status          "NVSwitch_${i}" ${nvlinksNum}
    runTest  nvswitch_nvlink_training_error_count_status_agg      "NVSwitch_${i}"
    runTests nvswitch_nvlink_runtime_error_count_status           "NVSwitch_${i}" ${nvlinksNum}
    runTest  nvswitch_nvlink_runtime_error_count_status_agg       "NVSwitch_${i}"
done


printParamsEnd
