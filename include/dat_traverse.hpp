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

#include "common.hpp"
#include "data_accessor.hpp"
#include "dbus_accessor.hpp"
#include "event_handler.hpp"
#include "event_info.hpp"
#include "util.hpp"

#include <nlohmann/json.hpp>

#include <functional>
#include <list>
#include <map>
#include <string>
#include <system_error>
#include <vector>

using json = nlohmann::json;

namespace dat_traverse
{

struct Status
{
    std::string health;
    std::string healthRollup;
    std::string originOfCondition;
    std::string triState;
};


struct TestPoint
{
    data_accessor::DataAccessor accessor;
    std::string expectedValue;
    util::Severity severity;
};

struct TestLayer
{
    std::map<std::string, TestPoint> testPoints;

    /**
     * @brief Return a list of DEVICE-type testpoints
     *
     * The elements are names of the devices. The elements may be non-unique if
     * the testpoint of the same device was provided more than once in the
     * configuration.
     */
    std::vector<std::string> getDeviceTypeTestpoints();
};

/**
 * @brief Enum type representing the type of the device (regular,
 * sensor, port, firmware).
 */
class DeviceType
{
  public:
    enum types
    {
        UNKNOWN_TYPE = 0,
        REGULAR,
        SENSOR,
        PORT,
        SOFTWARE
    };
    static std::map<std::string, enum types> valuesAllowed;

    DeviceType() : type(DeviceType::types::UNKNOWN_TYPE)
    {}
    DeviceType(const std::string& type);

    DeviceType::types get() const;
    void set(const std::string& type);

    operator std::string() const;

  private:
    enum types type;
};

/** @class Device
 *  @brief Object to represent Device in DAT that
 *         stores info from dat json profile
 */
class Device
{
  public:
    /** @brief Name of the device **/
    std::string name;

    /** @brief Type of the device **/
    DeviceType type;

    /** @brief dbusPathPrimary of the device **/
    std::optional<std::string> dbusPathPrimary;

    /** @brief dbusPathOoc of the device **/
    std::optional<std::string> dbusPathOoc;

    /** @brief Type of the device; default true **/
    std::optional<bool> dbus_set_health;

    /** @brief Downstream devices (children) **/
    std::vector<std::string> association;

    /** @brief Upstream devices **/
    std::vector<std::string> parents;

    /** @brief Struct containing health properties of device **/
    Status healthStatus;

    /** @brief Map of test layers  for device **/
    std::map<std::string, TestLayer> test;

    /**
     * @brief Outputs device data to verify content
     *
     * @param os
     * @param device
     * @return std::ostream&
     */
    friend std::ostream& operator<<(std::ostream& os, const Device& device);

    /** @brief Prints out DAT structure to verify population
     *
     * @param[in]  m  - DAT map
     *
     */
    static void printTree(const std::map<std::string, dat_traverse::Device>& m);

    /** @brief Populates DAT map with JSON profile contents
     *
     * @param[in]  dat     - DAT map
     * @param[in]  file    - name of JSON file
     *
     */
    static void populateMap(std::map<std::string, dat_traverse::Device>& dat,
                            const std::string& file);
    static void populateMap(std::map<std::string, dat_traverse::Device>& dat,
                            const nlohmann::json& js);

    /**
     * @brief Return a list of devices associated (directly) by test layers
     *
     * Filter out from consideration the layers on which @c layerPred returns @c
     * false. The list of devices is unique. No particular order is guaranteed.
     */
    std::vector<std::string> getTestLayerAssociations(
        const std::function<bool(const std::string&)>& layerPred);

    /**
     * @brief Return information about the type of this generalized device
     */
    DeviceType::types getType() const;

    /**
     * @brief Return a dbus path to be used in event logs when
     * this device is identified as an Origin of Condition.
     *
     * Do not attempt to calculate it, simply reflect the configuration.
     */
    std::optional<std::string> getDbusObjectOocSpecificExplicit() const;
    bool hasDbusObjectOocSpecificExplicit() const;

    /**
     * @brief Return the primary dbus object path associated with
     * this device.
     *
     * Example: for device "GPU_SXM_2" the primary dbus object path is
     * "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_2".
     *
     * Do not attempt to calculate it, simply reflect the configuration.
     */
    std::optional<std::string> getDbusObjectPrimaryExplicit();

    /**
     * @brief Tell whether the primary dbus object path associated with this
     * device can have its "Health" properties set
     *
     * Some devices, like sensors (eg. "Chassis_0_PCB_0_Temp_0"), provide the
     * HealthStatus interface but should nevertheless be left alone for
     * performance reasons, even if they are identifies as Origin of Condition,
     * which would normally imply their "Health" property being set to
     * "Critical".
     */
    bool canSetHealthOnDbus() const;

    /**
     * @brief getter for testpoints presence
     * @return true - has tesptoints defined, false - no testpoints found
     */
    bool hasTestpoints() const
    {
        return (this->testpointCount > 0);
    }

