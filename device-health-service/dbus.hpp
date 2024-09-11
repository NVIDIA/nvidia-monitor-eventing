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

#include <boost/algorithm/string.hpp>
#include <sdbusplus/bus.hpp>

#include <utility>

// copied from develop, commit d139e40b0cc0c65dd17875aff9167de4ff903d47
namespace dbus
{
constexpr auto freeDesktopInterface = "org.freedesktop.DBus.Properties";
constexpr auto getCall = "Get";
constexpr auto setCall = "Set";

/**
 * It is a common type used in openbmc DBus services
 */
using Association = std::tuple<std::string, std::string, std::string>;

/**
 *     This strange type just replaces std::monostate which should be a class
 *     and due that it is not supported by sdbusplus functions that requires
 *     std::variant as parameter
 *
 *     It is not expected to use a variable of this type
 **/
using InvalidMonoState =
    std::map<uint16_t, std::map<uint16_t, std::map<uint16_t, uint16_t>>>;

/**
 *  Variant type used for Dbus properties
 *  (only types needed for /xyz/openbmc_project/logging/entry/<id> interfaces)
 */
using PropertyVariant =
    std::variant<InvalidMonoState, bool, uint32_t, uint64_t, std::string,
                 std::vector<std::string>, std::vector<Association>>;

/**
 * @brief returns the service assigned with objectPath and interface
 * @param objectPath
 * @param interface
 * @return service name
 */
std::string getService(const std::string& objectPath,
                       const std::string& interface);

/**
 * @brief setDbusProperty() sets a value for a Dbus property
 * @param service
 * @param objPath
 * @param interface
 * @param property
 * @param val the new value to be set
 * @return true if could set this the value from 'val', false otherwise
 */
bool setDbusProperty(const std::string& service, const std::string& objPath,
                     const std::string& interface, const std::string& property,
                     const PropertyVariant& val);

/**
 * @brief setDbusProperty() just an overload function that calls getService()
 *                          to get the service for objPath and interface
 * @param objPath
 * @param interface
 * @param property
 * @param val the new value to be set
 * @return true if could set this the value from 'val', false otherwise
 */
bool setDbusProperty(const std::string& objPath, const std::string& interface,
                     const std::string& property, const PropertyVariant& val);

template <typename T>
class ObjectMapper
{
  public:
    // It's useful to have this field public for when a user wants to call a
    // method on some other service than ObjectMapper, but using the same bus.
    sdbusplus::bus::bus bus;

    /** @brief Manager -> Interface* */
    using ValueType = std::map<std::string, std::vector<std::string>>;

    /**
     * @brief Object -> (Manager -> Interface*)
     *
     * A dictionary mapping each object to a dictionay mapping a manager to a
     * list of defined interfaces. In dbus terminology "a{sa{sas}}"
     */
    using FullTreeType = std::map<std::string, ValueType>;

    ObjectMapper() : bus(sdbusplus::bus::new_default_system())
    {}

    ObjectMapper(sdbusplus::bus::bus&& bus) : bus(std::move(bus))
    {}

    /**
     * @brief Mimic the 'GetObject' method of 'ObjectMapper'
     * without actually using the dbus
     *
     * @code
     * - name: GetObject
     *   description: >
     *       Obtain a dictionary of service -> implemented interface(s)
     *       for the given path.
     *   parameters:
     *       - name: path
     *         type: path
     *         description: >
     *             The object path for which the result should be fetched.
     *       - name: interfaces
     *         type: array[string]
     *         description: >
     *             An array of result set constraining interfaces.
     *   returns:
     *       - name: services
     *         type: dict[string,array[string]]
     *         description: >
     *             A dictionary of service -> implemented interface(s).
     *   errors:
     *       - xyz.openbmc_project.Common.Error.ResourceNotFound
     * @endcode
     */

    ValueType getObject(const std::string& objectPath,
                        const std::vector<std::string>& interfaces = {})
    {
        return static_cast<T*>(this)->getObjectImpl(this->bus, objectPath,
                                                    interfaces);
    }

