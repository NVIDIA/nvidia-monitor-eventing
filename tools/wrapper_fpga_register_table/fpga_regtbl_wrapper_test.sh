#!/usr/bin/env bash

# Copyright (c) 2024, NVIDIA CORPORATION.  All rights reserved.
#
#  NVIDIA CORPORATION and its licensors retain all intellectual property
#  and proprietary rights in and to this software, related documentation
#  and any modifications thereto.  Any use, reproduction, disclosure or
#  distribution of this software and related documentation without an express
#  license agreement from NVIDIA CORPORATION is strictly prohibited.


testsTotal=0
testsPassed=0
testsFailed=0

# export for dependant map tool, you might need to set it yourself if doesn't work
export DEVICE_MAP_FILE="../device_id_mapping/device_id_map.csv"
export DEVICE_NAME_MAPPER="../device_id_mapping/device-id-norm.sh"

perfTest()
{
    testsTotal=$(($testsTotal + 1))
    local cmd="$1"
    local dev="$2"
    local exp="$3"
    local resp=`./fpga_regtbl_wrapper -test-run $cmd $dev`

    if [ "$resp" != "$exp" ]; then
        echo "ERROR: test failed $cmd $dev resp $resp != expected $exp"
        testsFailed=$(($testsFailed + 1))
    else
        testsPassed=$(($testsPassed + 1))
    fi

    echo -n -e "test $testsTotal\r"
}

summary()
{
    echo "Tests total $testsTotal passed $testsPassed failed $testsFailed"
    if [ $testsFailed -ne 0 ]; then
        exit 1
    fi
    exit 0
}

# quick qemu test
# ./fpga_regtbl_wrapper -verbose -dry-run "FPGA_SXM1_PWR_EN" "GPU_SXM_1"
# ./fpga_regtbl_wrapper -verbose -dry-run "FPGA_SXM2_PWR_EN" "GPU_SXM_2"
# ./fpga_regtbl_wrapper -verbose -dry-run "FPGA_SXM3_PWR_EN" "GPU_SXM_3"
# ./fpga_regtbl_wrapper -verbose -dry-run "FPGA_SXM4_PWR_EN" "GPU_SXM_4"
# ./fpga_regtbl_wrapper -verbose -dry-run "FPGA_SXM5_PWR_EN" "GPU_SXM_5"
# ./fpga_regtbl_wrapper -verbose -dry-run "FPGA_SXM6_PWR_EN" "GPU_SXM_6"
# ./fpga_regtbl_wrapper -verbose -dry-run "FPGA_SXM7_PWR_EN" "GPU_SXM_7"
# ./fpga_regtbl_wrapper -verbose -dry-run "FPGA_SXM8_PWR_EN" "GPU_SXM_8"

# ./fpga_regtbl_wrapper -verbose -dry-run "FPGA_NVSW_01_VDD1V8_PWR_EN" "NVSwitch_0"
# ./fpga_regtbl_wrapper -verbose -dry-run "FPGA_NVSW_01_VDD1V8_PWR_EN" "NVSwitch_1"
# ./fpga_regtbl_wrapper -verbose -dry-run "FPGA_NVSW_23_VDD1V8_PWR_EN" "NVSwitch_2"
# ./fpga_regtbl_wrapper -verbose -dry-run "FPGA_NVSW_23_VDD1V8_PWR_EN" "NVSwitch_3"

# ./fpga_regtbl_wrapper -verbose -dry-run "FPGA_RET_0123_0V9_PWR_EN" "PCIeRetimer_0"
# ./fpga_regtbl_wrapper -verbose -dry-run "FPGA_RET_0123_0V9_PWR_EN" "PCIeRetimer_1"
# ./fpga_regtbl_wrapper -verbose -dry-run "FPGA_RET_0123_0V9_PWR_EN" "PCIeRetimer_2"
# ./fpga_regtbl_wrapper -verbose -dry-run "FPGA_RET_0123_0V9_PWR_EN" "PCIeRetimer_3"
# ./fpga_regtbl_wrapper -verbose -dry-run "FPGA_RET_4567_0V9_PWR_EN" "PCIeRetimer_4"
# ./fpga_regtbl_wrapper -verbose -dry-run "FPGA_RET_4567_0V9_PWR_EN" "PCIeRetimer_5"
# ./fpga_regtbl_wrapper -verbose -dry-run "FPGA_RET_4567_0V9_PWR_EN" "PCIeRetimer_6"
# ./fpga_regtbl_wrapper -verbose -dry-run "FPGA_RET_4567_0V9_PWR_EN" "PCIeRetimer_7"