    explicit Device(const std::string& s);
    Device(const std::string& s, const json& j);
    ~Device();

  private:
    /** @brief Helper counter of test points not to calculate them over again
     *         as they're static after loading from config file **/
    unsigned int testpointCount;
};

} // namespace dat_traverse

namespace event_handler
{

/**
 * @brief A class for collect device telemetries and update device status based
 * on event.
 *
 */
class DATTraverse : public event_handler::EventHandler
{
  public:
    DATTraverse(const std::string& name = __PRETTY_FUNCTION__) :
        event_handler::EventHandler(name)
    {}
    ~DATTraverse();

  public:
    /**
     * @brief Traverse the Device Association Tree for every device telemetries
     * and update device status.
     *
     * @param event
     * @return eventing::RcCode
     */
    eventing::RcCode process([[maybe_unused]] event_info::EventNode& event) override
    {
        std::string problemDevice = event.device;
        if (problemDevice.length() == 0)
        {
            log_err("%s error: empty device\n", this->getName().c_str());
            return eventing::RcCode::error;
        }

        std::vector<std::function<void(dat_traverse::Device & device,
                                       const dat_traverse::Status& status)>>
            parentCallbacks;
        parentCallbacks.push_back(setHealthProperties);
        parentCallbacks.push_back(setOriginOfCondition);

        parentTraverse(this->dat, problemDevice, hasParents, parentCallbacks);

        return eventing::RcCode::succ;
    }

    /**
     * @brief Print health/healthrollup/OOC/state of branch specified in vector
     * @param dat
     * @param devices
     */
    void printBranch(const std::map<std::string, dat_traverse::Device>& dat,
                     const std::vector<std::string>& devices);

    /**
     * @brief Provide DAT structure so we can traverse it
     * @param dat
     */
    void setDAT(const std::map<std::string, dat_traverse::Device>& dat);

    /**
     *  @brief Return the list of all devices reachable by
     *  "association" relation.
     *
     *  @param[in] dat The device Association Tree the
     *  @rootDevice is part of.
     *
     *  @param[in] rootDevice The device from which to start the
     *  @dat traversal.
     *
     *  @return Vector of device names (keys from @dat map)
     *  reachable from @rootDevice, excluding the @rootDevice
     */
    std::vector<std::string>
        getAssociationConnectedDevices(const std::string& rootDevice);

    /**
     * @brief Populate 'Associations' property of the devices in dbus with
     * "health_rollup" entries.
     *
     * For each device 'D' found among the descendant of
     * '/xyz/openbmc_project/inventory/system/chassis/' in
     * 'xyz.openbmc_project.ObjectMapper' populate the 'Associations' property
     * of the 'xyz.openbmc_project.Association.Definitions' interface through
     * the manager service 'M' for this device (more on it later), with entries
     * of the form:
     *
     *   ("health_rollup", "", d)
     *
     * where 'd' is every device reachable from 'D' in a device association tree
     * given in @ref this->dat (excluding 'D' itself), for which there exists
     * the corresponding object path in 'xyz.openbmc_project.ObjectMapper'
     * ('d' is actually that path itself, not the mere name of the device, eg.
     * '/xyz/openbmc_project/inventory/system/processors/GPU0' instead of
     * 'GPU0').
     *
     * Obtain the managing service 'M' for the given device 'D' by querying
     * object '/xyz/openbmc_project/object_mapper' of the
     * 'xyz.openbmc_project.ObjectMapper' service using method 'GetObject' (eg.
     * 'xyz.openbmc_project.GpuMgr').
     */
    void datToDbusAssociation();

    template <typename T>
    void datToDbusAssociation(dbus::ObjectMapper<T>& om,
                              const std::string& devId)
    {
        using namespace std;

        std::vector<std::string> objPathAssociations;
        for (const auto& devAssoc : this->getAssociationConnectedDevices(devId))
        {
            for (const auto& objPathAssoc : om.getAllDevIdObjPaths(devAssoc))
            {
                objPathAssociations.push_back(objPathAssoc);
            }
        }
        for (const auto& devObjPath : om.getAllDevIdObjPaths(
                 devId, "xyz.openbmc_project.Association.Definitions"))
        {
            std::string manager = om.getManager(
                devObjPath, "xyz.openbmc_project.Association.Definitions");
            if (manager != "")
            {
                auto assocs = objPathAssociations;
                std::erase_if(assocs, [&devObjPath](const string& elem) {
                    return elem == devObjPath;
                });
                dbusSetHealthRollupAssociations(manager, devObjPath, assocs);
            }
        }
    }

    //  private:

