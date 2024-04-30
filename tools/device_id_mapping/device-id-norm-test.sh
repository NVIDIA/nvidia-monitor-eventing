#!/usr/bin/env bash

# Copyright (c) 2024, NVIDIA CORPORATION.  All rights reserved.
#
#  NVIDIA CORPORATION and its licensors retain all intellectual property
#  and proprietary rights in and to this software, related documentation
#  and any modifications thereto.  Any use, reproduction, disclosure or
#  distribution of this software and related documentation without an express
#  license agreement from NVIDIA CORPORATION is strictly prohibited.


[ -z "$SCRIPT" ] && SCRIPT='device-id-norm.sh'

runTest() {
    local deviceId=$1
    local expectedOutput=$2  # ignore for now
    # command=${scr} ${OPTS} ${deviceId}
    result=$( ${SCRIPT} ${OPTS} ${deviceId} ); rc=$?
    echo -n "${deviceId} -> ${result}"
    if (( rc == 0 )); then
        echo "  PASS"
    else
        echo "  FAIL"
    fi
}

dispatchOpts="1"
while [[ ${dispatchOpts} ]]; do
    if [[ "$1" == "--help" || "$1" == "-h" ]]; then
        HELP=$1
        shift
    elif [[ "$1" == "--opts" ]]; then
        OPTS=$2
        shift
        shift
    else
        dispatchOpts=""
    fi
done

############################# Old names ##############################

echo
echo "Old names"
echo

runTest Baseboard
runTest GPU0
runTest GPU1
runTest GPU2
runTest GPU3
runTest GPU4
runTest GPU5
runTest GPU6
runTest GPU7
runTest NVSwitch0
runTest NVSwitch1
runTest NVSwitch2
runTest NVSwitch3
runTest eth0
runTest usb0
runTest NVLinkFabric
runTest PCIeRetimerFabric
runTest PCIeSwitchFabric
runTest GPUDRAM0
runTest GPUDRAM1
runTest GPUDRAM2
runTest GPUDRAM3
runTest GPUDRAM4
runTest GPUDRAM5
runTest GPUDRAM6
runTest GPUDRAM7
runTest GPU0
runTest GPU1
runTest GPU2
runTest GPU3
runTest GPU4
runTest GPU5
runTest GPU6
runTest GPU7
runTest NVSwitch0
runTest NVSwitch1
runTest NVSwitch2
runTest NVSwitch3
runTest PCIeRetimer0
runTest PCIeRetimer1
runTest PCIeRetimer2
runTest PCIeRetimer3
runTest PCIeRetimer4
runTest PCIeRetimer5
runTest PCIeRetimer6
runTest PCIeRetimer7
runTest PCIeSwitch0
runTest DOWN0
runTest DOWN1
runTest DOWN2
runTest DOWN3
runTest NVLink0
runTest NVLink1
runTest NVLink10
runTest NVLink11
runTest NVLink12
runTest NVLink13
runTest NVLink14
runTest NVLink15
runTest NVLink16
runTest NVLink17
runTest NVLink18
runTest NVLink19
runTest NVLink2
runTest NVLink20
runTest NVLink21
runTest NVLink22
runTest NVLink23
runTest NVLink24
runTest NVLink25
runTest NVLink26
runTest NVLink27
runTest NVLink28
runTest NVLink29
runTest NVLink3
runTest NVLink30
runTest NVLink31
runTest NVLink32
runTest NVLink33
runTest NVLink34
runTest NVLink35
runTest NVLink36
runTest NVLink37
runTest NVLink38
runTest NVLink39
runTest NVLink4
runTest NVLink5
runTest NVLink6
runTest NVLink7
runTest NVLink8
runTest NVLink9
runTest UP0
runTest FPGA
runTest GPU0
runTest GPU1
runTest GPU2
runTest GPU3
runTest GPU4
runTest GPU5
runTest GPU6
runTest GPU7
runTest NVSwitch0
runTest NVSwitch1
runTest NVSwitch2
runTest NVSwitch3
runTest PCIeRetimer0
runTest PCIeRetimer1
runTest PCIeRetimer2
runTest PCIeRetimer3
runTest PCIeRetimer4
runTest PCIeRetimer5
runTest PCIeRetimer6
runTest PCIeRetimer7
runTest PCIeSwitch0