#FPGA_SXM0_PWR_EN indexed from 0
perfTest "FPGA_SXM0_PWR_EN" "GPU_SXM_1" "0x0b 1 0x3C 0" #expect particular slave addr, register, bit
perfTest "FPGA_SXM1_PWR_EN" "GPU_SXM_2" "0x0b 1 0x3C 1"
perfTest "FPGA_SXM2_PWR_EN" "GPU_SXM_3" "0x0b 1 0x3C 2"
perfTest "FPGA_SXM3_PWR_EN" "GPU_SXM_4" "0x0b 1 0x3C 3"
perfTest "FPGA_SXM4_PWR_EN" "GPU_SXM_5" "0x0b 1 0x3C 4"
perfTest "FPGA_SXM5_PWR_EN" "GPU_SXM_6" "0x0b 1 0x3C 5"
perfTest "FPGA_SXM6_PWR_EN" "GPU_SXM_7" "0x0b 1 0x3C 6"
perfTest "FPGA_SXM7_PWR_EN" "GPU_SXM_8" "0x0b 1 0x3C 7"

perfTest "GPU1_PWRGD" "GPU_SXM_1" "0x0b 1 0x14 0"
perfTest "GPU2_PWRGD" "GPU_SXM_2" "0x0b 1 0x14 1"
perfTest "GPU3_PWRGD" "GPU_SXM_3" "0x0b 1 0x14 2"
perfTest "GPU4_PWRGD" "GPU_SXM_4" "0x0b 1 0x14 3"
perfTest "GPU5_PWRGD" "GPU_SXM_5" "0x0b 1 0x14 4"
perfTest "GPU6_PWRGD" "GPU_SXM_6" "0x0b 1 0x14 5"
perfTest "GPU7_PWRGD" "GPU_SXM_7" "0x0b 1 0x14 6"
perfTest "GPU8_PWRGD" "GPU_SXM_8" "0x0b 1 0x14 7"

perfTest "FPGA_NVSW_01_VDD1V8_PWR_EN" "NVSwitch_0" "0x0b 1 0x3A 6"
perfTest "FPGA_NVSW_01_VDD1V8_PWR_EN" "NVSwitch_1" "0x0b 1 0x3A 6"
perfTest "FPGA_NVSW_23_VDD1V8_PWR_EN" "NVSwitch_2" "0x0b 1 0x3A 7"
perfTest "FPGA_NVSW_23_VDD1V8_PWR_EN" "NVSwitch_3" "0x0b 1 0x3A 7"

perfTest "NVSW_12_VDD1V8_PG" "NVSwitch_0" "0x0b 1 0x1C 6"
perfTest "NVSW_12_VDD1V8_PG" "NVSwitch_1" "0x0b 1 0x1C 6"
perfTest "NVSW_34_VDD1V8_PG" "NVSwitch_2" "0x0b 1 0x1C 7"
perfTest "NVSW_34_VDD1V8_PG" "NVSwitch_3" "0x0b 1 0x1C 7"

perfTest "NVSW_1_HVDD_PG" "NVSwitch_0" "0x0b 1 0x1C 2"
perfTest "NVSW_2_HVDD_PG" "NVSwitch_1" "0x0b 1 0x1C 5"
perfTest "NVSW_3_HVDD_PG" "NVSwitch_2" "0x0b 1 0x1E 2"
perfTest "NVSW_4_HVDD_PG" "NVSwitch_3" "0x0b 1 0x1E 5"

