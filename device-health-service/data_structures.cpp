/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES.
 * All rights reserved. SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "data_structures.hpp"

#include <phosphor-logging/lg2.hpp>

#include <stdexcept>

// Private

size_t TmpFileManager::getActiveCriticalErrorCountForDeviceUnchecked(
    const std::string& device_)
{
    shared_string device(device_.c_str(), file->get_allocator<char>());
    if (assertedEventsMap->contains(device))
    {
        AssertedErrorsForDevice& aefd = assertedEventsMap->at(device);
        return aefd.criticalErrors.size();
    }
    return 0;
}

size_t TmpFileManager::getActiveWarningErrorCountForDeviceUnchecked(
    const std::string& device_)
{
    shared_string device(device_.c_str(), file->get_allocator<char>());
    if (assertedEventsMap->contains(device))
    {
        AssertedErrorsForDevice& aefd = assertedEventsMap->at(device);
        return aefd.warningErrors.size();
    }
    return 0;
}

bool TmpFileManager::addCriticalErrorToDeviceUnchecked(
    const std::string& errorId_, const std::string& device_)
{
    shared_string device(device_.c_str(), file->get_allocator<char>());
    ensureMappingPresent(device);
    // Call at() to get a reference to the persistent object in shared memory
    AssertedErrorsForDevice& aefd = assertedEventsMap->at(device);
    if (aefd.criticalErrors.size() >= MAX_ASSERTED_ERROR_IDS_PER_DEV_SEV)
    {
        return false;
    }
    shared_string errorId(errorId_.c_str(), file->get_allocator<char>());
    aefd.criticalErrors.insert(errorId);
    return true;
}

bool TmpFileManager::addWarningErrorToDeviceUnchecked(
    const std::string& errorId_, const std::string& device_)
{
    shared_string device(device_.c_str(), file->get_allocator<char>());
    ensureMappingPresent(device);
    // Call at() to get a reference to the persistent object in shared memory
    AssertedErrorsForDevice& aefd = assertedEventsMap->at(device);
    if (aefd.warningErrors.size() >= MAX_ASSERTED_ERROR_IDS_PER_DEV_SEV)
    {
        return false;
    }
    shared_string errorId(errorId_.c_str(), file->get_allocator<char>());
    aefd.warningErrors.insert(errorId);
    return true;
}

bool TmpFileManager::removeErrorFromDeviceUnchecked(const std::string& errorId_,
                                                    const std::string& device_)
{
    shared_string device(device_.c_str(), file->get_allocator<char>());
    if (!assertedEventsMap->contains(device))
    {
        return false;
    }
    // Call at() to get a reference to the persistent object in shared memory
    AssertedErrorsForDevice& aefd = assertedEventsMap->at(device);
    shared_string errorId(errorId_.c_str(), file->get_allocator<char>());
    size_t criticalRemovedCount = aefd.criticalErrors.erase(errorId);
    size_t warningRemovedCount = aefd.warningErrors.erase(errorId);
    return (criticalRemovedCount + warningRemovedCount) != 0;
}

void TmpFileManager::ensureMappingPresent(const shared_string& device)
{
    // if the mapping does not exist, construct a new mapping and insert it
    if (!assertedEventsMap->contains(device))
    {
        AssertedErrorsForDevice newAefd(file->get_allocator<shared_string>());
        assertedEventsMap->insert(std::make_pair(device, newAefd));
    }
}

bool TmpFileManager::growRegion()
{
    size_t currentSize = file->get_size();
    size_t newSize = currentSize + INCREMENT_SHMEM_SIZE;
    lg2::warning("resize requested: current: {CURRENTSIZE}, newSize: {NEWSIZE}"
                 ", maxSize: {MAXSIZE}",
                 "CURRENTSIZE", currentSize, "NEWSIZE", newSize, "MAXSIZE",
                 MAXIMUM_SHMEM_SIZE);
    if (newSize > MAXIMUM_SHMEM_SIZE)
    {
        lg2::critical("resizing to {NEWSIZE} would exceed the maximum shared "
                      "memory size limit of {MAXSIZE}",
                      "NEWSIZE", newSize, "MAXSIZE", MAXIMUM_SHMEM_SIZE);
        return false;
    }
    // first, region needs to be taken offline.
    // This will invalidate any maps, sets, strings, etc.
    assertedEventsMap = nullptr;
    file.reset(nullptr); // deletes the object, closing the mapping
    // Now, grow the region
    bool result =
        bip::managed_mapped_file::grow(MAPPED_FILE_NAME, INCREMENT_SHMEM_SIZE);
    if (!result)
    {
        lg2::critical("bip::managed_mapped_file::grow failed!");
        throw std::runtime_error("bip::managed_mapped_file::grow failed!");
    }
    file = std::make_unique<bip::managed_mapped_file>(
        bip::open_or_create, MAPPED_FILE_NAME, INITIAL_SHMEM_SIZE);
    if (!file)
    {
        lg2::critical("file reopen failed!");
        throw std::runtime_error("failed to reopen shared memory file after "
                                 "closing it for resizing");
    }
    lg2::warning("file reopened");
    assertedEventsMap = file->find<shared_map>("DeviceAssertedErrorsMap").first;
    if (!assertedEventsMap)
    {
        lg2::critical("assertedEventsMap no longer in file!");
        throw std::runtime_error("data structure missing from shared memory "
                                 "after reopening it after resize");
    }
    lg2::warning("assertedEventsMap found in resized file");

    return true;
}
