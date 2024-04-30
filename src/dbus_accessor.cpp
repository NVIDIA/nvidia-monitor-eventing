/**
 * Copyright (c) 2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#include "dbus_accessor.hpp"

#include "log.hpp"
#include "util.hpp"

#include <boost/algorithm/string.hpp>
#include <phosphor-logging/elog.hpp>
#include <sdbusplus/exception.hpp>

namespace dbus
{

DbusDelayerConstLowerBound defaultDbusDelayer;

using namespace phosphor::logging;

static std::string errorMsg(const std::string& description,
                            const std::string& objpath,
                            const std::string& interface,
                            const std::string& property = std::string{""},
                            const char* eWhat = nullptr)
{
    std::string msg{};
    msg += description;
    if (objpath.empty() == false)
    {
        msg += " Objectpath=" + objpath;
    }
    if (interface.empty() == false)
    {
        msg += " Interface=" + interface;
    }
    if (property.empty() == false)
    {
        msg += " Property=" + property;
    }
    if (eWhat != nullptr)
    {
        msg += " Error: ";
        msg += eWhat;
    }
    return msg;
}

std::string getService(const std::string& objectPath,
                       const std::string& interface)
{
    log_elapsed();
    constexpr auto mapperBusBame = "xyz.openbmc_project.ObjectMapper";
    constexpr auto mapperObjectPath = "/xyz/openbmc_project/object_mapper";
    constexpr auto mapperInterface = "xyz.openbmc_project.ObjectMapper";

    /**
     * it looks like ObjectMapper does not know GpioStatusHandler service
     */
#if 1
    constexpr auto gpioStatusService = "xyz.openbmc_project.GpioStatusHandler";
    if (objectPath.find("GpioStatusHandler") != std::string::npos)
    {
        return gpioStatusService;
    }
#endif

    std::string ret{""};
    std::vector<std::pair<std::string, std::vector<std::string>>> response;
    auto bus = sdbusplus::bus::new_default();
    try
    {
        DelayedMethod method(&defaultDbusDelayer, bus, mapperBusBame,
                             mapperObjectPath, mapperInterface, "GetObject");
        method.append(std::string(objectPath));
        method.append(std::vector<std::string>({interface}));
        auto reply = method.call();

        reply.read(response);
        if (response.empty() == false)
        {
            ret = response.begin()->first;
        }
        else
        {
            std::string tmp = errorMsg("getService(): Service not found for",
                                       objectPath, interface);
            logs_err("%s\n", tmp.c_str());
        }
    }
    catch (const sdbusplus::exception::exception& e)
    {
        std::string tmp = errorMsg("getService(): DBus error for", objectPath,
                                   interface, e.what());
        logs_err("%s\n", tmp.c_str());
    }
    return ret;
}

RetCoreApi deviceGetCoreAPI(const int devId, const std::string& property)
{
    log_elapsed();
    constexpr auto service = "xyz.openbmc_project.GpuMgr";
    constexpr auto object = "/xyz/openbmc_project/GpuMgr";
    constexpr auto interface = "xyz.openbmc_project.GpuMgr.Server";
    constexpr auto callName = "DeviceGetData";
    constexpr auto accMode = 1; // Calling in Passthrough Mode. Blocked call.

    uint64_t value = 0;
    std::string valueStr = "";
    std::tuple<int, std::string, std::vector<uint32_t>> response;
    auto bus = sdbusplus::bus::new_default();
    try
    {
        DelayedMethod method(&defaultDbusDelayer, bus, service, object,
                             interface, callName);
        method.append(devId);
        method.append(property);
        method.append(accMode);
        auto reply = method.call();
        reply.read(response);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        std::string tmp =
            errorMsg("deviceGetCoreAPI(): DBus error for", std::string{""},
                     std::string{""}, property, e.what());
        logs_err("%s\n", tmp.c_str());

        return std::make_tuple(-1, valueStr, value);
    }

    // response example:
    // (isau) 0 "Baseboard GPU over temperature info : 0001" 2 1 0
    auto rc = std::get<int>(response);

    if (rc != 0)
    {
        std::string tmp = errorMsg("deviceGetCoreAPI(): bad return for",
                                   std::string{""}, std::string{""}, property);
        logs_err("%s rc=%ld\n", tmp.c_str(), rc);
    }
    else
    {
        auto data = std::get<std::vector<uint32_t>>(response);
        if (data.size() >= 2)
        {
            // Per SMBPBI spec: data[0]:dataOut, data[1]:exDataOut
            value = ((uint64_t)data[1] << 32 | data[0]);
        }

        // msg example: "Baseboard GPU over temperature info : 0001"
        valueStr = std::get<std::string>(response);
    }
    logs_dbg("devId: %d property: %s; rc=%ld value=%llu string='%s'\n", devId,
             property.c_str(), rc, value, valueStr.c_str());

    return std::make_tuple(rc, valueStr, value);
}

