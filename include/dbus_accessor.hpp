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

#include "log.hpp"
#include "property_accessor.hpp"

#include <boost/algorithm/string.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/bus.hpp>

#include <cassert>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <regex>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

namespace dbus
{

constexpr auto freeDesktopInterface = "org.freedesktop.DBus.Properties";
constexpr auto getCall = "Get";
constexpr auto setCall = "Set";

using DbusPropertyChangedHandler = std::unique_ptr<sdbusplus::bus::match_t>;
using CallbackFunction = sdbusplus::bus::match::match::callback_t;
using DbusAsioConnection = std::shared_ptr<sdbusplus::asio::connection>;

/**
 * @brief register for receiving signals from Dbus PropertyChanged
 *
 * @param conn        connection std::shared_ptr<sdbusplus::asio::connection>
 * @param objectPath  Dbus object path
 * @param interface   Dbus interface
 * @param callback    the callback function
 * @return the match_t register information which cannot be destroyed
 *         while receiving these Dbus signals
 */
DbusPropertyChangedHandler registerServicePropertyChanged(
    DbusAsioConnection conn, const std::string& objectPath,
    const std::string& interface, CallbackFunction callback);

/**
 * @brief overloaded function
 * @param bus       the bus type sdbusplus::bus::bus&
 * @param objectPath
 * @param interface
 * @param callback
 * @return
 */
DbusPropertyChangedHandler registerServicePropertyChanged(
    sdbusplus::bus::bus& bus, const std::string& objectPath,
    const std::string& interface, CallbackFunction callback);

/**
 *  @brief this is the return type for @sa deviceGetCoreAPI()
 */
using RetCoreApi = std::tuple<int, std::string, uint64_t>; // int = return code

/**
 * @brief returns the service assigned with objectPath and interface
 * @param objectPath
 * @param interface
 * @return service name
 */
std::string getService(const std::string& objectPath,
                       const std::string& interface);

/**
 * @brief Performs a DBus call in GpuMgr service calling DeviceGetData method
 * @param devId
 * @param property  the name of the property which defines which data to get
 * @return RetCoreApi with the information
 */
RetCoreApi deviceGetCoreAPI(const int devId, const std::string& property);

/**
 * @brief clear information present GpuMgr service DeviceGetData method
 * @param devId
 * @param property
 * @return 0 meaning success or other value to indicate an error
 */
int deviceClearCoreAPI(const int devId, const std::string& property);

/**
 * @brief getDbusProperty() gets the value from a property in DBUS
 * @param objPath
 * @param interface
 * @param property
 * @return the value based on std::variant
 */
PropertyVariant readDbusProperty(const std::string& objPath,
                                 const std::string& interface,
                                 const std::string& property);

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

/**
 * @brief Class implementing the ObjectMapper interface of the ObjectMapper
 * service
 *
 * @code
 * xyz.openbmc_project.ObjectMapper    interface -         -            -
 * .GetAncestors                       method    sas       a{sa{sas}}   -
 * .GetObject                          method    sas       a{sas}       -
 * .GetSubTree                         method    sias      a{sa{sas}}   -
 * .GetSubTreePaths                    method    sias      as           -
 * @endcode
 *
 * Of the 4 methods only 'GetObject' and 'GetSubTreePaths' proved to be useful,
 * so the rest is not supported atm.
 */

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

class CachingObjectMapper : public ObjectMapper<CachingObjectMapper>
{

  public:
    CachingObjectMapper() : ObjectMapper(), isInitialized(false)
    {}

    CachingObjectMapper(sdbusplus::bus::bus&& bus) :
        ObjectMapper(std::move(bus)), isInitialized(false)
    {}

    ValueType getObjectImpl(sdbusplus::bus::bus& bus,
                            const std::string& objectPath,
                            const std::vector<std::string>& interfaces);

    std::vector<std::string>
        getSubTreePathsImpl(sdbusplus::bus::bus& bus,
                            const std::string& subtree, int depth,
                            const std::vector<std::string>& interfaces);
    std::vector<std::string>
        getSubTreePathsImpl(sdbusplus::bus::bus& bus,
                            const std::vector<std::string>& interfaces);

    // Not implemented for now
    FullTreeType getSubtreeImpl(sdbusplus::bus::bus& bus,
                                const std::string& subtree, int depth,
                                const std::vector<std::string>& interfaces);

    /** @brief Synchronize the internal mirror data (@c
     * objectsServicesMapping) with dbus **/
    void refresh();
    // void refresh(sdbusplus::bus::bus& bus);

    /**
     * @brief Given one of the values from the main dictionary return the set of
     * managers implementing a given interfaces (disjunction).
     */
    static ValueType scopeManagers(const ValueType& implementations,
                                   const std::vector<std::string>& interfaces);

