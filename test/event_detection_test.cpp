#include "common.hpp"
#include "dat_traverse.hpp"
#include "event_detection.hpp"
#include "message_composer.hpp"
#include "nlohmann/json.hpp"
#include "selftest.hpp"

#include "gmock/gmock.h"

static  void create_event_by_device_type(event_info::EventNode& ev,
                                        const std::string& deviceType)
{
    nlohmann::json j;
    j["event"] = "Event0";
    j["error_id"] = "Event0-Error";
    j["device_type"] = deviceType;
    j["sub_type"] = "";
    j["severity"] = "Critical";
    j["resolution"] = "Contact NVIDIA Support";
    j["redfish"]["message_id"] = "ResourceEvent.1.0.ResourceErrorsDetected";
    j["redfish"]["message_args"]["patterns"] = {"p1", "p2"};
    j["redfish"]["message_args"]["parameters"] = nlohmann::json::array();
    j["telemetries"] = {"t0", "t1"};
    j["trigger_count"] = 0;
    j["event_trigger"]["metadata"] = "metadata";
    j["event_trigger"]["type"] = "DBUS";
    j["action"] = "do something";
    j["event_counter_reset"]["type"] = "type";
    j["event_counter_reset"]["metadata"] = "metadata";
    j["accessor"]["metadata"] = "metadata";
    j["accessor"]["type"] = "DBUS";
    j["value_as_count"] = false;
    ev.loadFrom(j);
}

static bool setup_event_cnt_test(event_info::EventNode& ev,
                                 int tested_trigger_count,
                                 const std::string& device_name)
{
    nlohmann::json j;
    j["event"] = "Event0";
    j["error_id"] = "Event0-Error";
    j["device_type"] = "GPU";
    j["sub_type"] = "";
    j["severity"] = "Critical";
    j["resolution"] = "Contact NVIDIA Support";
    j["redfish"]["message_id"] = "ResourceEvent.1.0.ResourceErrorsDetected";
    j["redfish"]["message_args"]["patterns"] = {"p1", "p2"};
    j["redfish"]["message_args"]["parameters"] = nlohmann::json::array();
    j["telemetries"] = {"t0", "t1"};
    j["trigger_count"] = tested_trigger_count;
    j["event_trigger"]["metadata"] = "metadata";
    j["event_trigger"]["type"] = "DBUS";
    j["action"] = "do something";
    j["event_counter_reset"]["type"] = "type";
    j["event_counter_reset"]["metadata"] = "metadata";
    j["accessor"]["metadata"] = "metadata";
    j["accessor"]["type"] = "DBUS";
    j["value_as_count"] = false;
    ev.loadFrom(j);
    ev.device = device_name;
    // ev.print();

    return true;
}

TEST(EventLookupTest, TriggerAccessor)
{
    event_info::EventNode ev("Sample Event");
    nlohmann::json j;
    j["event"] = "Event0";
    j["error_id"] = "Event0-Error";
    j["device_type"] = "GPU";
    j["sub_type"] = "";
    j["severity"] = "Critical";
    j["resolution"] = "Contact NVIDIA Support";
    j["redfish"]["message_id"] = "ResourceEvent.1.0.ResourceErrorsDetected";
    j["redfish"]["message_args"]["patterns"] = {"p1", "p2"};
    j["redfish"]["message_args"]["parameters"] = nlohmann::json::array();
    j["telemetries"] = {"t0", "t1"};
    j["trigger_count"] = 0;
    j["event_trigger"]["property"] = "property";
    j["event_trigger"]["type"] = "DBUS";
    j["action"] = "do something";
    j["event_counter_reset"]["type"] = "type";
    j["event_counter_reset"]["metadata"] = "metadata";
    j["accessor"]["property"] = "property";
    j["accessor"]["type"] = "DBUS";
    j["value_as_count"] = false;
    ev.loadFrom(j);

    event_info::EventNode ev2("Sample Event2");
    nlohmann::json j2;
    j2["event"] = "Event0";
    j2["error_id"] = "Event0-Error";
    j2["device_type"] = "GPU";
    j2["sub_type"] = "";
    j2["severity"] = "Critical";
    j2["resolution"] = "Contact NVIDIA Support";
    j2["redfish"]["message_id"] = "ResourceEvent.1.0.ResourceErrorsDetected";
    j2["redfish"]["message_args"]["patterns"] = {"p1", "p2"};
    j2["redfish"]["message_args"]["parameters"] = nlohmann::json::array();
    j2["telemetries"] = {"t0", "t1"};
    j2["trigger_count"] = 0;
    j2["event_trigger"] = nlohmann::json::object();
    j2["action"] = "do something";
    j2["event_counter_reset"]["type"] = "type";
    j2["event_counter_reset"]["metadata"] = "metadata";
    j2["accessor"]["property"] = "property2";
    j2["accessor"]["type"] = "DBUS";
    j2["value_as_count"] = false;
    ev2.loadFrom(j2);

    std::vector<event_info::EventNode> v;
    v.push_back(ev);
    v.push_back(ev2);

    event_info::EventMap eventMap;

    eventMap.insert(
        std::pair<std::string, std::vector<event_info::EventNode>>("GPU", v));

    event_info::PropertyFilterSet propertyFilterSet;

    event_handler::EventHandlerManager* eventHdlrMgr = nullptr;

    event_detection::EventDetection eventDetection(
        "EventDetection2", &eventMap, &propertyFilterSet, eventHdlrMgr);

    ev.device = "GPU";
    EXPECT_EQ(eventDetection.IsEvent(ev, ev.device), true);
    EXPECT_EQ(ev.count[ev.device], 0);

    ev.valueAsCount = true;
    EXPECT_EQ(eventDetection.IsEvent(ev, ev.device, 10), true);

    ev.triggerCount = 20;
    EXPECT_EQ(eventDetection.IsEvent(ev, ev.device, 10), false);

    ev.count[ev.device] = 10;
    ev.valueAsCount = false;
    EXPECT_EQ(eventDetection.IsEvent(ev, ev.device), false);
    EXPECT_EQ(ev.count[ev.device], 11);
    ev.triggerCount = 0;

    nlohmann::json j3;
    j3["property"] = "property2";
    j3["type"] = "DBUS";
    data_accessor::DataAccessor accessor(j3);

    nlohmann::json j4;
    j4["property"] = "property";
    j4["type"] = "DBUS";
    data_accessor::DataAccessor accessorTrigger(j4);

    nlohmann::json j5;
    j5["property"] = "prop";
    j5["type"] = "DBUS";
    data_accessor::DataAccessor accessorNil(j5);
    std::vector<std::shared_ptr<event_info::EventNode>> eventPtrs;

    EXPECT_EQ(eventDetection.EventsDetection(accessor, eventPtrs).empty(), true);
    EXPECT_EQ(eventDetection.EventsDetection(accessorTrigger, eventPtrs).empty(),
              true);
    EXPECT_EQ(eventDetection.EventsDetection(accessorNil, eventPtrs).empty(), true);
}