    ValueType getObject(const std::string& objectPath,
                        const std::string& interface)
    {
        return getObject(objectPath, std::vector<std::string>{interface});
    }

    /**
     * @brief Mimic the 'GetSubTreePaths' method of 'ObjectMapper' without
     * actually using the dbus.
     *
     * From the "ObjectMapper.interface.yaml":
     *
     * @code
     * - name: GetSubTreePaths
     *   description: >
     *       Obtain an array of paths where array elements are in subtree.
     *   parameters:
     *       - name: subtree
     *         type: path
     *         description: >
     *             The subtree path for which the result should be fetched.
     *       - name: depth
     *         type: int32
     *         description: >
     *             The maximum subtree depth for which results should be
     * fetched. For unconstrained fetches use a depth of zero.
     *       - name: interfaces
     *         type: array[string]
     *         description: >
     *             An array of result set constraining interfaces.
     *   returns:
     *       - name: paths
     *         type: array[path]
     *         description: >
     *             An array of paths.
     *   errors:
     *       - xyz.openbmc_project.Common.Error.ResourceNotFound
     * @endcode
     *
     * From the command line perspective this function corresponds to a call
     *
     *   busctl call                                      \
     *     xyz.openbmc_project.ObjectMapper               \
     *     /xyz/openbmc_project/object_mapper             \
     *     xyz.openbmc_project.ObjectMapper               \
     *     GetSubTreePaths sias                           \
     *     @subtree                                       \
     *     @depth                                         \
     *     N @interfaces[0] ... @interfaces[N-1]
     *
     * @return Something along
     *
     *   [
     *     ...
     *     "/xyz/openbmc_project/inventory/system/chassis/GPU5"
     *     "/xyz/openbmc_project/inventory/system/chassis/GPU6"
     *     "/xyz/openbmc_project/inventory/system/chassis/GPU7"
     *     "/xyz/openbmc_project/inventory/system/chassis/NVSwitch0"
     *     "/xyz/openbmc_project/inventory/system/chassis/NVSwitch1"
     *     ...
     *   ]
     *
     * or an empty vector if error in method call occured.
     */
    std::vector<std::string>
        getSubTreePaths(const std::string& subtree, int depth,
                        const std::vector<std::string>& interfaces = {})
    {
        return static_cast<T*>(this)->getSubTreePathsImpl(this->bus, subtree,
                                                          depth, interfaces);
    }

    std::vector<std::string> getSubTreePaths(const std::string& subtree,
                                             const std::string& interface)
    {
        return getSubTreePaths(subtree, 0, std::vector<std::string>{interface});
    }

    std::vector<std::string> getSubTreePaths(const std::string& interface)
    {
        return getSubTreePaths("/", interface);
    }

    std::vector<std::string> getSubTreePaths()
    {
        return getSubTreePaths("/", 0);
    }

    /**
     * @brief Mimic the 'GetSubTree' method of 'ObjectMapper' without actually
     * using the dbus.
     *
     * From the "ObjectMapper.interface.yaml":
     *
     * @code
     * - name: GetSubTree
     *   description: >
     *       Obtain a dictionary of path -> services where path is in
     *       sutbtree and services is of the type returned by the
     *       GetObject method.
     *   parameters:
     *       - name: subtree
     *         type: path
     *         description: >
     *             The subtree path for which the result should be fetched.
     *       - name: depth
     *         type: int32
     *         description: >
     *             The maximum subtree depth for which results should be
     * fetched. For unconstrained fetches use a depth of zero.
     *       - name: interfaces
     *         type: array[string]
     *         description: >
     *             An array of result set constraining interfaces.
     *   returns:
     *       - name: objects
     *         type: dict[path,dict[string,array[string]]]
     *         description: >
     *             A dictionary of path -> services.
     *   errors:
     *       - xyz.openbmc_project.Common.Error.ResourceNotFound
     * @endcode
     */
    FullTreeType getSubtree(const std::string& subtree, int depth,
                            const std::vector<std::string>& interfaces = {})
    {
        return static_cast<T*>(this)->getSubtreeImpl(this->bus, subtree, depth,
                                                     interfaces);
    }