int deviceClearCoreAPI(const int devId, const std::string& property)
{
    log_elapsed();
    constexpr auto service = "xyz.openbmc_project.GpuMgr";
    constexpr auto object = "/xyz/openbmc_project/GpuMgr";
    constexpr auto interface = "xyz.openbmc_project.GpuMgr.Server";
    constexpr auto callName = "DeviceClearData";

    int rc = -1;
    std::string dbusError{""};
    auto bus = sdbusplus::bus::new_default();
    try
    {
        DelayedMethod method(&defaultDbusDelayer, bus, service, object,
                             interface, callName);
        method.append(devId);
        method.append(property);
        auto reply = method.call();
        reply.read(rc);
    }
    catch (const sdbusplus::exception::SdBusError& error)
    {
        if (rc == 0)
        {
            rc = -1; // just in case reply.read() failed but set rc = 0
        }
        dbusError = " DBus failed, ";
        dbusError += error.what();
    }

    if (rc != 0)
    {
        std::string msg{"deviceClearCoreAPI() Failed "};
        msg += "devId:" + std::to_string(devId);
        std::string tmp = errorMsg(msg, std::string{""}, std::string{""},
                                   property, dbusError.c_str());
        logs_err("%s\n", tmp.c_str());
    }
    logs_dbg("rc=%d property='%s' devId=%d\n", rc, property.c_str(), devId);
    return rc;
}

PropertyVariant readDbusProperty(const std::string& objPath,
                                 const std::string& interface,
                                 const std::string& property)
{
    log_elapsed();
    PropertyVariant value;
    if (util::existsRange(objPath) == true)
    {
        std::string tmp = errorMsg("readDbusProperty(): PATH with range",
                                   objPath, interface, property);
        logs_err("%s\n", tmp.c_str());
        return value;
    }

    auto service = getService(objPath, interface);
    if (service.empty() == true)
    {
        // getService() already printed error message
        return value;
    }
    auto bus = sdbusplus::bus::new_default();
    try
    {
        DelayedMethod method(&defaultDbusDelayer, bus, service.c_str(),
                             objPath.c_str(), freeDesktopInterface, getCall);
        method.append(interface);
        method.append(property);
        auto reply = method.call();

        reply.read(value);
        auto valueStr = data_accessor::PropertyValue(value).getString();
        logs_dbg("object=%s \n\tinterface=%s property=%s value=%s\n",
                 objPath.c_str(), interface.c_str(), property.c_str(),
                 valueStr.c_str());
    }
    catch (const sdbusplus::exception::exception& e)
    {
        std::string tmp = errorMsg("readDbusProperty() Failed to get property",
                                   objPath, interface, property, e.what());
        logs_err("%s\n", tmp.c_str());
    }
    return value;
}

DbusPropertyChangedHandler registerServicePropertyChanged(
    DbusAsioConnection conn, const std::string& objectPath,
    const std::string& interface, CallbackFunction callback)
{
    return registerServicePropertyChanged(
        static_cast<sdbusplus::bus::bus&>(*conn), objectPath, interface,
        callback);
}