TEST(EventDetectionTest, TID_1_triggers_instantly)
{
    event_info::EventMap* eventMap = nullptr;
    event_info::PropertyFilterSet* propertyFilterSet = nullptr;
    event_handler::EventHandlerManager* eventHdlrMgr = nullptr;
    event_detection::EventDetection eventDetection(
        "EventDetection1", eventMap, propertyFilterSet, eventHdlrMgr);
    event_info::EventNode event("TID_1_triggers_instantly");
    int trigger_count = 0;
    EXPECT_EQ(setup_event_cnt_test(event, trigger_count, "GPU5"), true);

    EXPECT_EQ(eventDetection.IsEvent(event, event.device), true);
}

TEST(EventDetectionTest, TID_2_triggers_instantly)
{
    event_info::EventMap* eventMap = nullptr;
    event_info::PropertyFilterSet* propertyFilterSet = nullptr;
    event_handler::EventHandlerManager* eventHdlrMgr = nullptr;
    event_detection::EventDetection eventDetection(
        "EventDetection1", eventMap, propertyFilterSet, eventHdlrMgr);
    event_info::EventNode event("TID_2_triggers_instantly");
    int trigger_count = 1;
    EXPECT_EQ(setup_event_cnt_test(event, trigger_count, "GPU5"), true);

    EXPECT_EQ(eventDetection.IsEvent(event, event.device), true);
}

TEST(EventDetectionTest, TID_3_triggers_on_trigger_count)
{
    event_info::EventMap* eventMap = nullptr;
    event_info::PropertyFilterSet* propertyFilterSet = nullptr;
    event_handler::EventHandlerManager* eventHdlrMgr = nullptr;
    event_detection::EventDetection eventDetection(
        "EventDetection1", eventMap, propertyFilterSet, eventHdlrMgr);
    event_info::EventNode event("TID_3_triggers_on_trigger_count");
    int trigger_count = 5;
    EXPECT_EQ(setup_event_cnt_test(event, trigger_count, "GPU5"), true);

    for (auto i = 1; i < trigger_count; i++)
    {
        EXPECT_EQ(eventDetection.IsEvent(event, event.device), false);
    }

    EXPECT_EQ(eventDetection.IsEvent(event, event.device), true);
    EXPECT_EQ(eventDetection.IsEvent(event, event.device), true);
    EXPECT_EQ(eventDetection.IsEvent(event, event.device), true);
}

TEST(EventDetectionTest, TID_4_counter_separation)
{
    event_info::EventMap* eventMap = nullptr;
    event_info::PropertyFilterSet* propertyFilterSet = nullptr;
    event_handler::EventHandlerManager* eventHdlrMgr = nullptr;
    event_detection::EventDetection eventDetection(
        "EventDetection1", eventMap, propertyFilterSet, eventHdlrMgr);
    event_info::EventNode ev1("TID_4_counter_separation");
    int ev1_trig_cnt = 1;
    EXPECT_EQ(setup_event_cnt_test(ev1, ev1_trig_cnt, "GPU1"), true);

    event_info::EventNode ev2("test event2");
    int ev2_trig_cnt = 3;
    EXPECT_EQ(setup_event_cnt_test(ev2, ev2_trig_cnt, "GPU2"), true);

    event_info::EventNode ev3("test event3");
    int ev3_trig_cnt = 9;
    EXPECT_EQ(setup_event_cnt_test(ev3, ev3_trig_cnt, "GPU3"), true);

    for (auto i = 1; i < 10; i++)
    {
        /* decrement counter and return false to ternary operator */
        EXPECT_EQ(eventDetection.IsEvent(ev1, ev1.device),
                  (ev1_trig_cnt <= 1) ? (true) : (ev1_trig_cnt--, false));
        EXPECT_EQ(eventDetection.IsEvent(ev2, ev2.device),
                  (ev2_trig_cnt <= 1) ? (true) : (ev2_trig_cnt--, false));
        EXPECT_EQ(eventDetection.IsEvent(ev3, ev3.device),
                  (ev3_trig_cnt <= 1) ? (true) : (ev3_trig_cnt--, false));
    }
}

