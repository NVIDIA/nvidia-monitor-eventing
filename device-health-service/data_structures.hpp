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

#pragma once

#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/set.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <phosphor-logging/lg2.hpp>

#include <functional>
#include <memory>
#include <set>

#ifndef MAPPED_FILE_NAME
#define MAPPED_FILE_NAME "/tmp/device-health-service-shmem"
#endif // MAPPED_FILE_NAME

#ifndef INITIAL_SHMEM_SIZE
#define INITIAL_SHMEM_SIZE 65536
#endif // INITIAL_SHMEM_SIZE

#ifndef INCREMENT_SHMEM_SIZE
#define INCREMENT_SHMEM_SIZE 65536
#endif // INCREMENT_SHMEM_SIZE

#ifndef MAXIMUM_SHMEM_SIZE
#define MAXIMUM_SHMEM_SIZE 1048576 // 1MiB
#endif                             // MAXIMUM_SHMEM_SIZE

#ifndef MAX_ASSERTED_ERROR_IDS_PER_DEV_SEV
#define MAX_ASSERTED_ERROR_IDS_PER_DEV_SEV 50
#endif // MAX_ASSERTED_ERROR_IDS_PER_DEV_SEV

namespace bip = boost::interprocess;
using segment_manager_t = bip::managed_mapped_file::segment_manager;
// using void_allocator_t = bip::allocator<void, segment_manager_t>;
using shared_string =
    bip::basic_string<char, std::char_traits<char>,
                      bip::allocator<char, segment_manager_t>>;
using shared_set = bip::set<shared_string, std::less<shared_string>,
                            bip::allocator<shared_string, segment_manager_t>>;
struct AssertedErrorsForDevice
{
    AssertedErrorsForDevice(
        const bip::allocator<shared_string, segment_manager_t>& allocator) :
        criticalErrors(allocator),
        warningErrors(allocator)
    {}
    shared_set criticalErrors;
    shared_set warningErrors;
};
using shared_map = bip::map<
    shared_string, AssertedErrorsForDevice, std::less<shared_string>,
    bip::allocator<std::pair<const shared_string, AssertedErrorsForDevice>,
                   segment_manager_t>>;

class TmpFileManager
{
  public:
    TmpFileManager() :
        file(std::make_unique<bip::managed_mapped_file>(
            bip::open_or_create, MAPPED_FILE_NAME, INITIAL_SHMEM_SIZE)),
        assertedEventsMap(nullptr)
    {
        lg2::warning("create TmpFileManager");
        assertedEventsMap =
            file->find_or_construct<shared_map>("DeviceAssertedErrorsMap")(
                file->get_allocator<
                    std::pair<const shared_string, AssertedErrorsForDevice>>());
    }

    std::set<std::string> getKnownDeviceSet()
    {
        std::set<std::string> out;
        for (const auto& [key, value] : *assertedEventsMap)
        {
            out.insert(std::string(key.c_str()));
        }
        return out;
    }

    size_t getActiveCriticalErrorCountForDevice(const std::string& device_)
    {
        return callWithAllocHandling<size_t>([this, device_]() {
            return getActiveCriticalErrorCountForDeviceUnchecked(device_);
        });
    }

    size_t getActiveWarningErrorCountForDevice(const std::string& device_)
    {
        return callWithAllocHandling<size_t>([this, device_]() {
            return getActiveWarningErrorCountForDeviceUnchecked(device_);
        });
    }

    /**
     * @return @c false if active error ID limit reached, otherwise @c true
     */
    bool addCriticalErrorToDevice(const std::string& errorId_,
                                  const std::string& device_)
    {
        return callWithAllocHandling<bool>([this, errorId_, device_]() {
            return addCriticalErrorToDeviceUnchecked(errorId_, device_);
        });
    }

    /**
     * @return @c false if active error ID limit reached, otherwise @c true
     */
    bool addWarningErrorToDevice(const std::string& errorId_,
                                 const std::string& device_)
    {
        return callWithAllocHandling<bool>([this, errorId_, device_]() {
            return addWarningErrorToDeviceUnchecked(errorId_, device_);
        });
    }

    bool removeErrorFromDevice(const std::string& errorId_,
                               const std::string& device_)
    {
        return callWithAllocHandling<bool>([this, errorId_, device_]() {
            return removeErrorFromDeviceUnchecked(errorId_, device_);
        });
    }

    ~TmpFileManager()
    {}

  private:
    std::unique_ptr<bip::managed_mapped_file> file;
    shared_map* assertedEventsMap;

    void ensureMappingPresent(const shared_string& device);

    size_t getActiveCriticalErrorCountForDeviceUnchecked(
        const std::string& device_);
    size_t getActiveWarningErrorCountForDeviceUnchecked(
        const std::string& device_);
    bool addCriticalErrorToDeviceUnchecked(const std::string& errorId_,
                                           const std::string& device_);
    bool addWarningErrorToDeviceUnchecked(const std::string& errorId_,
                                          const std::string& device_);
    bool removeErrorFromDeviceUnchecked(const std::string& errorId_,
                                        const std::string& device_);

    template <typename R>
    R callWithAllocHandling(const auto& func)
    {
        try
        {
            return func();
        }
        catch (boost::interprocess::bad_alloc& e)
        {
            // TODO: what if it's a big allocation and we only retry once?
            lg2::error("allocation failed: {WHAT}", "WHAT", e.what());
            lg2::error("try to increase region size");
            try
            {
                growRegion();
            }
            catch (std::exception& e)
            {
                lg2::critical("Exception from growRegion: {WHAT} "
                              "\n\tcannot continue, aborting",
                              "WHAT", e.what());
                throw;
            }
            lg2::info("retry operation");
            try
            {
                return func();
            }
            catch (boost::interprocess::bad_alloc& e)
            {
                lg2::critical("allocation still failed, giving up: {WHAT}",
                              "WHAT", e.what());
                throw;
            }
        }
    }

    bool growRegion();
};