############################# New names ##############################

echo
echo "New names"
echo

runTest HGX_Baseboard_0
runTest HGX_FPGA_0
runTest HGX_GPU_SXM_1
runTest HGX_GPU_SXM_2
runTest HGX_GPU_SXM_3
runTest HGX_GPU_SXM_4
runTest HGX_GPU_SXM_5
runTest HGX_GPU_SXM_6
runTest HGX_GPU_SXM_7
runTest HGX_GPU_SXM_8
runTest HGX_HMC_0
runTest HGX_NVSwitch_0
runTest HGX_NVSwitch_1
runTest HGX_NVSwitch_2
runTest HGX_NVSwitch_3
runTest HGX_PCIeRetimer_0
runTest HGX_PCIeRetimer_1
runTest HGX_PCIeRetimer_2
runTest HGX_PCIeRetimer_3
runTest HGX_PCIeRetimer_4
runTest HGX_PCIeRetimer_5
runTest HGX_PCIeRetimer_6
runTest HGX_PCIeRetimer_7
runTest HGX_PCIeSwitch_0
runTest eth0
runTest usb0
runTest HGX_NVLinkFabric_0
runTest HGX_PCIeRetimerTopology_0
runTest HGX_PCIeSwitchTopology_0
runTest HGX_BMC_0
runTest HGX_FabricManager_0
runTest GPU_SXM_1_DRAM_0
runTest GPU_SXM_2_DRAM_0
runTest GPU_SXM_3_DRAM_0
runTest GPU_SXM_4_DRAM_0
runTest GPU_SXM_5_DRAM_0
runTest GPU_SXM_6_DRAM_0
runTest GPU_SXM_7_DRAM_0
runTest GPU_SXM_8_DRAM_0
runTest GPU_SXM_1
runTest GPU_SXM_2
runTest GPU_SXM_3
runTest GPU_SXM_4
runTest GPU_SXM_5
runTest GPU_SXM_6
runTest GPU_SXM_7
runTest GPU_SXM_8
runTest NVSwitch_0
runTest NVSwitch_1
runTest NVSwitch_2
runTest NVSwitch_3
runTest PCIeSwitch_0
runTest DOWN_0
runTest DOWN_1
runTest DOWN_2
runTest DOWN_3
runTest NVLink_0
runTest NVLink_1
runTest NVLink_10
runTest NVLink_11
runTest NVLink_12
runTest NVLink_13
runTest NVLink_14
runTest NVLink_15
runTest NVLink_16
runTest NVLink_17
runTest NVLink_18
runTest NVLink_19
runTest NVLink_2
runTest NVLink_20
runTest NVLink_21
runTest NVLink_22
runTest NVLink_23
runTest NVLink_24
runTest NVLink_25
runTest NVLink_26
runTest NVLink_27
runTest NVLink_28
runTest NVLink_29
runTest NVLink_3
runTest NVLink_30
runTest NVLink_31
runTest NVLink_32
runTest NVLink_33
runTest NVLink_34
runTest NVLink_35
runTest NVLink_36
runTest NVLink_37
runTest NVLink_38
runTest NVLink_39
runTest NVLink_4
runTest NVLink_5
runTest NVLink_6
runTest NVLink_7
runTest NVLink_8
runTest NVLink_9
runTest UP_0
runTest FPGA_0
runTest GPU_SXM_1
runTest GPU_SXM_2
runTest GPU_SXM_3
runTest GPU_SXM_4
runTest GPU_SXM_5
runTest GPU_SXM_6
runTest GPU_SXM_7
runTest GPU_SXM_8
runTest NVSwitch_0
runTest NVSwitch_1
runTest NVSwitch_2
runTest NVSwitch_3
runTest PCIeRetimer_0
runTest PCIeRetimer_1
runTest PCIeRetimer_2
runTest PCIeRetimer_3
runTest PCIeRetimer_4
runTest PCIeRetimer_5
runTest PCIeRetimer_6
runTest PCIeRetimer_7
runTest PCIeSwitch_0
runTest HGX_Baseboard_0