    /**
     * @brief Get a list of all object paths from @c
     * xyz.openbmc_project.ObjectMapper tree corresponding to the given @c devId
     * under any possible context, and implementing any of the @c interfaces.
     *
     * For example, for the @c GPU_SXM_3 return a list similar to
     *
     * "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_3",
     * "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_3/PCIeDevices/GPU_SXM_3",
     * "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Endpoints/GPU_SXM_3",
     * "/xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_2/Endpoints/GPU_SXM_3",
     * "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_3"
     *
     */

    std::vector<std::string>
        getAllDevIdObjPaths(const std::string& devId,
                            const std::vector<std::string>& interfaces = {})
    {
        return getDevIdPaths(interfaces, [&devId](const std::string& objPath) {
            return !(isObjPathPrimaryDevId(objPath, devId) ||
                     isObjPathDerivativeDevId(objPath, devId));
        });
    }

    std::vector<std::string> getAllDevIdObjPaths(const std::string& devId,
                                                 const std::string& interface)
    {
        return getAllDevIdObjPaths(devId, std::vector<std::string>{interface});
    }

    /**
     * @brief Get a list of all object paths from @c
     * xyz.openbmc_project.ObjectMapper tree corresponding directly to the given
     * @c devId, and implementing any of the @c interfaces.
     *
     * For example, for the @c GPU_SXM_3 return a list similar to
     *
     * "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_3"
     *
     */

    std::vector<std::string>
        getPrimaryDevIdPaths(const std::string& devId,
                             const std::vector<std::string>& interfaces = {})
    {
        return getDevIdPaths(interfaces, [&devId](const std::string& objPath) {
            return !isObjPathPrimaryDevId(objPath, devId);
        });
    }

    /**
     * @brief
     *
     * Simplified version of @c getObject.
     */
    /*
    std::string getManager(const std::string& objectPath,
                           const std::string& interface)
    {
        ValueType ret =
            this->getObject(objectPath, std::vector<std::string>{interface});
        if (ret.size() == 0)
        {
            log_err("Requested a manager for the object path '%s'"
                    "of the 'xyz.openbmc_project.ObjectMapper', "
                    "but none found\n",
                    objectPath.c_str());
            return "";
        }
        else if (ret.size() > 1)
        {
            std::stringstream ss;
            int i = 0;
            for (const auto& [key, value] : ret)
            {
                ss << (i++ == 0 ? "" : ", ") << "'" << key << "'";
            }
            log_err("Requested a manager for the object path '%s'"
                    "of the 'xyz.openbmc_project.ObjectMapper', "
                    "but multiple managers found (%s)\n",
                    objectPath.c_str(), ss.str().c_str());
            return "";
        }
        else
        {
            return ret.cbegin()->first;
        }
    }

    sdbusplus::message::message getMethod(const std::string& objectPath,
                                          const std::string& managerInterface,
                                          const std::string& callInterface,
                                          const std::string& method)
    {
        return this->bus.new_method_call(
            this->getManager(objectPath, managerInterface), objectPath,
            callInterface, method);
    }
    */

  private:
    std::vector<std::string> getDevIdPaths(
        const std::vector<std::string>& interfaces,
        const std::function<bool(const std::string& objPath)>& antiPredicate)
    {
        return scopeObjectPathsDevId(
            this->getSubTreePaths(std::string("/"), 0, interfaces),
            antiPredicate);
    }

    static std::vector<std::string> scopeObjectPathsDevId(
        std::vector<std::string> objectPaths,
        const std::function<bool(const std::string& objPath)>& antiPredicate)
    {
        std::erase_if(objectPaths, antiPredicate);
        return objectPaths;
    }