perfTest "FPGA_RET_0123_0V9_PWR_EN" "PCIeRetimer_0" "0x0b 1 0x87 0"
perfTest "FPGA_RET_0123_0V9_PWR_EN" "PCIeRetimer_1" "0x0b 1 0x87 0"
perfTest "FPGA_RET_0123_0V9_PWR_EN" "PCIeRetimer_2" "0x0b 1 0x87 0"
perfTest "FPGA_RET_0123_0V9_PWR_EN" "PCIeRetimer_3" "0x0b 1 0x87 0"
perfTest "FPGA_RET_4567_0V9_PWR_EN" "PCIeRetimer_4" "0x0b 1 0x87 3"
perfTest "FPGA_RET_4567_0V9_PWR_EN" "PCIeRetimer_5" "0x0b 1 0x87 3"
perfTest "FPGA_RET_4567_0V9_PWR_EN" "PCIeRetimer_6" "0x0b 1 0x87 3"
perfTest "FPGA_RET_4567_0V9_PWR_EN" "PCIeRetimer_7" "0x0b 1 0x87 3"

perfTest "RET0_0123_0V9_PG" "PCIeRetimer_0" "0x0b 1 0x28 0"
perfTest "RET0_4567_0V9_PG" "PCIeRetimer_4" "0x0b 1 0x28 4"
perfTest "RET1_0123_0V9_PG" "PCIeRetimer_1" "0x0b 1 0x28 1"
perfTest "RET1_4567_0V9_PG" "PCIeRetimer_5" "0x0b 1 0x28 5"
perfTest "RET2_0123_0V9_PG" "PCIeRetimer_2" "0x0b 1 0x28 2"
perfTest "RET2_4567_0V9_PG" "PCIeRetimer_6" "0x0b 1 0x28 6"
perfTest "RET3_0123_0V9_PG" "PCIeRetimer_3" "0x0b 1 0x28 3"
perfTest "RET3_4567_0V9_PG" "PCIeRetimer_7" "0x0b 1 0x28 7"

perfTest "PEX_SWITCH_PWR_EN" "PCIeSwitch_0" "0x0b 1 0x12 4"

perfTest "PEXSW_PEX0V8_PG" "PCIeSwitch_0" "0x0b 1 0x2A 7"

perfTest "FPGA_HSC1_LEFT_EN"   "BMC_0"     "0x0b 1 0x39 0"
perfTest "FPGA_HSC2_LEFT_EN"   "BMC_0"     "0x0b 1 0x39 1"
perfTest "FPGA_HSC3_LEFT_EN"   "BMC_0"     "0x0b 1 0x39 2"
perfTest "FPGA_HSC4_LEFT_EN"   "BMC_0"     "0x0b 1 0x39 3"
perfTest "FPGA_HSC5_RIGHT_EN"  "BMC_0"     "0x0b 1 0x39 4"
perfTest "FPGA_HSC6_RIGHT_EN"  "BMC_0"     "0x0b 1 0x39 5"
perfTest "FPGA_HSC7_RIGHT_EN"  "BMC_0"     "0x0b 1 0x39 6"
perfTest "FPGA_HSC8_RIGHT_EN"  "BMC_0"     "0x0b 1 0x39 7"
perfTest "FPGA_HSC9_LEFT_EN"   "BMC_0"     "0x0b 1 0x38 4"
perfTest "FPGA_HSC10_RIGHT_EN" "BMC_0"     "0x0b 1 0x38 5"
perfTest "FPGA_HSC11_RIGHT_EN" "BMC_0"     "0x0b 1 0x38 6"

perfTest "GPU1_EROT_FATAL_ERROR_N"  "erot"     "0x0b 1 0x7A 0"
perfTest "GPU8_EROT_FATAL_ERROR_N"  "erot"     "0x0b 1 0x7A 7"
perfTest "NVSW1_EROT_FATAL_ERROR_N" "erot"     "0x0b 1 0x7C 0"
perfTest "NVSW4_EROT_FATAL_ERROR_N" "erot"     "0x0b 1 0x7C 3"
perfTest "PEXSW_EROT_FATAL_ERROR_N" "erot"     "0x0b 1 0x7C 4"
perfTest "HMC_EROT_FATAL_ERROR_N"   "erot"     "0x0b 1 0x7C 5"

perfTest "FPGA_MOD0_PWR_EN"  "HSC_0"     "0x11 2 0x00 0x01 0"

# hsc already in bmc_0

# vr already in gpu_sxm

summary