TEST(EventDetectionTest, PropertyFilterMap)
{
    nlohmann::json j;
    j["event"] = "Event0";
    j["error_id"] = "Event0-Error";
    j["device_type"] = "GPU";
    j["sub_type"] = "";
    j["severity"] = "Critical";
    j["resolution"] = "Contact NVIDIA Support";
    j["redfish"]["message_id"] = "ResourceEvent.1.0.ResourceErrorsDetected";
    j["redfish"]["message_args"]["patterns"] = {"p1", "p2"};
    j["redfish"]["message_args"]["parameters"] = nlohmann::json::array();
    j["telemetries"] = nlohmann::json::array();
    j["trigger_count"] = 0;
    j["event_trigger"] = "trigger";
    j["action"] = "do something";
    j["event_counter_reset"]["type"] = "type";
    j["event_counter_reset"]["metadata"] = "metadata";
    j["accessor"]["type"] = "DBUS";
    j["accessor"]["object"] =
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_[1-8]/Ports/NVLink_[0-17]";
    j["accessor"]["interface"] = "xyz.openbmc_project.Inventory.Item.Port";
    j["accessor"]["property"] = "RecoveryCount";
    j["value_as_count"] = false;
    event_info::EventNode event("test event");
    event.loadFrom(j);
    event_info::PropertyFilterSet propertyFilterSet;
    event_info::addEventToPropertyFilterSet(event, propertyFilterSet);

    event_info::EventMap* eventMap = nullptr;
    event_handler::EventHandlerManager* eventHdlrMgr = nullptr;
    event_detection::EventDetection eventDetection(
        "EventDetection1", eventMap, &propertyFilterSet, eventHdlrMgr);

    {
        nlohmann::json match_both;
        match_both["type"] = "DBUS";
        match_both["object"] =
            "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_1/Ports/NVLink_7";
        match_both["interface"] = "xyz.openbmc_project.Inventory.Item.Port";
        match_both["property"] = "RecoveryCount";
        data_accessor::DataAccessor acc_match_both(match_both);
        // EXPECT_TRUE(event_info::EventNode::getIsAccessorInteresting(event,
        // acc_match_both));
        EXPECT_TRUE(eventDetection.getIsAccessorInteresting(acc_match_both));
    }

    {
        nlohmann::json match_both_2;
        match_both_2["type"] = "DBUS";
        match_both_2["object"] =
            "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_6/Ports/NVLink_2";
        match_both_2["interface"] = "xyz.openbmc_project.Inventory.Item.Port";
        match_both_2["property"] = "RecoveryCount";
        data_accessor::DataAccessor acc_match_both_2(match_both_2);
        // EXPECT_TRUE(event_info::EventNode::getIsAccessorInteresting(event,
        // acc_match_both_2));
        EXPECT_TRUE(eventDetection.getIsAccessorInteresting(acc_match_both_2));
    }

    {
        nlohmann::json match_first;
        match_first["type"] = "DBUS";
        match_first["object"] =
            "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_1/Ports/SomethingElse";
        match_first["interface"] = "xyz.openbmc_project.Inventory.Item.Port";
        match_first["property"] = "RecoveryCount";
        data_accessor::DataAccessor acc_match_first(match_first);
        // EXPECT_FALSE(event_info::EventNode::getIsAccessorInteresting(event,
        // acc_match_first));
        EXPECT_FALSE(eventDetection.getIsAccessorInteresting(acc_match_first));
    }

    {
        nlohmann::json match_second;
        match_second["type"] = "DBUS";
        match_second["object"] =
            "/xyz/openbmc_project/inventory/system/processors/NVSwitch_0/Ports/NVLink_7";
        match_second["interface"] = "xyz.openbmc_project.Inventory.Item.Port";
        match_second["property"] = "RecoveryCount";
        data_accessor::DataAccessor acc_match_second(match_second);
        // EXPECT_FALSE(event_info::EventNode::getIsAccessorInteresting(event,
        // acc_match_second));
        EXPECT_FALSE(eventDetection.getIsAccessorInteresting(acc_match_second));
    }

    {
        nlohmann::json match_neither;
        match_neither["type"] = "DBUS";
        match_neither["object"] =
            "/xyz/openbmc_project/inventory/system/processors/NVSwitch_0/Ports/Something";
        match_neither["interface"] = "xyz.openbmc_project.Inventory.Item.Port";
        match_neither["property"] = "RecoveryCount";
        data_accessor::DataAccessor acc_match_neither(match_neither);
        // EXPECT_FALSE(event_info::EventNode::getIsAccessorInteresting(event,
        // acc_match_neither));
        EXPECT_FALSE(
            eventDetection.getIsAccessorInteresting(acc_match_neither));
    }

    {
        nlohmann::json match_bad_range;
        match_bad_range["type"] = "DBUS";
        match_bad_range["object"] =
            "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_12/Ports/NVLink_80";
        match_bad_range["interface"] =
            "xyz.openbmc_project.Inventory.Item.Port";
        match_bad_range["property"] = "RecoveryCount";
        data_accessor::DataAccessor acc_match_bad_range(match_bad_range);
        // EXPECT_FALSE(event_info::EventNode::getIsAccessorInteresting(event,
        // acc_match_bad_range));
        EXPECT_FALSE(
            eventDetection.getIsAccessorInteresting(acc_match_bad_range));
    }

    {
        nlohmann::json wrong_prefix;
        wrong_prefix["type"] = "DBUS";
        wrong_prefix["object"] =
            "/xyz/openbmc_project/inventory/SOMETHING/ELSE/GPU_SXM_1/Ports/NVLink_7";
        wrong_prefix["interface"] = "xyz.openbmc_project.Inventory.Item.Port";
        wrong_prefix["property"] = "RecoveryCount";
        data_accessor::DataAccessor acc_wrong_prefix(wrong_prefix);
        // EXPECT_FALSE(event_info::EventNode::getIsAccessorInteresting(event,
        // acc_wrong_prefix));
        EXPECT_FALSE(eventDetection.getIsAccessorInteresting(acc_wrong_prefix));
    }

    {
        nlohmann::json wrong_iface;
        wrong_iface["type"] = "DBUS";
        wrong_iface["object"] =
            "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_1/Ports/NVLink_7";
        wrong_iface["interface"] =
            "xyz.openbmc_project.Inventory.Item.Port12345";
        wrong_iface["property"] = "RecoveryCount";
        data_accessor::DataAccessor acc_wrong_iface(wrong_iface);
        // EXPECT_FALSE(event_info::EventNode::getIsAccessorInteresting(event,
        // acc_wrong_iface));
        EXPECT_FALSE(eventDetection.getIsAccessorInteresting(acc_wrong_iface));
    }

    {
        nlohmann::json wrong_prop;
        wrong_prop["type"] = "DBUS";
        wrong_prop["object"] =
            "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_1/Ports/NVLink_7";
        wrong_prop["interface"] = "xyz.openbmc_project.Inventory.Item.Port";
        wrong_prop["property"] = "SomethingElse";
        data_accessor::DataAccessor acc_wrong_prop(wrong_prop);
        // EXPECT_FALSE(event_info::EventNode::getIsAccessorInteresting(event,
        // acc_wrong_prop));
        EXPECT_FALSE(eventDetection.getIsAccessorInteresting(acc_wrong_prop));
    }
}