    /**
     * Examples of primary object paths:
     *
     *   "/xyz/openbmc_project/inventory/system/chassis/HGX_Chassis_0",
     *   "/xyz/openbmc_project/inventory/system/chassis/HGX_FPGA_0",
     *   "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_1",
     *   "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_2",
     *   "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_3",
     *   "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_4",
     *   "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_5",
     *   "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_6",
     *   "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_7",
     *   "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_8",
     *   "/xyz/openbmc_project/inventory/system/chassis/HGX_NVSwitch_0",
     *   "/xyz/openbmc_project/inventory/system/chassis/HGX_NVSwitch_1",
     *   "/xyz/openbmc_project/inventory/system/chassis/HGX_NVSwitch_2",
     *   "/xyz/openbmc_project/inventory/system/chassis/HGX_NVSwitch_3",
     *   "/xyz/openbmc_project/inventory/system/chassis/HGX_PCIeRetimer_0",
     *   "/xyz/openbmc_project/inventory/system/chassis/HGX_PCIeRetimer_1",
     *   "/xyz/openbmc_project/inventory/system/chassis/HGX_PCIeRetimer_2",
     *   "/xyz/openbmc_project/inventory/system/chassis/HGX_PCIeRetimer_3",
     *   "/xyz/openbmc_project/inventory/system/chassis/HGX_PCIeRetimer_4",
     *   "/xyz/openbmc_project/inventory/system/chassis/HGX_PCIeRetimer_5",
     *   "/xyz/openbmc_project/inventory/system/chassis/HGX_PCIeRetimer_6",
     *   "/xyz/openbmc_project/inventory/system/chassis/HGX_PCIeRetimer_7",
     *   "/xyz/openbmc_project/inventory/system/chassis/HGX_PCIeSwitch_0"
     */
    static bool isObjPathPrimaryDevId(const std::string& objPath,
                                      const std::string& devId)
    {
        return (objPath ==
                "/xyz/openbmc_project/inventory/system/chassis/" + devId) ||
               (objPath ==
                "/xyz/openbmc_project/inventory/system/chassis/HGX_" + devId);
    }

    /**
     * Example of derivative object paths corresponding to the GPU_SXM_5 @c
     * devId:
     *
     * /xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_5/PCIeDevices/GPU_SXM_5
     * /xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Endpoints/GPU_SXM_5
     * /xyz/openbmc_project/inventory/system/fabrics/HGX_PCIeRetimerTopology_4/Endpoints/GPU_SXM_5
     * /xyz/openbmc_project/inventory/system/processors/GPU_SXM_5
     */
    static bool isObjPathDerivativeDevId(const std::string& objPath,
                                         const std::string& devId)
    {
        return boost::algorithm::ends_with(objPath, "/" + devId);
    }
};

class DirectObjectMapper : public ObjectMapper<DirectObjectMapper>
{

  public:
    DirectObjectMapper()
    {}

    DirectObjectMapper(sdbusplus::bus::bus&& bus) : ObjectMapper(std::move(bus))
    {}

    ValueType getObjectImpl(sdbusplus::bus::bus& bus,
                            const std::string& objectPath,
                            const std::vector<std::string>& interfaces) const;

    std::vector<std::string>
        getSubTreePathsImpl(sdbusplus::bus::bus& bus,
                            const std::string& subtree, int depth,
                            const std::vector<std::string>& interfaces) const;

    FullTreeType
        getSubtreeImpl(sdbusplus::bus::bus& bus, const std::string& subtree,
                       int depth,
                       const std::vector<std::string>& interfaces) const;
};

/**
 * @brief Makes D-Bus calls to set the device Health to the specified value
 * ("OK"/"Warning"/"Critical")
 *
 * Updating the device Health requires 3 steps:
 * 1. Finding the object path(s) corresponding to the device with a Health
 * property
 * 2. Finding the D-Bus service that created each path
 * 3. Actually writing to the service and object path found in steps 1 and 2
 * (The Health interface is fixed: xyz.openbmc_project.State.Decorator.Health)
 *
 * @return @c true if the device Health was successfully updated, @c false
 * otherwise (D-Bus exception, object path does not exist, write failed, etc.)
 */
bool setDeviceHealth(const std::string& device, const std::string& health);

} // namespace dbus