    /**
     * @brief searches for vector of associated subdevices
     *
     * @param dat device association tree in memory
     * @param device device to return associations
     * @param doTraverseTestpoints should return all associations by
     * testpoints or by explicit association key (default) of DAT?
     *
     * @return vector of associated devices to device argument
     */
    static std::vector<std::string>
        getSubAssociations(std::map<std::string, dat_traverse::Device>& dat,
                           const std::string& device,
                           const bool doTraverseTestpoints = false);

    /**
     * @brief Return a list of devices reachable from @c rootDevice in an
     * "origin of condition"-oriented fashion
     *
     * "Origin of condition"-oriented fashion means reachability in a graph of
     * associations with the "association" defined as follows:
     *
     * 1. For x != rootDevice: device y is associated with x if y can be found
     * among the recursive testpoints of x
     *
     * 2. For x = rootDevice: device y is associated with rootDevice if the
     * "category" attribute of @c eventNode contains a layer in which the device
     * y is listed as a recursive testpoint.
     *
     * The "recursive testpoint" is something like this
     * @code
     * {
     *   "name": "ERoT_GPU_SXM_1",
     *   "accessor": {
     *     "type": "DEVICE",
     *     "device_name": "ERoT_GPU_SXM_1"
     *   },
     *   "expected_value": "PASS"
     * }
     * @endcode
     *
     * The algorithm thus descends from @c rootDevice down the recursive
     * testpoints, but in the first level of descension only the testpoints
     * within selected layers are considered.
     *
     * As a special case, if the "categories" value is an empty array, or if
     * it's missing entirely (in both cases the @c eventNode.getCategories() is
     * expected to return empty vector) then the condition (2) is reduced to
     * (1).
     *
     * The devices are listed in a breadth-first order, starting from @c
     * rootDevice. The @c rootDevice itself is always included.
     *
     * @param[in] dat
     * @param[in] rootDevice The device on which the @c eventNode occurs
     * @param[in] eventNode
     */
    static std::vector<std::string> getTestLayerSubAssociations(
        std::map<std::string, dat_traverse::Device>& dat,
        const std::string& rootDevice, const event_info::EventNode& eventNode);

    /**
     * @brief Fully traverses a device and stops if predicate
     *        detects an issue in which case action function
     *        will update originOfCondition
     *
     * Walk Device Association Tree @dat starting at @device in a
     * depth-first manner, where the children of @device are taken
     * from its @association field. Apply every function in @action
     * vector to the @dat and currently visited device, respectively
     * (including the starting @device). Use @predicate to determine
     * whether to explore its children: @true - yes, @false - no.
     *
     * Keep track of the visited devices. @predicate will never be
     * called more than once on a device, and neither any of the
     * functions in @action.
     *
     * Return the list of names of visited devices, in the order of
     * visiting. It will always contain the @device as the first
     * element.
     *
     * @param dat
     * @param device
     * @param predicate
     * @param action
     *
     * @return vector of devices which we saw an issue with
     */
    std::vector<std::string> childTraverse(
        std::map<std::string, dat_traverse::Device>& dat,
        const std::string& device,
        const std::function<bool(const dat_traverse::Device& device)> predicate,
        const std::vector<
            std::function<void(std::map<std::string, dat_traverse::Device>& dat,
                               const dat_traverse::Device& device)>>& action);

    /**
     * @brief Fully traverses a device in reverse direction
              (from child to parent) and stops if comparator
     *        detects an issue in which case action function
     *        will update originOfCondition, health,
     *        healthrollup, etc,
     *
     * @param dat
     * @param device
     * @param comparator
     * @param action
     *
     */
    void parentTraverse(
        std::map<std::string, dat_traverse::Device>& dat,
        const std::string& device,
        const std::function<bool(const dat_traverse::Device& device)>
            comparator,
        const std::vector<std::function<void(
            dat_traverse::Device& device, const dat_traverse::Status& status)>>&
            action);

    /**
     * @brief Checks to see if the device has parents
     *
     * @param device
     * @return boolean for whether or not parent(s) were found
     */
    static bool hasParents(const dat_traverse::Device& device);

    /**
     * @brief Checks the health of the device
     *
     * @param device
     * @return boolean for whether or not device is healthy
     */
    static bool checkHealth(const dat_traverse::Device& device);

    /**
     * @brief Sets origion of condition to all upstream devices
     *
     * @param device
     * @param dat
     *
     */
    static void setOriginOfCondition(dat_traverse::Device& targetDevice,
                                     const dat_traverse::Status& status);

    /**
     * @brief Sets healthrollup/state properties
     *
     * @param device
     * @param dat
     *
     */
    static void setHealthProperties(dat_traverse::Device& targetDevice,
                                    const dat_traverse::Status& status);

    /** @brief Data structure for the DAT to traverse **/
    std::map<std::string, dat_traverse::Device> dat;

    // TODO: define Device Association Tree pointer here

  private:
    static void dbusSetHealthRollupAssociations(
        const std::string& manager, const std::string& devicePath,
        const std::vector<std::string>& subAssocDevicePaths);
};

} // namespace event_handler