TEST(EventDetectionTest, PcQueueNormal)
{
    // make a queue and a worker thread, push items one at a time
    // and then push a batch quickly, make sure all items are received
    // also check that before any push and after all pushes
    // popping elements fails correctly when the queue is empty
    PcQueueType queue(100);

    // std::thread worker([&queue]() {
    //     // This runs second
    //     std::this_thread::sleep_for(std::chrono::milliseconds(500));
    //     PcDataType d;
    //     // pop 5 elements
    //     for (int i = 0; i < 5; i++)
    //     {
    //         EXPECT_TRUE(queue.pop(d));
    //     }
    // });
}

TEST(EventDetectionTest, PcQueuePushFull1)
{
    // make a queue and a worker thread, fill the queue
    // then try to push elements and ensure it fails
    // then take items off the queue and ensure correct
    // number of pushes succeed and available space is correct
    PcQueueType queue(100);
    std::mutex mut;
    mut.lock();
    std::thread worker([&queue, &mut]() {
        std::scoped_lock lock(mut); // this will block until pushes are complete
        // This runs second
        PcDataType d;
        // try to get the 100 elements back
        for (int i = 0; i < 100; i++)
        {
            EXPECT_TRUE(queue.pop(d));
        }
        // now try to get more, it should fail
        for (int i = 0; i < 20; i++)
        {
            EXPECT_FALSE(queue.pop(d));
        }
    });

    // This runs first
    // push 100 (default) elements
    for (int i = 0; i < 100; i++)
    {
        EXPECT_TRUE(queue.push(PcDataType{}));
    }
    // try to push 10 more elements that should fail since the queue is full
    for (int i = 0; i < 10; i++)
    {
        EXPECT_FALSE(queue.push(PcDataType{}));
    }
    mut.unlock(); // let worker thread run
    worker.join();
}

TEST(EventDetectionTest, PcQueuePushFull2)
{
    // make a queue and a worker thread, push items one at a time
    // and then push a batch quickly, make sure all items are received
    // also check that before any push and after all pushes
    // popping elements fails correctly when the queue is empty
    PcQueueType queue(100);

    nlohmann::json j;
    j["type"] = "DBUS";
    j["object"] =
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_1/Ports/NVLink_7";
    j["interface"] = "xyz.openbmc_project.Inventory.Item.Port";
    j["property"] = "RecoveryCount";

    std::mutex mut;
    mut.lock();
    std::thread worker([&queue, &mut]() {
        std::scoped_lock lock(mut); // this will block until pushes are complete
        // This runs second
        PcDataType d;
        // pop 5 elements
        for (int i = 0; i < 5; i++)
        {
            EXPECT_TRUE(queue.pop(d));
            EXPECT_EQ(i, d.accessor.getDataValue().getInteger());
        }
    });

    // This runs first
    // push 100 elements
    std::vector<std::shared_ptr<event_info::EventNode>> eventPtrs;
    for (int i = 0; i < 100; i++)
    {
        data_accessor::PropertyValue pv(std::to_string(i));
        data_accessor::DataAccessor acc(j, pv);
        EXPECT_TRUE(queue.push(PcDataType{acc, eventPtrs}));
    }
    // try to push 10 more elements that should fail since the queue is full
    for (int i = 0; i < 10; i++)
    {
        data_accessor::PropertyValue pv(std::to_string(1000 + i));
        data_accessor::DataAccessor acc(j, pv);
        EXPECT_FALSE(queue.push(PcDataType{acc, eventPtrs}));
    }
    // let the worker thread drain 5 elements
    mut.unlock();
    std::this_thread::sleep_for(std::chrono::seconds(5));
    // we should be able to push 5 more elements, but no more
    for (int i = 0; i < 5; i++)
    {
        EXPECT_TRUE(queue.push(PcDataType{}));
    }
    for (int i = 0; i < 10; i++)
    {
        EXPECT_FALSE(queue.push(PcDataType{}));
    }
    worker.join();
}