DbusPropertyChangedHandler registerServicePropertyChanged(
    sdbusplus::bus::bus& bus, const std::string& objectPath,
    const std::string& interface, CallbackFunction callback)
{
    log_elapsed();
    DbusPropertyChangedHandler propertyHandler;
    try
    {
        auto subscribeStr = sdbusplus::bus::match::rules::propertiesChanged(
            objectPath, interface);
        logs_dbg("subscribeStr: %s\n", subscribeStr.c_str());
        propertyHandler = std::make_unique<sdbusplus::bus::match_t>(
            bus, subscribeStr, callback);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        std::string tmp = errorMsg("registerServicePropertyChanged(): Error",
                                   objectPath, interface, "", e.what());
        logs_err("%s\n", tmp.c_str());
        throw std::runtime_error(e.what());
    }
    return propertyHandler;
}

bool setDbusProperty(const std::string& objPath, const std::string& interface,
                     const std::string& property, const PropertyVariant& val)
{
    return setDbusProperty(getService(objPath, interface), objPath, interface,
                           property, val);
}

bool setDbusProperty(const std::string& service, const std::string& objPath,
                     const std::string& interface, const std::string& property,
                     const PropertyVariant& val)
{
    log_elapsed();
    auto bus = sdbusplus::bus::new_default();
    bool ret = false;
    try
    {
        DelayedMethod method(&defaultDbusDelayer, bus, service.c_str(),
                             objPath.c_str(), freeDesktopInterface, setCall);
        method.append(interface);
        method.append(property);
        method.append(val);

        ret = true;
        if (!method.call())
        {
            ret = false;
        }
    }
    catch (const sdbusplus::exception::exception& e)
    {
        std::string tmp = errorMsg("setDbusProperty() Failed to set property",
                                   objPath, interface, property, e.what());
        logs_err("%s\n", tmp.c_str());
    }
    return ret;
}

// CachingObjectMapper ////////////////////////////////////////////////////////

CachingObjectMapper::ValueType CachingObjectMapper::getObjectImpl(
    [[maybe_unused]] sdbusplus::bus::bus& bus, const std::string& objectPath,
    const std::vector<std::string>& interfaces)
{
    ensureIsInitialized();
    return scopeManagers(objectsServicesMapping.at(objectPath), interfaces);
}

std::vector<std::string> CachingObjectMapper::getSubTreePathsImpl(
    [[maybe_unused]] sdbusplus::bus::bus& bus, const std::string& subtree,
    int depth, const std::vector<std::string>& interfaces)
{
    ensureIsInitialized();
    if (depth == 0 && subtree == "/")
    {
        return getSubTreePathsImpl(bus, interfaces);
    }
    else
    {
        throw std::runtime_error(
            "CachingObjectMapper::getSubTreePathsImpl not implemented"
            "for depth != and subtree != '/'");
    }
}

std::vector<std::string> CachingObjectMapper::getSubTreePathsImpl(
    [[maybe_unused]] sdbusplus::bus::bus& bus,
    const std::vector<std::string>& interfaces)
{
    ensureIsInitialized();
    std::vector<std::string> result;
    for (const auto& keyValPair : objectsServicesMapping)
    {
        if (scopeManagers(keyValPair.second, interfaces).size() != 0)
        {
            result.push_back(keyValPair.first);
        }
    }
    return result;
}

void CachingObjectMapper::refresh()
{
    this->objectsServicesMapping = DirectObjectMapper().getSubtreeImpl(
        this->bus, "/", 0, std::vector<std::string>({}));
    this->isInitialized = true;
}

CachingObjectMapper::ValueType CachingObjectMapper::scopeManagers(
    const ValueType& implementations,
    const std::vector<std::string>& interfaces)
{
    CachingObjectMapper::ValueType result;
    for (const auto& keyValPair : implementations)
    {
        const auto& intfs = keyValPair.second;
        bool implementsAny = true;
        // Yes, the original xyz.openbmc_project.ObjectMapper's logic of the
        // object paths scoping based on a list of interfaces is whether a
        // particular path implements any of them, not all
        for (std::vector<std::string>::const_iterator it = interfaces.cbegin();
             it != interfaces.cend() && implementsAny; ++it)
        {
            implementsAny =
                std::find(intfs.cbegin(), intfs.cend(), *it) != intfs.cend();
        }
        if (implementsAny)
        {
            result[keyValPair.first] = intfs;
        }
    }
    return result;
}

