#!/usr/bin/env bash

# Copyright (c) 2024, NVIDIA CORPORATION.  All rights reserved.
#
#  NVIDIA CORPORATION and its licensors retain all intellectual property
#  and proprietary rights in and to this software, related documentation
#  and any modifications thereto.  Any use, reproduction, disclosure or
#  distribution of this software and related documentation without an express
#  license agreement from NVIDIA CORPORATION is strictly prohibited.


if [[ "$1" != "-y" ]]; then
    echo "Are you running this test script on hw machine? [y/n]"
    read userInput
    if [[ "$userInput" != "y" ]]; then
        echo "it has to be run on hw, exiting"
        exit 1
    fi
fi

testsTotal=0
testsPassed=0
testsFailed=0

scriptName="./i2c_wrapper"
export DEVICE_NAME_MAPPER="device-id-norm.sh"
ITER_CFG=500

perfTest()
{
    testsTotal=$(($testsTotal + 1))
    local symbol="$1"
    local dev="$2"
    local artOut="$3"
    local rcCode=$4
    local exp="$5"
    local optional_compare_override_rc="$6"

    # workaround to bash quotes stripping
    artOut=${artOut//' '/'_'}

    local cmd="$scriptName -test-run $symbol $dev $artOut $rcCode"
    local resp=$($cmd);

    # workaround - do second quiet run just to catch rc code, above always rets 0
    $cmd 1> /dev/null 2> /dev/null
    local cmdRc=$?

    if [[ -n $optional_compare_override_rc ]]; then
        rcCode=$optional_compare_override_rc
    fi

    if [ "$resp" != "$exp" ] || [ $rcCode -ne $cmdRc ]; then
        testsFailed=$(($testsFailed + 1))
        echo "ERROR: test failed $symbol $dev $artOut $rcCode $exp"

        if [ "$resp" != "$exp" ]; then
            echo "resp $resp != expected $exp"
        fi

        if [ $rcCode -ne $cmdRc ]; then
            echo "resp RC $cmdRc != expected RC $rcCode"
        fi

    else
        testsPassed=$(($testsPassed + 1))
    fi

    echo -n -e "test $testsTotal\r"
}

errorRateTest()
{
    testsTotal=$(($testsTotal + 1))
    local ITER_TO_FINISH=$ITER_CFG
    local ITER=0
    local SUCC=0
    local FAIL=0
    local CMD="$scriptName i2c_access $1"

    echo "start: $(date) using cmd \"$CMD\" iter cnt $ITER_TO_FINISH"

    while true
    do
        OUTPUT=$($CMD)
        ORIGINAL_RC=$?

        if  [[ "$OUTPUT" == *"link-up"* ]]; then
            RC=0
        else
            RC=1
        fi

        ITER=$((ITER+1))

        if [ $RC -ne 0 ]; then
            FAIL=$((FAIL+1))
            echo "ERROR: iter $ITER succ $SUCC fail $FAIL cmd \"$CMD\" out \"$OUTPUT\" rc \"$ORIGINAL_RC\""
        else
            SUCC=$((SUCC+1))
        fi

        if (( ITER % 100 == 0 )); then
            echo "progress: $(date) current iter $ITER / $ITER_TO_FINISH success $SUCC fail $FAIL"
        fi

        if (( ITER >= ITER_TO_FINISH )); then
            echo "finished: $(date) current iter $ITER success $SUCC fail $FAIL"
            break
        fi

    done

    if [ $FAIL -ne 0 ]; then
        testsFailed=$(($testsFailed + 1))
        echo "ERROR: error rate test failed, iter $ITER success $SUCC fail $FAIL"
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
# ./i2c_wrapper -verbose -dry-run i2c_access GPU_SXM_4; echo "rc = $?"
# ./i2c_wrapper -verbose -dry-run i2c_access GPU_SXM_11; echo "rc = $?"

###########################################################
# bmc uses different method (detect instead of get)
perfTest "i2c_access" "BMC_0" "<dummy>" 0 $'link-up'
perfTest "i2c_access" "BMC_0" "<dummy>" 1 $'link-down; rc 100; <dummy>' 0

perfTest "i2c_access" "GPU_SXM_4" "0x11" 0 $'link-up'
perfTest "i2c_access" "GPU_SXM_0" " " 1 "Error: cannot map deviceID (GPU_SXM_0)"
perfTest "i2c_access" "GPU_SXM_4" "Error: Read failed" 1 $'link-down; rc 100; error: read failed' 0

perfTest "i2c_access" "GPU_SXM_4" "Error: Sending messages failed: Protocol error" 1 $'link-down; rc 101' 0
perfTest "i2c_access" "GPU_SXM_4" "Error: Sending messages failed: No such device or address" 1 $'' 2
perfTest "i2c_access" "GPU_SXM_4" "Error: Sending messages failed: Input/output error" 1 $'link-down; rc 102' 0
perfTest "i2c_access" "GPU_SXM_4" "Error: Sending messages failed: Connection timed out" 1 $'link-down; rc 103' 0
perfTest "i2c_access" "GPU_SXM_4" "Error: Sending messages failed: unknown error" 1 $'link-down; rc 100; error: sending messages failed: unknown error' 0

perfTest "i2c_access" "GPU_SXM_1"      "0x11" 0 $'link-up'
perfTest "i2c_access" "GPU_SXM_2"      "0x11" 0 $'link-up'
perfTest "i2c_access" "GPU_SXM_3"      "0x11" 0 $'link-up'
perfTest "i2c_access" "GPU_SXM_4"      "0x11" 0 $'link-up'
perfTest "i2c_access" "GPU_SXM_5"      "0x11" 0 $'link-up'
perfTest "i2c_access" "GPU_SXM_6"      "0x11" 0 $'link-up'
perfTest "i2c_access" "GPU_SXM_7"      "0x11" 0 $'link-up'
perfTest "i2c_access" "GPU_SXM_8"      "0x11" 0 $'link-up'
perfTest "i2c_access" "NVSwitch_0"     "0x11" 0 $'link-up'
perfTest "i2c_access" "NVSwitch_1"     "0x11" 0 $'link-up'
perfTest "i2c_access" "NVSwitch_2"     "0x11" 0 $'link-up'
perfTest "i2c_access" "NVSwitch_3"     "0x11" 0 $'link-up'
perfTest "i2c_access" "PCIeRetimer_0"  "0x11" 0 $'link-up'
perfTest "i2c_access" "PCIeRetimer_1"  "0x11" 0 $'link-up'
perfTest "i2c_access" "PCIeRetimer_2"  "0x11" 0 $'link-up'
perfTest "i2c_access" "PCIeRetimer_3"  "0x11" 0 $'link-up'
perfTest "i2c_access" "PCIeRetimer_4"  "0x11" 0 $'link-up'
perfTest "i2c_access" "PCIeRetimer_5"  "0x11" 0 $'link-up'
perfTest "i2c_access" "PCIeRetimer_6"  "0x11" 0 $'link-up'
perfTest "i2c_access" "PCIeRetimer_7"  "0x11" 0 $'link-up'
perfTest "i2c_access" "PCIeSwitch_0"   "0x11" 0 $'link-up'
perfTest "i2c_access" "FPGA_0"         "0x11" 0 $'link-up'
perfTest "i2c_access" "BMC_0"          "0x11" 0 $'link-up'

echo ""

errorRateTest "GPU_SXM_1"
errorRateTest "GPU_SXM_2"
errorRateTest "GPU_SXM_3"
errorRateTest "GPU_SXM_4"
errorRateTest "GPU_SXM_5"
errorRateTest "GPU_SXM_6"
errorRateTest "GPU_SXM_7"
errorRateTest "GPU_SXM_8"
errorRateTest "NVSwitch_0"
errorRateTest "NVSwitch_1"
errorRateTest "NVSwitch_2"
errorRateTest "NVSwitch_3"
errorRateTest "PCIeRetimer_0"
errorRateTest "PCIeRetimer_1"
errorRateTest "PCIeRetimer_2"
errorRateTest "PCIeRetimer_3"
errorRateTest "PCIeRetimer_4"
errorRateTest "PCIeRetimer_5"
errorRateTest "PCIeRetimer_6"
errorRateTest "PCIeRetimer_7"
errorRateTest "PCIeSwitch_0"
errorRateTest "FPGA_0"
errorRateTest "BMC_0"
###########################################################

summary