TEST(EventDetectionTest, PcQueueConcurrentStress)
{
    // make a queue and a worker thread, push items one at a time
    // and then push a batch quickly, make sure all items are received
    // also check that before any push and after all pushes
    // popping elements fails correctly when the queue is empty
    PcQueueType queue(1000);
    constexpr int NTHREADS = 5;
    constexpr int NELEMENTS = 180;
    std::unique_ptr<std::thread> threads[NTHREADS];

    nlohmann::json j;
    j["type"] = "DBUS";
    j["object"] =
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_1/Ports/NVLink_7";
    j["interface"] = "xyz.openbmc_project.Inventory.Item.Port";
    j["property"] = "RecoveryCount";

    for (int tid = 0; tid < NTHREADS; tid++)
    {
        threads[tid] = std::make_unique<std::thread>([&queue, j, tid]() {
            std::vector<std::shared_ptr<event_info::EventNode>> eventPtrs;
            for (int i = 0; i < NELEMENTS; i++)
            {
                data_accessor::PropertyValue pv(
                    std::to_string(10000 * tid + i));
                data_accessor::DataAccessor acc(j, pv);
                EXPECT_TRUE(queue.push(PcDataType{acc, eventPtrs}));
            }
        });
    }
    for (int tid = 0; tid < NTHREADS; tid++)
    {
        threads[tid]->join();
    }
    std::set<int> expectedSet;
    for (int i = 0; i < NTHREADS; i++)
    {
        for (int j = 0; j < NELEMENTS; j++)
        {
            expectedSet.insert(10000 * i + j);
        }
    }
    PcDataType popped_element;
    for (int i = 0; i < NTHREADS * NELEMENTS; i++)
    {
        EXPECT_TRUE(queue.pop(popped_element));
        int value = popped_element.accessor.getDataValue().getInteger();
        // value should be in the expected set exactly once
        EXPECT_TRUE(expectedSet.contains(value));
        EXPECT_EQ(1, expectedSet.erase(value));
    }
    // the queue and remaining expected set should both be empty
    EXPECT_FALSE(queue.pop(popped_element));
    EXPECT_TRUE(expectedSet.empty());
}

TEST(EventDetectionTest,  CreateEventFromFailedCmdLineSecureBoot)
{
   auto   secureBootEventInfo =
   R"(
       {
            "type": "CMDLINE",
            "executable": "mctp-vdm-util-wrapper",
            "arguments": "active_auth_status GPU_SXM_[1-8]",
            "check": {
                "equal": "6"
            }
        }
   )";

    auto secureBootSelfTest =
    R"(
        {
            "type": "CMDLINE",
            "executable": "mctp-vdm-util-wrapper",
            "arguments": "active_auth_status GPU_SXM_5"
        }
    )";

    // Event Info Accessor
    nlohmann::json eventInfoecureBoot = nlohmann::json::parse(secureBootEventInfo);
    data_accessor::DataAccessor eventInfoSecureBoot(eventInfoecureBoot);

    // SelfTest Accessor
    data_accessor::PropertyValue fail(uint32_t(6));
    nlohmann::json jsonSecureBoot = nlohmann::json::parse(secureBootSelfTest);
    data_accessor::DataAccessor failedSecureBoot(jsonSecureBoot, fail);

    data_accessor::CheckAccessor checkObj("GPU_SXM_[1-8]");
    bool passed = checkObj.check(eventInfoSecureBoot, failedSecureBoot);
    EXPECT_EQ(passed, true);
    auto assertedData = checkObj.getAssertedDevices();
    EXPECT_EQ(assertedData.size(), 1);
    EXPECT_EQ(assertedData.at(0).device, "GPU_SXM_5");
    EXPECT_EQ(assertedData.at(0).deviceIndexTuple, device_id::PatternIndex(5));
}

TEST(EventDetectionTest,  CreateEventFromFailedCoreApiTestPoint)
{
    auto eventInfoGpuOverTemp =
    R"(
      {
        "type": "DeviceCoreAPI",
                 "property": "gpu.thermal.temperature.overTemperatureInfo",
                              "check": {
            "equal": "1"
        }
      }
    )";

    auto selftestGpuOverTemp =
    R"(
       {
          "type": "DeviceCoreAPI",
          "property": "gpu.thermal.temperature.overTemperatureInfo"
       }
    )";

    // failed Accessor from SelfTest
    data_accessor::PropertyValue fail(uint32_t(1));
    nlohmann::json jsonGpuOverTemp = nlohmann::json::parse(selftestGpuOverTemp);
    data_accessor::DataAccessor failedGpuOverTemp(jsonGpuOverTemp);
    // lets it save the device
    std::string inputDevice{"GPU_SXM_4"};
    failedGpuOverTemp.setDevice(inputDevice);
    failedGpuOverTemp.setDataValue(fail); // input the failure data

    // Accessor from Event Info
    nlohmann::json eventInfoGpuOverJson =
        nlohmann::json::parse(eventInfoGpuOverTemp);
    data_accessor::DataAccessor eventInfoGpuOverTempAcc(eventInfoGpuOverJson);
    data_accessor::CheckAccessor checkObj("GPU_SXM_[1-8]");
    bool passed = checkObj.check(eventInfoGpuOverTempAcc, failedGpuOverTemp);
    EXPECT_EQ(passed, true);
    auto assertedData = checkObj.getAssertedDevices();
    EXPECT_EQ(assertedData.size(), 1);
    auto& assertedEvent = assertedData.at(0);
    auto& device = assertedEvent.device;
    EXPECT_EQ(device, inputDevice);
    auto& index  = assertedEvent.deviceIndexTuple;
    EXPECT_EQ(index, device_id::PatternIndex(4));
}

/**
 * @brief Simulation of the Event Detection logic in event_detection.hpp
 */
TEST(CheckAccessor, Logic)
{
    auto eventInfo =
        R"(
      {
        "type": "DBUS",
        "object": "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_[1-8]",
        "interface": "xyz.openbmc_project.Inventory.Item.Cpu.OperatingConfig",
        "property": "MinSpeed",
        "check": {
          "equal": "0"
        }
      }
    )";

    auto pcTrigger =
        R"(
      {
        "type": "DBUS",
        "object": "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_5",
        "interface": "xyz.openbmc_project.Inventory.Item.Cpu.OperatingConfig",
        "property": "MinSpeed"
      }
    )";

    auto cmdLine =
        R"(
       {
        "type": "CMDLINE",
        "executable": "echo",
        "arguments": "AP0_BOOTCOMPLETE_TIMEOUT GPU_SXM_[1-8]",
        "check": {
          "equal": "AP0_BOOTCOMPLETE_TIMEOUT GPU_SXM_5"
        }
       }
    )";

    nlohmann::json jsonEventINfo = nlohmann::json::parse(pcTrigger);
    data_accessor::PropertyValue vv(int32_t(0));
    data_accessor::PropertyValue value(vv);
    data_accessor::DataAccessor pcTriggerAcc(jsonEventINfo, value);

    nlohmann::json jsonPcTrigger = nlohmann::json::parse(eventInfo);
    data_accessor::DataAccessor eventTriggerAcc(jsonPcTrigger);

    nlohmann::json jsonCmdLine = nlohmann::json::parse(cmdLine);
    data_accessor::DataAccessor cmdLineAcc(jsonCmdLine);

    std::unique_ptr<data_accessor::CheckAccessor>
        checkObj(new data_accessor::CheckAccessor("GPU_SXM_[1-8]"));

    checkObj->check(eventTriggerAcc, cmdLineAcc, pcTriggerAcc);
    EXPECT_EQ(checkObj->passed(), true);
}

