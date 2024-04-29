
/**
 *
 */

#pragma once

#include "common.hpp"
#include "data_accessor.hpp"
#include "event_info.hpp"
#include "object.hpp"
#include "util.hpp"

#include <memory>
#include <string>
#include <vector>

namespace event_handler
{

/**
 * @brief Base class for all event handlers.
 *
 */
class EventHandler : public object::Object
{
  public:
    EventHandler(const std::string& name = __PRETTY_FUNCTION__) :
        object::Object(name)
    {}

  public:
    virtual eventing::RcCode process(event_info::EventNode&) = 0;
};

/**
 * @brief A class for clear event handler.
 *
 */
class ClearEvent : public EventHandler
{
  public:
    ClearEvent(const std::string& name = __PRETTY_FUNCTION__) :
        EventHandler(name)
    {}
    ~ClearEvent() = default;

  public:
    /**
     * @brief Clear event via its accessor.
     *
     * @param event
     * @return eventing::RcCode
     */
    eventing::RcCode process([[maybe_unused]] event_info::EventNode& event) override
    {

        if (event.accessor.isTypeDeviceCoreApi())
        {
            std::string property = event.accessor.getProperty();
            if (!property.empty())
            {
                int deviceId = util::getMappedDeviceId(event.event);
                if (deviceId == util::InvalidDeviceId)
                {
                    // not all devices are mapped, ten use common getDeviceId()
                    deviceId = util::getDeviceId(event.event);
                }
                log_dbg("Clear API Debug property(%s) of deviceId(%d)!\n",
                        property.c_str(), deviceId);
                int rc = dbus::deviceClearCoreAPI(deviceId, property);
                if (rc != 0)
                {
                    log_err(
                        "Clear API Error on property(%s) of deviceId(%d), rc=%d!\n",
                        property.c_str(), deviceId, rc);
                    return eventing::RcCode::error;
                }
            }
        }

        return eventing::RcCode::succ;
    }
};

/**
 * @brief A class for managing all event handlers.
 *
 */
class EventHandlerManager : public object::Object
{
  public:
    EventHandlerManager(const std::string& name = __PRETTY_FUNCTION__) :
        object::Object(name)
    {}
    ~EventHandlerManager() = default;

  public:
    /**
     * @brief Register event handlers.
     *
     * @param hdlr
     */
    void RegisterHandler(EventHandler* hdlr)
    {
        _handlers.emplace_back(hdlr);
        log_dbg("handlers(%s) registered.\n", hdlr->getName().c_str());
    }

    /**
     * @brief Run a particular handler by name.
     *
     * @param event
     * @return eventing::RcCode
     */
    eventing::RcCode RunHandler(event_info::EventNode& event,
                           const std::string& name)
    {
        for (auto& hdlr : _handlers)
        {
            if (name == hdlr->getName())
            {
                log_dbg("running handler(%s) on event(%s)\n",
                        hdlr->getName().c_str(), event.getName().c_str());
                auto rc = hdlr->process(event);
                if (rc != eventing::RcCode::succ)
                {
                    log_err("handler(%s) on event(%s) failed, rc = %d!\n",
                            hdlr->getName().c_str(), event.getName().c_str(),
                            eventing::to_integer(rc));
                }
                return eventing::RcCode::succ;
            }
        }
        return eventing::RcCode::succ;
    }

    /**
     * @brief Run all registered event handlers in order.
     *
     * @param event
     * @return eventing::RcCode
     */
    eventing::RcCode RunAllHandlers(event_info::EventNode& event)
    {
        for (auto& hdlr : _handlers)
        {
            log_dbg("running handler(%s) on event(%s)\n",
                    hdlr->getName().c_str(), event.getName().c_str());

            auto rc = hdlr->process(event);

            if (rc != eventing::RcCode::succ)
            {
                log_err("handler(%s) on event(%s) failed, rc = %d!\n",
                        hdlr->getName().c_str(), event.getName().c_str(),
                        eventing::to_integer(rc));
                // let it generate the Event even there is a failure
                // TODO: add some handling use case when it should stop/continue
            }
        }
        return eventing::RcCode::succ;
    }

  private:
    /**
     * @brief hold all event handlers.
     *
     */
    std::vector<EventHandler*> _handlers;
};

} // namespace event_handler
