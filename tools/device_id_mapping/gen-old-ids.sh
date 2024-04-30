#!/usr/bin/bash

# Copyright (c) 2024, NVIDIA CORPORATION.  All rights reserved.
#
#  NVIDIA CORPORATION and its licensors retain all intellectual property
#  and proprietary rights in and to this software, related documentation
#  and any modifications thereto.  Any use, reproduction, disclosure or
#  distribution of this software and related documentation without an express
#  license agreement from NVIDIA CORPORATION is strictly prohibited.


echo -n > $2
for newId in $(cat $1); do
    result=$( ./device-id-norm-gen.sh ${newId} )
    if (( $? != 0 )); then
        echo "ERROR: ./device-id-norm-gen.sh ${newId}"
    else
        echo "${result}" >> $2
    fi
done;
exit 0