TEST(CheckAccessor, LogicDiffEventType)
{
    auto eventInfo =
        R"(
      {
        "type": "DBUS",
        "object": "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_[1-8]",
        "interface": "com.nvidia.SMPBI",
        "property": "BackendErrorCode",
        "check": {
          "not_equal": "0"
        }
      }
    )";

    auto pcTrigger =
        R"(
      {
        "type": "DBUS",
        "object": "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_5",
        "interface": "om.nvidia.SMPBI",
        "property": "BackendErrorCode"
      }
    )";

    auto cmdLine =
        R"(
       {
        "type": "CMDLINE",
        "executable": "echo",
        "arguments": "AP0_BOOTCOMPLETE_TIMEOUT GPU_SXM_[1-8]",
        "check": {
          "equal": "AP0_BOOTCOMPLETE_TIMEOUT GPU_SXM_5"
        }
       }
    )";

    nlohmann::json jsonEventTrigger = nlohmann::json::parse(eventInfo);
    data_accessor::DataAccessor eventTriggerAcc(jsonEventTrigger);

    data_accessor::PropertyValue vv(int32_t(2));
    data_accessor::PropertyValue value(vv);
    nlohmann::json jsonPcTrigger = nlohmann::json::parse(pcTrigger);
    data_accessor::DataAccessor pcTriggerAcc(jsonPcTrigger, value);

    nlohmann::json jsonCmdLine = nlohmann::json::parse(cmdLine);
    data_accessor::DataAccessor cmdLineAcc(jsonCmdLine);

    std::string deviceType{"ERoT_GPU_SXM_[1-8]"};

    data_accessor::CheckAccessor checkObj(deviceType);
    auto ret = checkObj.check(eventTriggerAcc, cmdLineAcc, pcTriggerAcc);
    EXPECT_EQ(ret, true);
    auto assertedData = checkObj.getAssertedDevices();
    EXPECT_EQ(assertedData.size(), 1);
    auto& assertedEvent = assertedData.at(0);
    auto& device = assertedEvent.device;
    EXPECT_EQ(device, std::string{"ERoT_GPU_SXM_5"});
    auto& index  = assertedEvent.deviceIndexTuple;
    EXPECT_EQ(index, device_id::PatternIndex(5));
}


TEST(EventDeviceType, FullDeviceName)
{
    std::string deviceType{"PCIeSwitch_0"};
    event_info::EventNode event("ev");
    create_event_by_device_type(event, deviceType);
    // there is no index yet
    EXPECT_EQ(event.getDataDeviceType().index.dim(), 0);
    EXPECT_EQ(event.getFullDeviceName(), deviceType);

    event.setDeviceIndexTuple(device_id::PatternIndex(0));
    // now there is an index
    EXPECT_EQ(event.getDataDeviceType().index.dim(), 1);
    EXPECT_EQ(event.getFullDeviceName(), deviceType);

    deviceType = "GPU_[1-8]";
    event_info::EventNode gpu("gpu");
    create_event_by_device_type(gpu, deviceType);
    gpu.setDeviceIndexTuple(device_id::PatternIndex(3));
    EXPECT_EQ(gpu.getFullDeviceName(), "GPU_3");

    deviceType = "PCIeSwitch_0/Down[0-3]";
    event_info::EventNode down("down");
    create_event_by_device_type(down, deviceType);
    down.setDeviceIndexTuple(device_id::PatternIndex(3));
    EXPECT_EQ(down.getFullDeviceName(), "PCIeSwitch_0/Down3");

    deviceType = "NVSwitch_[0-3]/NVLink_[0-17]";
    event_info::EventNode nvlink("nvlink");
    create_event_by_device_type(nvlink, deviceType);
    nvlink.setDeviceIndexTuple(device_id::PatternIndex(3, 15));
    EXPECT_EQ(nvlink.getFullDeviceName(), "NVSwitch_3/NVLink_15");

    deviceType = "NVSwitch_[0|0-3]/NVLink_[1|0-17]";
    event_info::EventNode nvlinkSpec("nvlinkSpec");
    create_event_by_device_type(nvlinkSpec, deviceType);
    nvlinkSpec.setDeviceIndexTuple(device_id::PatternIndex(3, 15));
    EXPECT_EQ(nvlinkSpec.getFullDeviceName(), "NVSwitch_3/NVLink_15");
}


