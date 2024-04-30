#!/bin/bash

# Copyright (c) 2024, NVIDIA CORPORATION.  All rights reserved.
#
#  NVIDIA CORPORATION and its licensors retain all intellectual property
#  and proprietary rights in and to this software, related documentation
#  and any modifications thereto.  Any use, reproduction, disclosure or
#  distribution of this software and related documentation without an express
#  license agreement from NVIDIA CORPORATION is strictly prohibited.

I=$'\r'

# Google sheet encodes newlines inside a cell as a 'carriage
# return' ('\r') character. This complicates the csv file
# handling.

# Convert 'carriage return' characters into dollar signs '$' (but
# not those at the end of line in a '\r\n' sequence). This can
# remove the multi-line cells mess while keeping information
# about the newlines (not ideal though)

sed -e "s/${I}\(.\)/\\$\1/g" < "$1"
