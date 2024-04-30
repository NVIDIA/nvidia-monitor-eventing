/**
 * Copyright (c) 2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#pragma once

#include "data_accessor.hpp"
#include "device_id.hpp"

namespace data_accessor
{

/** @brief Data structure to forward the full result of a check operation */
struct AssertedDevice
{
    /** it always contains the Property value from dbus PropertyChange signal */
    DataAccessor trigger;
    /** contains the data as result of a call to DataAccessor::read() */
    DataAccessor accessor;
    /** the single asserted device, example "GPU_SXM_1" */
    std::string device;
    /** The device index(es) from device(s)
     * Example, Object path: /xyz/openbmc_project/NVSwitch_2/Ports/NVLink_23
     * Supposing the example below, it will contain {2,23}
     */
    device_id::PatternIndex deviceIndexTuple;
};

using AssertedDeviceList = std::vector<AssertedDevice>;

/** indexes from a expanded range */
using DeviceIndexesList = std::vector<device_id::PatternIndex>;

/**
 * @brief A class for performing check against Data Accessor
 *
 */
class CheckAccessor
{
  public:
    /**  It may be useful for Performance purpose
     *     sometimes calling check just once is enough
     */
    enum CheckStatus
    {
        /** no check performed yet, Object just created */
        NotPerformed,
        /** check performed and passed */
        Passed,
        /** check performed and NOT passed */
        NotPassed
    };

    /** constructor
     *  The parameter @a deviceType is by default empty to allow */
    explicit CheckAccessor(const std::string& deviceType)
        : _lastStatus(NotPerformed), _devIdData(deviceType)
    {
        // Empty
    }

    /** default destructor generated by compiler */
    ~CheckAccessor() = default;

    /** @returns true if a check has been performed at least once */
    inline bool performed() const
    {
        return _lastStatus != NotPerformed;
    }

    /** @returns true if previous check has already been performed and passed */
    inline bool passed() const
    {
        return _lastStatus == Passed;
    }

    /** @brief returns true if check() got one or more passed at least once*/
    inline bool hasAssertedDevices() const
    {
        return _assertedDevices.empty() == false;
    }

    /**
     * @brief check if jsonAcc criteria matches dataAcc data
     *
     *   The jsonAcc DataAccessor has the DataAccessor operation criteria which
     *      Json data having "check" parameter which is optional.
     *      If jsonAcc criteria does not contain "check" it passes automatically
     *
     * @param dataAcc if type is "DBUS" and it already contains data
     *                DataAccessor::read() will not be called  for dataAcc.
     *
     * @note  Regarding the flow it is always type DBUS and always contains the
     *        real Objec Path and Property data received from DBus.
     *        So this is the "event.trigger".
     *        Unless _trigger is already set by a previous check() call, dataAcc
     *        will be copied into _trigger member.
     *
     *
     * @return true of the jsonAcc criteria matches the value from dataAcc
     */
    bool check(const DataAccessor& jsonAcc, const DataAccessor& dataAcc);

    /**
     * @brief Performs double check in sequence, joins 2 other check() versions
     * @param jsonTrig The event.trigger
     * @param jsonAcc The event.accessor
     * @param dataAcc  The Accessor got from DUBS PropertyChange or SelfTest
     * @return true if Checks match (all asserted)
     */
    bool check(const DataAccessor& jsonTrig, const DataAccessor& jsonAcc,
               const DataAccessor& dataAcc);

    /** @brief [overloaded]
     *
     *    Uses the information from a previous CheckAccessor (event.trigger)
     *
     * @param triggerCheck a CheckAccessor already performed using event.trigger
     */
    bool check(const DataAccessor& jsonAcc, const CheckAccessor& triggerCheck);

    /**
     * @brief  the @sa check() is supposed to fill _assertedDevices
     *
     * @return the list of Device information
     */
    inline AssertedDeviceList getAssertedDevices() const
    {
        return _assertedDevices;
    }

    /**
     * @brief just a part of the @sa check() logic, it intends for testing
     *      It should be used when dataAcc already has data (after @sa read())
     */
    bool subCheck(const DataAccessor& jsonAcc, DataAccessor& dataAcc,
                  const std::string& dev2Read,
                  const int deviceId = util::InvalidDeviceId);
  private:
    /*
     * @sa check() that is the wrapper for privCheck()
     */
    bool privCheck(const DataAccessor& jsonAcc, DataAccessor& dataAcc);

    /**
     * @brief performs a check() for all devices from 'devices' parameter
     *
     *        The Accessor::read(single-device) is performed on *this
     * @note
     *        *this and dataAcc CANNOT BE THE SAME OBJECT
     *
     * @param deviceIndexes all indexes from the expanded device range
     * @param dataAcc the Accessor which will have the assertedDeviceList
     * @return a single return, true if one or more checks return true
     */
    bool loopDevices(const DeviceIndexesList& deviceIndexes,
                     const DataAccessor& jsonAcc, DataAccessor& dataAcc);

   /**
     * @brief fills the _latestAssertedDevices with a single deviceName
     * @param realDevice  the device name
     */
    bool buildSingleAssertedDeviceName(
        const DataAccessor& dataAcc, const std::string& realDevice,
        const device_id::PatternIndex& patternIndex);

    bool buildSingleAssertedDeviceName(const DataAccessor& dataAcc,
                                       const std::string& realDevice,
                                       const int deviceId);

    /**
     *  @returns the current device (if present) being handled in the check
     *
     *  It uses _devIdData that is supposed to be already populated
     */
    std::string getCurrentDevice();

  private:
    /**  @brief keep the status of the last performed check */
    CheckStatus _lastStatus;

    /**
     * @brief after calling check() it may contain device names with its data
     */
    AssertedDeviceList _assertedDevices;

    /** The event trigger is necessary to populate the AssertedDevice
     *    the second Accessor is used as trigger
     */
    DataAccessor _trigger;

    /** that information comes from a previous check */
    std::string _triggerAssertedDevice;

    /** full information (pattern and indexes) about Event device_type */
    util::DeviceIdData _devIdData;
};

} // namespace data_accessor