TEST(EventDeviceType, getFullDeviceNameSeparated)
{
    std::string deviceType{"PCIeSwitch_0"};
    std::vector<std::string> deviceNames;

    event_info::EventNode event("ev");
    create_event_by_device_type(event, deviceType);
    // there is no index yet
    EXPECT_EQ(event.getDataDeviceType().index.dim(), 0);
    device_id::PatternIndex  eventIndex(0);
    deviceNames = event.getFullDeviceNameSeparated(eventIndex);
    EXPECT_EQ(deviceNames.size(), 1);
    if (deviceNames.size() == 1)
    {
        EXPECT_EQ(deviceNames.at(0), deviceType);
    }

    deviceType = "GPU_[1-8]";
    event_info::EventNode gpu("gpu");
    create_event_by_device_type(gpu, deviceType);
    device_id::PatternIndex gpuIndex(3);
    EXPECT_EQ(gpu.getFullDeviceNameSeparated(gpuIndex).size(), 1);
    EXPECT_EQ(gpu.getFullDeviceNameSeparated(gpuIndex).at(0), "GPU_3");

    deviceType = "PCIeSwitch_0/Down_[0-3]";
    event_info::EventNode down("down");
    create_event_by_device_type(down, deviceType);
    device_id::PatternIndex downIndex(3);
    auto devices = down.getFullDeviceNameSeparated(downIndex);
    EXPECT_EQ(devices.size(), 2);
    EXPECT_EQ(devices.at(0), "PCIeSwitch_0");
    EXPECT_EQ(devices.at(1), "Down_3");

    device_id::PatternIndex downIndexComplete(0,2);
    devices = down.getFullDeviceNameSeparated(downIndexComplete);
    EXPECT_EQ(devices.size(), 2);
    EXPECT_EQ(devices.at(0), "PCIeSwitch_0");
    EXPECT_EQ(devices.at(1), "Down_2");

    deviceType = "NVSwitch_[0-3]/NVLink_[0-17]";
    event_info::EventNode nvlink("nvlink");
    create_event_by_device_type(nvlink, deviceType);
    device_id::PatternIndex nvIndex(3, 15);
    devices = nvlink.getFullDeviceNameSeparated(nvIndex);
    EXPECT_EQ(devices.size(), 2);
    EXPECT_EQ(devices.at(0), "NVSwitch_3");
    EXPECT_EQ(devices.at(1), "NVLink_15");

    deviceType = "NVSwitch_[0|0-3]/NVLink_[1|0-17]";
    event_info::EventNode nvlinkSpec("nvlinkSpec");
    create_event_by_device_type(nvlinkSpec, deviceType);
    device_id::PatternIndex nvIndexSpec(3, 15);
    devices = nvlinkSpec.getFullDeviceNameSeparated(nvIndexSpec);
    EXPECT_EQ(devices.size(), 2);
    EXPECT_EQ(devices.at(0), "NVSwitch_3");
    EXPECT_EQ(devices.at(1), "NVLink_15");
}

TEST(CheckAccessor, BitmapWithoutRangeInCMDLINE)
{
    const nlohmann::json triggerJson = {
        {"type", "DBUS"},
        {"object", "/xyz/openbmc_project/GpioStatusHandler"},
        {"interface", "xyz.openbmc_project.GpioStatus"},
        {"property", "I2C3_ALERT"},
        {"check", {{"equal", "true"}}}};

    const std::string deviceType{"ERoT_GPU_SXM_[1-8]"};

    data_accessor::DataAccessor triggerAccessor(triggerJson);
    data_accessor::DataAccessor dataTriggerAccessor(PropertyVariant(std::string{"true"}));

    const nlohmann::json jsonAccessor = {
        {"type", "CMDLINE"},
        {"executable", "/bin/echo"},
        {"arguments", "5"},
        {"check", {{"bitmap", "1"}}}};
    data_accessor::DataAccessor accessorFromJson(jsonAccessor);

    data_accessor::CheckAccessor trippleCheck(deviceType);
    auto ok =
            trippleCheck.check(triggerAccessor, accessorFromJson,
                               dataTriggerAccessor);

    EXPECT_EQ(ok, true);
    auto devicesAsserted = trippleCheck.getAssertedDevices();

    EXPECT_EQ(ok, true);
    EXPECT_EQ(devicesAsserted.size(), 2);
    EXPECT_EQ(devicesAsserted[0].device, "ERoT_GPU_SXM_1");
    EXPECT_EQ(devicesAsserted[1].device, "ERoT_GPU_SXM_3");
}

TEST(CheckAccessor, BitmapRangeInCMDLINE)
{
    const nlohmann::json triggerJson = {
        {"type", "DBUS"},
        {"object", "/xyz/openbmc_project/GpioStatusHandler"},
        {"interface", "xyz.openbmc_project.GpioStatus"},
        {"property", "I2C3_ALERT"},
        {"check", {{"equal", "true"}}}};

    const std::string deviceType{"ERoT_GPU_SXM_[1-8]"};

    data_accessor::DataAccessor triggerAccessor(triggerJson);
    data_accessor::DataAccessor dataTriggerAccessor(PropertyVariant(std::string{"true"}));

    const nlohmann::json jsonAccessor = {
        {"type", "CMDLINE"},
        {"executable", "/bin/echo"},
        {"arguments", "GPU_SXM_[1-8]"},
        {"check", {{"lookup", "_4"}}}};
    data_accessor::DataAccessor accessorFromJson(jsonAccessor);

    data_accessor::CheckAccessor trippleCheck(deviceType);
    auto ok =
            trippleCheck.check(triggerAccessor, accessorFromJson,
                               dataTriggerAccessor);

    EXPECT_EQ(ok, true);
    auto devicesAsserted = trippleCheck.getAssertedDevices();

    EXPECT_EQ(ok, true);
    EXPECT_EQ(devicesAsserted.size(), 1);
    EXPECT_EQ(devicesAsserted[0].device, "ERoT_GPU_SXM_4");
}