void CachingObjectMapper::ensureIsInitialized()
{
    if (!this->isInitialized)
    {
        this->refresh();
    }
}

// DbusDelayer ////////////////////////////////////////////////////////////////

const char* DbusDelayer::stateToStr(State state)
{
    static std::map<State, const char*> stateToNameMap(
        {{State::idle, "idle"},
         {State::waiting, "waiting"},
         {State::calling, "calling"}});
    return stateToNameMap.at(state);
}

std::string timeToStringMsec(
    const std::chrono::time_point<std::chrono::steady_clock>& time)
{
    std::stringstream ss;
    ss << std::chrono::duration_cast<std::chrono::milliseconds>(
              time.time_since_epoch())
              .count();
    return ss.str();
}

std::chrono::milliseconds
    DbusDelayer::callStartAttempt(const std::string& signature)
{
    if (this->state == State::idle)
    {
        auto now = std::chrono::steady_clock::now();
        std::chrono::milliseconds waitTime =
            this->callStartAttemptImpl(signature, now);
        this->state = State::waiting;
        return waitTime;
    }
    else // ! this->state == State::idle
    {
        log_dbg("Function called on object in an incorrect state (%s). "
                "Expected state: %s. Ignoring the call.\n",
                stateToStr(this->state), stateToStr(State::idle));
        return std::chrono::milliseconds(0);
    }
}

void DbusDelayer::callStartActual(const std::string& signature)
{
    if (this->state == State::waiting)
    {
        auto now = std::chrono::steady_clock::now();
        this->callStartActualImpl(signature, now);
        this->state = State::calling;
    }
    else // ! this->state == State::waiting
    {
        log_dbg("Function called on object in an incorrect state (%s). "
                "Expected state: %s. Ignoring the call.\n",
                stateToStr(this->state), stateToStr(State::waiting));
    }
}

void DbusDelayer::callFinished(const std::string& signature)
{
    if (this->state == State::calling)
    {
        auto now = std::chrono::steady_clock::now();
        this->callFinishedImpl(signature, now);
        this->state = State::idle;
    }
    else // ! this->state == State::calling
    {
        log_dbg("Function called on object in an incorrect state (%s). "
                "Expected state: %s. Ignoring the call.\n",
                stateToStr(this->state), stateToStr(State::calling));
    }
}

std::chrono::milliseconds DbusDelayer::callStartAttemptImpl(
    [[maybe_unused]] const std::string& signature,
    [[maybe_unused]] const std::chrono::time_point<std::chrono::steady_clock>&
        now)
{
    return std::chrono::milliseconds(0);
}

void DbusDelayer::callStartActualImpl(
    [[maybe_unused]] const std::string& signature,
    [[maybe_unused]] const std::chrono::time_point<std::chrono::steady_clock>&
        now)
{}

void DbusDelayer::callFinishedImpl(
    [[maybe_unused]] const std::string& signature,
    [[maybe_unused]] const std::chrono::time_point<std::chrono::steady_clock>&
        now)
{}

// DbusDelayerConstLowerBound /////////////////////////////////////////////////

void DbusDelayerConstLowerBound::setDelayTime(
    const std::chrono::milliseconds& waitTimeLowerBound)
{
    _waitTimeLowerBound = waitTimeLowerBound;
}

void DbusDelayerConstLowerBound::callFinishedImpl(
    [[maybe_unused]] const std::string& signature,
    const std::chrono::time_point<std::chrono::steady_clock>& now)
{
    _lastCallFinish = now;
}