  private:
    bool isInitialized;
    FullTreeType objectsServicesMapping;

    void ensureIsInitialized();
};

// DbusDelayer ////////////////////////////////////////////////////////////////

/**
 * @brief
 *
 * +---------------------+
 * |        idle         | <+
 * +---------------------+  |
 *   |                      |
 *   | callStartAttempt()   |
 *   v                      |
 * +---------------------+  |
 * |       waiting       |  | callFinished()
 * +---------------------+  |
 *   |                      |
 *   | callStartActual()    |
 *   v                      |
 * +---------------------+  |
 * |       calling       | -+
 * +---------------------+
 */

class DbusDelayer
{
  public:
    enum State
    {
        idle,
        waiting,
        calling
    };

    static const char* stateToStr(State state);

    DbusDelayer() : mutex(), state(State::idle)
    {}
    virtual ~DbusDelayer() = default;

    std::chrono::milliseconds callStartAttempt(const std::string& signature);
    void callStartActual(const std::string& signature);
    void callFinished(const std::string& signature);

    State getState() const
    {
        return this->state;
    }

    std::mutex mutex;

  protected:
    virtual std::chrono::milliseconds callStartAttemptImpl(
        const std::string& signature,
        const std::chrono::time_point<std::chrono::steady_clock>& now);
    virtual void callStartActualImpl(
        const std::string& signature,
        const std::chrono::time_point<std::chrono::steady_clock>& now);
    virtual void callFinishedImpl(
        const std::string& signature,
        const std::chrono::time_point<std::chrono::steady_clock>& now);

  private:
    State state;
};

class DbusDelayerConstLowerBound : public DbusDelayer
{
  public:
    DbusDelayerConstLowerBound() :
        _waitTimeLowerBound(std::chrono::milliseconds(0)),
        _lastCallFinish(std::chrono::steady_clock::now())
    {}
    DbusDelayerConstLowerBound(
        const std::chrono::milliseconds& waitTimeLowerBound) :
        _waitTimeLowerBound(waitTimeLowerBound),
        _lastCallFinish(std::chrono::steady_clock::now())
    {}

    void setDelayTime(const std::chrono::milliseconds& waitTimeLowerBound);

  protected:
    std::chrono::milliseconds callStartAttemptImpl(
        const std::string& signature,
        const std::chrono::time_point<std::chrono::steady_clock>& now);
    void callFinishedImpl(
        const std::string& signature,
        const std::chrono::time_point<std::chrono::steady_clock>& now);

  private:
    std::chrono::milliseconds _waitTimeLowerBound;
    std::chrono::time_point<std::chrono::steady_clock> _lastCallFinish;
};

extern DbusDelayerConstLowerBound defaultDbusDelayer;

/**
 * @brief Makes sure the underlying DbusDelayer object is in proper state
 */

class DbusDelayerStateGuard
{
  public:
    DbusDelayerStateGuard(DbusDelayer* dbusDelayer, const std::string& repr) :
        _dbusDelayer(dbusDelayer), _repr(repr)
    {}

    ~DbusDelayerStateGuard();

  private:
    DbusDelayer* _dbusDelayer;
    const std::string& _repr;
};

class DelayedMethod
{

  public:
    DelayedMethod(DbusDelayer* dbusDelayer, sdbusplus::bus::bus& bus,
                  const std::string& service, const std::string& object,
                  const std::string& interface, const std::string& method) :
        _dbusDelayer(dbusDelayer),
        _repr(service + " " + object + " " + interface + " " + method),
        _bus(bus),
        _method(bus.new_method_call(service.c_str(), object.c_str(),
                                    interface.c_str(), method.c_str()))
    {
        if (dbusDelayer == nullptr)
        {
            throw std::runtime_error("'dbusDelayer' argument must be non-null");
        }
    }

    DelayedMethod(sdbusplus::bus::bus& bus, const std::string& service,
                  const std::string& object, const std::string& interface,
                  const std::string& method) :
        _dbusDelayer(&defaultDbusDelayer),
        _repr(service + " " + object + " " + interface + " " + method),
        _bus(bus),
        _method(bus.new_method_call(service.c_str(), object.c_str(),
                                    interface.c_str(), method.c_str()))
    {}

    template <typename T>
    void append(const T& arg)
    {
        _method.append(arg);
    }

    sdbusplus::message::message
        call(std::optional<sdbusplus::SdBusDuration> timeout = std::nullopt);

  private:
    DbusDelayer* _dbusDelayer;
    std::string _repr;
    sdbusplus::bus::bus& _bus;
    sdbusplus::message::message _method;
};

} // namespace dbus