TEST(BootupSelfTestDiscovery, IgnoreDbusTriggerIfThereisAccessor)
{
    auto eventInfoRaw = R"(
 {
  "NVSwitch": [
   {
      "event": "Boot completed failure",
      "error_id": "NVSW_BOOT_COMPLETE_FAILURE-Error",
      "device_type": "NVSwitch_[0-3]",
      "category": [
        "firmware_status"
      ],
      "event_trigger": {
        "type": "DBUS",
        "object": "/xyz/openbmc_project/inventory/system/chassis/HGX_NVSwitch_[0-3]/PCIeDevices/NVSwitch_[0|0-3]",
        "interface": "xyz.openbmc_project.Inventory.Item.PCIeDevice",
        "property": "PCIeType",
        "check": {
          "not_equal": "xyz.openbmc_project.Inventory.Item.PCIeDevice.PCIeTypes.Gen4"
        }
      },
      "accessor": {
        "type": "CMDLINE",
        "executable": "mctp-vdm-util-wrapper",
        "arguments": "AP0_BOOTCOMPLETE_TIMEOUT NVSwitch_[0-3]",
        "check": {
          "equal": "1"
        }
      },
      "recovery": {
        "type": "DBUS",
        "object": "/xyz/openbmc_project/inventory/system/chassis/HGX_NVSwitch_[0-3]/PCIeDevices/NVSwitch_[0|0-3]",
        "interface": "xyz.openbmc_project.Inventory.Item.PCIeDevice",
        "property": "PCIeType",
        "check": {
          "equal": "xyz.openbmc_project.Inventory.Item.PCIeDevice.PCIeTypes.Gen4"
        }
      },
      "severity": "Critical",
      "resolution": "ERoT will try to recover AP by a reset/reboot. If there is still a problem, collect ERoT logs and reflash AP firmware with recovery flow.",
      "trigger_count": 1,
      "event_counter_reset": {
        "type": "",
        "metadata": ""
      },
      "redfish": {
        "message_id": "ResourceEvent.1.0.ResourceErrorsDetected",
        "origin_of_condition": "/redfish/v1/UpdateService/FirmwareInventory/HGX_FW_NVSwitch_[0|0-3]",
        "message_args": {
          "patterns": [
            "NVSwitch_[0|0-3] Firmware",
            "Boot Complete Failure"
          ],
          "parameters": []
        }
      },
      "telemetries": [],
      "action": "",
      "value_as_count": false
    }
  ]
 }
)" ;

    auto triggerJson = R"(
    {
        "type": "DBUS",
        "object": "/xyz/openbmc_project/inventory/system/chassis/HGX_NVSwitch_0/PCIeDevices/NVSwitch_0",
        "interface": "xyz.openbmc_project.Inventory.Item.PCIeDevice",
        "property": "PCIeType"
    }
)";

    event_info::EventMap eventMap;
    event_info::PropertyFilterSet propertyFilterSet;

    event_info::loadFromJson(
        eventMap, propertyFilterSet,
        event_detection::eventTriggerView, event_detection::eventAccessorView,
        event_detection::eventRecoveryView, nlohmann::json::parse(eventInfoRaw));

    std::string strValue("xyz.openbmc_project.Inventory.Item.PCIeDevice.PCIeTypes.Unkown");
    data_accessor::DataAccessor trigger(nlohmann::json::parse(triggerJson),
                                        data_accessor::PropertyValue(strValue));

    trigger.setDevice(std::string{"NVSwitch_0"});
    auto eventsSent = event_detection::EventDetection::eventDiscovery(trigger, true);
    EXPECT_EQ(eventsSent, 0);
}

TEST(BootupSelfTestDiscovery, IgnoreCmdLineAccessorWithoutData)
{
    auto eventInfoRaw = R"(
 {
  "GPU": [
   {
      "event": "Secure boot failure",
      "error_id": "GPU_SECURE_BOOT_FAILURE-ERROR",
      "device_type": "GPU_SXM_[1-8]",
      "category": [
        "firmware_status"
      ],
      "event_trigger": {
        "type": "DBUS",
        "object": "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_[1-8]",
        "interface": "xyz.openbmc_project.Inventory.Item.Cpu.OperatingConfig",
        "property": "MinSpeed",
        "check": {
          "equal": "0"
        }
      },
      "accessor": {
        "type": "CMDLINE",
        "executable": "mctp-vdm-util-wrapper",
        "arguments": "active_auth_status GPU_SXM_[1-8]",
        "check": {
          "equal": "6"
        }
      },
      "recovery": {
        "type": "DBUS",
        "object": "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_[1-8]",
        "interface": "xyz.openbmc_project.Inventory.Item.Cpu.OperatingConfig",
        "property": "MinSpeed",
        "check": {
          "not_equal": "0"
        }
      },
      "severity": "Critical",
      "resolution": "ERoT will try to recover AP by a reset/reboot. If there is still a problem, collect ERoT logs and reflash AP firmware with recovery flow.",
      "trigger_count": 1,
      "event_counter_reset": {
        "type": "",
        "metadata": ""
      },
      "redfish": {
        "message_id": "ResourceEvent.1.0.ResourceErrorsDetected",
        "origin_of_condition": "/redfish/v1/UpdateService/FirmwareInventory/HGX_FW_GPU_SXM_[0|1-8]",
        "message_args": {
          "patterns": [
            "GPU_SXM_[0|1-8] Firmware",
            "Secure Boot Failure"
          ],
          "parameters": []
        }
      },
      "telemetries": [],
      "action": "",
      "value_as_count": false
    }
  ]
 }
)" ;

    auto sfFailedJson = R"(
    {
        "type": "CMDLINE",
        "executable": "mctp-vdm-util-wrapper",
        "arguments": "active_auth_status GPU_SXM_4"
    }
)";

    event_info::EventMap eventMap;
    event_info::PropertyFilterSet propertyFilterSet;

    event_info::loadFromJson(
        eventMap, propertyFilterSet,
        event_detection::eventTriggerView, event_detection::eventAccessorView,
        event_detection::eventRecoveryView, nlohmann::json::parse(eventInfoRaw));

    data_accessor::DataAccessor
        tpFailedAccessor(nlohmann::json::parse(sfFailedJson));

    tpFailedAccessor.setDevice(std::string{"GPU_SXM_4"});
    auto eventsSent =
        event_detection::EventDetection::eventDiscovery(tpFailedAccessor, true);
    EXPECT_EQ(eventsSent, 0);
}