std::chrono::milliseconds DbusDelayerConstLowerBound::callStartAttemptImpl(
    [[maybe_unused]] const std::string& signature,
    [[maybe_unused]] const std::chrono::time_point<std::chrono::steady_clock>&
        now)
{
    return std::max(_waitTimeLowerBound -
                        std::chrono::duration_cast<std::chrono::milliseconds>(
                            now - _lastCallFinish),
                    std::chrono::milliseconds(0));
}

// DbusDelayerStateGuard //////////////////////////////////////////////////////

DbusDelayerStateGuard::~DbusDelayerStateGuard()
{
    if (_dbusDelayer->getState() == DbusDelayer::State::waiting)
    {
        _dbusDelayer->callStartActual(_repr);
    }
    if (_dbusDelayer->getState() == DbusDelayer::State::calling)
    {
        _dbusDelayer->callFinished(_repr);
    }
}

// DirectObjectMapper /////////////////////////////////////////////////////////

DirectObjectMapper::ValueType DirectObjectMapper::getObjectImpl(
    sdbusplus::bus::bus& bus, const std::string& objectPath,
    const std::vector<std::string>& interfaces) const
{
    ValueType result;
    DelayedMethod method(bus, "xyz.openbmc_project.ObjectMapper",
                         "/xyz/openbmc_project/object_mapper",
                         "xyz.openbmc_project.ObjectMapper", "GetObject");
    method.append(objectPath);
    method.append(interfaces);
    auto reply = method.call();
    reply.read(result);
    return result;
}

std::vector<std::string> DirectObjectMapper::getSubTreePathsImpl(
    sdbusplus::bus::bus& bus, const std::string& subtree, int depth,
    const std::vector<std::string>& interfaces) const
{

    std::vector<std::string> result;
    DelayedMethod method(bus, "xyz.openbmc_project.ObjectMapper",
                         "/xyz/openbmc_project/object_mapper",
                         "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths");
    method.append(subtree);
    method.append(depth);
    method.append(interfaces);
    auto reply = method.call();
    reply.read(result);
    return result;
}

DirectObjectMapper::FullTreeType DirectObjectMapper::getSubtreeImpl(
    sdbusplus::bus::bus& bus, const std::string& subtree, int depth,
    const std::vector<std::string>& interfaces) const
{
    FullTreeType result;
    DelayedMethod method(bus, "xyz.openbmc_project.ObjectMapper",
                         "/xyz/openbmc_project/object_mapper",
                         "xyz.openbmc_project.ObjectMapper", "GetSubTree");
    method.append(subtree);
    method.append(depth);
    method.append(interfaces);
    auto reply = method.call();
    reply.read(result);
    return result;
}

// DelayedMethod //////////////////////////////////////////////////////////////

sdbusplus::message::message
    DelayedMethod::call(std::optional<sdbusplus::SdBusDuration> timeout)
{
    const std::lock_guard<std::mutex> lock(_dbusDelayer->mutex);
    // 'ddsg' makes sure that '_dbusDelayer' is always left in
    // the 'State::idle' state after leving this function, even
    // if exception occurs in dbus read
    DbusDelayerStateGuard ddsg(_dbusDelayer, _repr);
    if (_dbusDelayer->getState() == DbusDelayer::State::idle)
    {
        auto waitTime = _dbusDelayer->callStartAttempt(_repr);
        {
            std::stringstream ss;
            ss << "Delayed for " << waitTime.count() << " ms dbus call '"
               << _repr << "'" << std::endl;
            log_dbg("%s", ss.str().c_str());
        }
        std::this_thread::sleep_for(waitTime);
        _dbusDelayer->callStartActual(_repr);
        auto reply = _bus.call(_method, timeout);
        _dbusDelayer->callFinished(_repr);
        return reply;
    }
    else // ! _dbusDelayer->getState() == DbusDelayer::State::idle
    {
        log_err("_dbusDelayer object expected to be in '%s' state. "
                "(actual state: '%s'). "
                "Falling back to regular, non-delayed dbus call.\n",
                DbusDelayer::stateToStr(DbusDelayer::State::idle),
                DbusDelayer::stateToStr(_dbusDelayer->getState()));
        return _bus.call(_method, timeout);
    }
}

} // namespace dbus
