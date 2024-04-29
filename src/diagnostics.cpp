#include "diagnostics.hpp"

#include "eventing_main.hpp"
#include "json_proc.hpp"
#include "printing_util.hpp"

#include <boost/algorithm/string/join.hpp>
// #include <boost/format.hpp>
#include <nlohmann/json.hpp>

#include <deque>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <optional>
#include <string>

namespace diagnostics
{
// Utils //////////////////////////////////////////////////////////////////////

std::string eventNodeId(const event_info::EventNode& eventNode)
{
    return eventNode.event + ", " + eventNode.getStringifiedDeviceType();
}

std::string wrongTypeMsg(const std::string& entityName,
                         const nlohmann::json& json,
                         nlohmann::json::value_t expectedType)
{
    return entityName + " type expected to be " +
           json_proc::getTypeAsString(expectedType) +
           ". Actual type: " + json_proc::getTypeAsString(json.type());
}

template <typename T>
std::string
    setToString(const std::vector<T>& elements,
                const std::function<std::string(const T& elem)>& transform)
{
    std::vector<std::string> output;
    std::transform(elements.begin(), elements.end(), std::back_inserter(output),
                   transform);
    return std::string("{") + boost::algorithm::join(output, ", ") + "}";
}

std::string
    wrongTypeMsg(const std::string& entityName, const nlohmann::json& json,
                 const std::vector<nlohmann::json::value_t>& expectedTypes)
{
    std::vector<std::string> output;
    std::transform(expectedTypes.begin(), expectedTypes.end(),
                   std::back_inserter(output),
                   [](const nlohmann::json::value_t& elem) {
                       return json_proc::getTypeAsString(elem);
                   });
    return entityName + " type expected to be one of {" +
           boost::algorithm::join(output, ", ") +
           "}. Actual type: " + json_proc::getTypeAsString(json.type());
}

bool checkProperType(const std::string& entityName, const nlohmann::json& json,
                     nlohmann::json::value_t expectedType,
                     nlohmann::ordered_json& problems)
{
    bool isCorrectType = (json.type() == expectedType);
    if (!isCorrectType)
    {
        problems += wrongTypeMsg(entityName, json, expectedType);
    }
    return isCorrectType;
}

bool checkProperType(const std::string& entityName, const nlohmann::json& json,
                     const std::vector<nlohmann::json::value_t>& expectedTypes,
                     nlohmann::ordered_json& problems)
{
    bool isCorrectType =
        (std::find(expectedTypes.cbegin(), expectedTypes.cend(), json.type()) !=
         expectedTypes.cend());
    if (!isCorrectType)
    {
        problems += wrongTypeMsg(entityName, json, expectedTypes);
    }
    return isCorrectType;
}

bool checkAttributeExists(const std::string& attrName,
                          const nlohmann::json& objectJson,
                          nlohmann::ordered_json& problems)
{
    auto hasAttr = objectJson.contains(attrName);
    if (!hasAttr)
    {
        problems +=
            std::string("Missing mandatory attribute '") + attrName + "'";
    }
    return hasAttr;
}

template <typename T>
bool checkMandatoryAttributeType(const std::string& attrName,
                                 const nlohmann::json& objectJson,
                                 const T& expected,
                                 nlohmann::ordered_json& problems)
{
    return checkAttributeExists(attrName, objectJson, problems) &&
           checkProperType("'" + attrName + "' attribute",
                           objectJson.at(attrName), expected, problems);
}

template <typename T>
bool checkOptionalAttributeType(const std::string& attrName,
                                const nlohmann::json& objectJson,
                                const T& expected,
                                nlohmann::ordered_json& problems)
{
    return !objectJson.contains(attrName) ||
           checkProperType("'" + attrName + "' attribute",
                           objectJson.at(attrName), expected, problems);
}

bool checkAccessorFormat([[maybe_unused]] const nlohmann::json& json,
                         [[maybe_unused]] nlohmann::ordered_json& problems)
{
    // marcinw:TODO:
    return true;
}

bool checkValueInSet(const nlohmann::json& json,
                     const std::vector<nlohmann::json>& expectedValues,
                     nlohmann::ordered_json& problems)
{
    bool isValueInSet =
        (std::find(expectedValues.cbegin(), expectedValues.cend(), json) !=
         expectedValues.cend());
    if (!isValueInSet)
    {
        // Object 'json.get<T>()' not found in 'expectedValues'
        problems +=
            std::string("Value ") + json.dump() + " expected to be one of " +
            setToString<nlohmann::json>(
                expectedValues, [](const nlohmann::json& elem) -> std::string {
                    return elem.dump();
                });
    }
    return isValueInSet;
}

std::string eventNodeStringRepr(const std::string& deviceCategory,
                                unsigned nodeIndex)
{
    // marcinw:TODO: why not just use path notation?
    return deviceCategory + ", " + std::to_string(nodeIndex);
}

// Test::Status ///////////////////////////////////////////////////////////////

bool Test::Status::providesResult() const
{
    return value == passed || value == done;
}

bool Test::Status::requiresAttention() const
{
    return value == failed;
}

std::string Test::Status::toString() const
{
    static std::map<int, std::string> stringRepresentations{
        {passed, "PASSED"},
        {failed, "FAILED"},
        {skipped, "SKIPPED"},
        {done, "DONE"}};
    return stringRepresentations.at(value);
}

// Test::Result ///////////////////////////////////////////////////////////////

Test::Result::Result(Status status, nlohmann::ordered_json details,
                     std::vector<std::shared_ptr<Test>> newTests) :
    status(status),
    details(details), newTests(newTests)
{}

// ArtifactTest ///////////////////////////////////////////////////////////////

template <typename ArtifactType>
ArtifactTest<ArtifactType>::ArtifactTest(
    const std::string& type, const std::string& instance,
    const std::vector<std::shared_ptr<Test>>& dependencies) :
    Test(type, instance, dependencies)
{}

template <typename ArtifactType>
Test::Result
    ArtifactTest<ArtifactType>::rawRun(const nlohmann::json& resultsSoFar)
{
    ResultExt testResultAndArtifact = rawRunWithArtifact(resultsSoFar);
    // marcinw:TODO: std::move?
    artifact = testResultAndArtifact.second;
    return testResultAndArtifact.first;
}

template <typename ArtifactType>
bool ArtifactTest<ArtifactType>::hasArtifact() const
{
    return artifact.has_value();
}

template <typename ArtifactType>
const ArtifactType& ArtifactTest<ArtifactType>::getArtifact() const
{
    if (hasArtifact())
    {
        return *artifact;
    }
    else
    {
        throw std::runtime_error("! hasArtifact()");
    }
}

// Test ///////////////////////////////////////////////////////////////////////

Test::Test(const std::string& type, const std::string& instance,
           const std::vector<std::shared_ptr<Test>>& dependencies) :
    type(type),
    instance(instance), dependencies(dependencies), result(std::nullopt)
{}

std::string Test::getTestType() const
{
    return type;
}

std::string Test::getTestInstanceName() const
{
    return instance;
}

std::string Test::toString() const
{
    return getTestType() + ": " + getTestInstanceName();
}

bool Test::isPerformed() const
{
    return result.has_value();
}

Test::Status Test::getStatus() const
{
    checkIsPerformed();
    return result->status;
}

bool Test::hasDetails() const
{
    checkIsPerformed();
    return !result->details.is_null();
}

const nlohmann::ordered_json& Test::getDetails() const
{
    checkIsPerformed();
    return result->details;
}

std::vector<std::shared_ptr<Test>> Test::getNewTests() const
{
    checkIsPerformed();
    return result->newTests;
}

void Test::run(const nlohmann::json& resultsSoFar)
{
    if (isPerformed())
    {
        throw std::runtime_error(alreadyPerformedErrMsg());
    }
    auto notPerformedDepIt =
        std::find_if_not(dependencies.cbegin(), dependencies.cend(),
                         [](const auto& dep) { return dep->isPerformed(); });
    if (notPerformedDepIt != dependencies.cend())
    {
        throw std::runtime_error(std::string("Dependency test '") +
                                 (*notPerformedDepIt)->toString() + "' for '" +
                                 this->toString() + "' was not performed");
    }
    try
    {
        auto badStatusDepIt = std::find_if_not(
            dependencies.cbegin(), dependencies.cend(),
            [](const auto& dep) { return dep->getStatus().providesResult(); });
        if (badStatusDepIt == dependencies.cend())
        {
            // All depending tests provided results
            result = this->rawRun(resultsSoFar);
        }
        else
        {
            result = Result(
                Status(Status::skipped),
                nlohmann::ordered_json{
                    {"info", "No results provided by the depending test"},
                    {"dependency_test", (*badStatusDepIt)->toString()}
                    // TODO:
                    // {"dependent_test_path",""}
                });
        }
    }
    catch (const std::exception& e)
    {
        result = Result(
            Status(Status::failed),
            std::string("Exception thrown during execution of the test: ") +
                e.what());
    }
    catch (...)
    {
        result = Result(
            Status(Status::failed),
            std::string(
                "Unknown exception thrown during execution of the test"));
    }
}

nlohmann::ordered_json Test::getFullResultNode() const
{
    checkIsPerformed();
    auto result = nlohmann::ordered_json{{"description", getDescription()},
                                         {"status", getStatus().toString()}};
    if (hasDetails())
    {
        result += {"details", getDetails()};
    }
    return result;
}

void Test::checkIsPerformed() const
{
    if (!isPerformed())
    {
        throw std::runtime_error(alreadyPerformedErrMsg());
    }
}

std::string Test::alreadyPerformedErrMsg() const
{
    return std::string("Test '") + this->toString() +
           std::string("' was already performed");
}

// JsonReadTest ///////////////////////////////////////////////////////////////

std::shared_ptr<JsonReadTest>
    JsonReadTest::create(const std::string& jsonFileName)
{
    return std::shared_ptr<JsonReadTest>(new JsonReadTest(jsonFileName));
}

JsonReadTest::JsonReadTest(const std::string& jsonFileName) :
    ArtifactTest("Json file read", jsonFileName, {}), jsonFileName(jsonFileName)
{}

std::string JsonReadTest::getDescription() const
{
    return std::string("Test whether the '") + jsonFileName +
           "' file can be read properly into nlohmann::json object";
}

JsonReadTest::ResultExt JsonReadTest::rawRunWithArtifact([
    [maybe_unused]] const nlohmann::json& resultsSoFar)
{
    nlohmann::ordered_json json;
    std::ifstream fileStream(jsonFileName);
    fileStream >> json;
    // Any errors in parsing are indicated by exception which will be forwarded
    // to the 'Test::run' function this one is called from and attended to
    // there. If we get to this point the parsing was successful
    return ResultExt(Result(Status(Status::passed), json), json);
}

// JsonSchemaTest /////////////////////////////////////////////////////////////

std::shared_ptr<JsonSchemaTest>
    JsonSchemaTest::create(const std::string& instanceName,
                           std::shared_ptr<json_schema::JsonSchema> schema,
                           std::shared_ptr<JsonReadTest> jsonReadTest)
{
    return std::shared_ptr<JsonSchemaTest>(
        new JsonSchemaTest(instanceName, schema, jsonReadTest));
}

JsonSchemaTest::JsonSchemaTest(const std::string& instanceName,
                               std::shared_ptr<json_schema::JsonSchema> schema,
                               std::shared_ptr<JsonReadTest> jsonReadTest) :
    Test("Check json format with its schema", instanceName, {jsonReadTest}),
    Dependency<JsonReadTest>(jsonReadTest), schema(schema)
{}

std::string JsonSchemaTest::getDescription() const
{
    return "Check whether the given json object '" + instance +
           "' conforms to its schema";
}

Test::Result JsonSchemaTest::rawRun([
    [maybe_unused]] const nlohmann::json& resultsSoFar)
{
    std::vector<json_proc::JsonFormatProblem> problems;
    if (schema->check(Dependency<JsonReadTest>::get()->getArtifact(), problems))
    {
        return Result(Status(Status::passed), nullptr, newTests());
    }
    else
    {
        std::vector<std::string> stringifiedProblems;
        std::transform(
            problems.begin(), problems.end(),
            std::back_inserter(stringifiedProblems),
            [](const json_proc::JsonFormatProblem& problem) -> std::string {
                return json_proc::to_string(problem);
            });
        return Result(Status(Status::failed), stringifiedProblems);
    }
}

std::vector<std::shared_ptr<Test>> JsonSchemaTest::newTests() const
{
    return {};
}

// DatParseTest ///////////////////////////////////////////////////////////////

std::shared_ptr<DatParseTest>
    DatParseTest::create(std::shared_ptr<JsonSchemaTest> datJsonSchemaTestDep)
{
    return std::shared_ptr<DatParseTest>(
        new DatParseTest(datJsonSchemaTestDep));
}

DatParseTest::DatParseTest(
    std::shared_ptr<JsonSchemaTest> datJsonSchemaTestDep) :
    ArtifactTest("Parse DAT json to native C++ object", "Device::populateMap",
                 {datJsonSchemaTestDep}),
    Dependency<JsonSchemaTest>(datJsonSchemaTestDep)
{}

std::string DatParseTest::getDescription() const
{
    return "Attempt to parse the nlohmann::json object into native "
           "C++ object using "
           "'void Device::populateMap(std::map<std::string, "
           "dat_traverse::Device>&, "
           "const nlohmann::json&)' "
           "function";
}

DatParseTest::ResultExt DatParseTest::rawRunWithArtifact([
    [maybe_unused]] const nlohmann::json& resultsSoFar)
{
    DatType dat;
    dat_traverse::Device::populateMap(
        dat, (nlohmann::json)Dependency<JsonSchemaTest>::get()
                 ->Dependency<JsonReadTest>::get()
                 ->getArtifact());
    // Any errors in parsing are indicated by exception which will be forwarded
    // to the 'Test::run' function this one is called from and attended to
    // there. If we get to this point the parsing was successful
    return ResultExt(Result(Status(Status::passed)), dat);
}

// EventInfoInnerConsistencyTest //////////////////////////////////////////////

std::shared_ptr<EventInfoInnerConsistencyTest>
    EventInfoInnerConsistencyTest::create(
        std::shared_ptr<JsonSchemaTest> eventInfoJsonSchemaTest)
{
    return std::shared_ptr<EventInfoInnerConsistencyTest>(
        new EventInfoInnerConsistencyTest(eventInfoJsonSchemaTest));
}

EventInfoInnerConsistencyTest::EventInfoInnerConsistencyTest(
    std::shared_ptr<JsonSchemaTest> eventInfoJsonSchemaTest) :
    Test("Check inner consistency of the event info json file",
         eventInfoJsonSchemaTest->Dependency<JsonReadTest>::get()
             ->getTestInstanceName(),
         {eventInfoJsonSchemaTest}),
    Dependency<JsonSchemaTest>(eventInfoJsonSchemaTest)
{}

std::string EventInfoInnerConsistencyTest::getDescription() const
{
    return "Givent the schema-conforming event info json file '" +
           getTestInstanceName() +
           ("', check whether it's internally consistent. "
            "(While schema tests the validity of json entries independently, "
            "this test checks the inter-relationships between fields)");
}

// using DomainTrackType = std::map<JsonPath, std::set<int>>;

bool evaluatesForArgsOf(const json_proc::JsonPattern& jsPattern,
                        const device_id::DeviceIdPattern& pivot,
                        const std::string& pivotName, nlohmann::json& problems)
{
    bool allOk = true;
    for (const auto& arg : pivot.domain())
    {
        try
        {
            jsPattern.eval(arg);
        }
        catch (const std::exception& e)
        {
            problems.push_back(
                "Event node doesn't evaluate successfully at point " +
                to_string(arg) + " (in the domain of '" + pivotName +
                "', value '" + pivot.pattern() +
                "'). Exception caught: " + e.what());
            allOk = false;
        }
    }
    return allOk;
}

bool evaluatesForArgsOfIfGiven(const json_proc::JsonPattern& jsPattern,
                               const json_proc::JsonPath& jsPath,
                               nlohmann::json& problems)
{
    if (json_proc::contains(jsPattern.getJson(), jsPath))
    {
        return evaluatesForArgsOf(
            jsPattern,
            device_id::DeviceIdPattern(
                json_proc::nodeAt(jsPattern.getJson(), jsPath)
                    .get<std::string>()),
            json_proc::to_string(jsPath), problems);
    }
    else
    {
        return true;
    }
}

bool checkBracketsConsistency(const nlohmann::json& eventNode,
                              nlohmann::json& problems)
{
    // For each string-type entry in 'json', and for each axis in its domain,
    // the set of arguments should not be different than the respective set
    // (if there is one) for any other string-type 'json' entry
    json_proc::JsonPattern jsPattern(eventNode);
    return evaluatesForArgsOfIfGiven(
               jsPattern, json_proc::JsonPath{"device_type"}, problems) &&
           evaluatesForArgsOfIfGiven(
               jsPattern, json_proc::JsonPath{"event_trigger", "object"},
               problems) &&
           evaluatesForArgsOfIfGiven(
               jsPattern, json_proc::JsonPath{"accessor", "object"}, problems);
}

Test::Result EventInfoInnerConsistencyTest::rawRun([
    [maybe_unused]] const nlohmann::json& resultsSoFar)
{
    return Result(Status(Status::passed),
                  nlohmann::json("WIP, always ending with succeess for now"));
}

// Test::Result
//     EventInfoInnerConsistencyTest::rawRun(const nlohmann::json& resultsSoFar)
// {
//     nlohmann::ordered_json problemsList = nlohmann::ordered_json::object();
//     for (const auto& [deviceCategory, events] :
//          Dependency<JsonSchemaTest>::get()
//              ->Dependency<JsonReadTest>::get()
//              ->getArtifact()
//              .items())
//     {
//         for (unsigned i = 0u; i < events.size(); ++i)
//         {
//             nlohmann::json problems = nlohmann::json::array();
//             if (!checkBracketsConsistency(events.at(i), problems))
//             {
//                 problemsList[eventNodeStringRepr(deviceCategory, i)] =
//                 problems;
//             }
//         }
//     }
//     if (problemsList.empty())
//     {
//         return Result(Status(Status::passed));
//     }
//     else
//     {
//         return Result(Status(Status::failed), problemsList);
//     }
// }

// EventInfoParseTest /////////////////////////////////////////////////////////

std::shared_ptr<EventInfoParseTest>
    EventInfoParseTest::create(std::shared_ptr<EventInfoInnerConsistencyTest>
                                   eventInfoInnerConsistencyTest)
{
    return std::shared_ptr<EventInfoParseTest>(
        new EventInfoParseTest(eventInfoInnerConsistencyTest));
}

EventInfoParseTest::EventInfoParseTest(
    std::shared_ptr<EventInfoInnerConsistencyTest>
        eventInfoInnerConsistencyTest) :
    ArtifactTest(
        "Parse event info json to native C++ object",
        eventInfoInnerConsistencyTest->Dependency<JsonSchemaTest>::get()
            ->Dependency<JsonReadTest>::get()
            ->getTestInstanceName(),
        {eventInfoInnerConsistencyTest}),
    Dependency<EventInfoInnerConsistencyTest>(eventInfoInnerConsistencyTest)
{}

std::string EventInfoParseTest::getDescription() const
{
    return "Parse event info json object from file '" +
           Dependency<EventInfoInnerConsistencyTest>::get()
               ->Dependency<JsonSchemaTest>::get()
               ->Dependency<JsonReadTest>::get()
               ->getTestInstanceName() +
           ("' to the native C++ object 'event_info::EventMap', using "
            "'event_info::loadFromJson(...)' function");
}

EventInfoParseTest::ResultExt EventInfoParseTest::rawRunWithArtifact([
    [maybe_unused]] const nlohmann::json& resultsSoFar)
{
    event_info::EventMap eventMap;
    event_info::PropertyFilterSet propertyFilterSet;
    event_info::EventTriggerView eventTriggerView;
    event_info::EventAccessorView eventAccessorView;
    event_info::EventRecoveryView eventRecoveryView;
    event_info::loadFromJson(eventMap, propertyFilterSet, eventTriggerView,
                             eventAccessorView, eventRecoveryView,
                             Dependency<EventInfoInnerConsistencyTest>::get()
                                 ->Dependency<JsonSchemaTest>::get()
                                 ->Dependency<JsonReadTest>::get()
                                 ->getArtifact());
    return ResultExt(Result(Status(Status::passed)), eventMap);
}

// EventInfoDatInterConsistencyTest ///////////////////////////////////////////

std::shared_ptr<EventInfoDatInterConsistencyTest>
    EventInfoDatInterConsistencyTest::create(
        std::shared_ptr<DatParseTest> datParseTest,
        std::shared_ptr<EventInfoParseTest> eventInfoParseTest)
{
    return std::shared_ptr<EventInfoDatInterConsistencyTest>(
        new EventInfoDatInterConsistencyTest(datParseTest, eventInfoParseTest));
}

EventInfoDatInterConsistencyTest::EventInfoDatInterConsistencyTest(
    std::shared_ptr<DatParseTest> datParseTest,
    std::shared_ptr<EventInfoParseTest> eventInfoParseTest) :
    Test("EventInfoDatInterConsistencyTest: TODO",
         "EventInfoDatInterConsistencyTest instance: TODO",
         {datParseTest, eventInfoParseTest}),
    Dependency<DatParseTest>(datParseTest), Dependency<EventInfoParseTest>(
                                                eventInfoParseTest)
{}

std::string EventInfoDatInterConsistencyTest::getDescription() const
{
    return "TODO";
}

Test::Result EventInfoDatInterConsistencyTest::rawRun([
    [maybe_unused]] const nlohmann::json& resultsSoFar)
{
    return Result(Status(Status::passed));
}

// Test::Result
//     EventNodeDatConsistencyTest::rawRun(const nlohmann::json& resultsSoFar)
// {
//     nlohmann::ordered_json problems = nlohmann::json::array();
//     // problems += checkAllMainDevicesExistInDat();
//     // marcinw:TODO: ?
//     // checkAllCategoriesCorrespondToLayers()
//     // marcinw:TODO: reference
//     DatType dat = Dependency<DatParseTest>::get()->getArtifact();
//     // event_info::EventNode eventNode =
//     event_info::EventNode eventNode =
//         Dependency<EventNodeParseTest>::get()->getArtifact();
//     device_id::DeviceIdPattern pat(eventNode.getMainDeviceType());
//     for (const auto& arg : pat.domain())
//     {
//         std::string deviceId = pat.eval(arg);
//         if (!dat.contains(deviceId))
//         {
//             problems += std::string("Device '") + deviceId +
//                         "' being an expansion of main device patern '" +
//                         pat.pattern() + "' for index tuple " + to_string(arg)
//                         + " doesn't exist in DAT";
//         }
//     }
//     if (problems.empty())
//     {
//         return Result(Status(Status::passed), nullptr,
//                       {PossibleOriginsOfConditionTest::create(
//                           Dependency<EventNodeParseTest>::get(),
//                           Dependency<DatParseTest>::get())});
//     }
//     else
//     {
//         return Result(Status(Status::failed), problems);
//     }
// }

// PossibleOriginsOfConditionTest /////////////////////////////////////////////

std::shared_ptr<PossibleOriginsOfConditionTest>
    PossibleOriginsOfConditionTest::create(
        std::shared_ptr<EventInfoDatInterConsistencyTest>
            eventInfoDatInterConsistencyTest)
{
    return std::shared_ptr<PossibleOriginsOfConditionTest>(
        new PossibleOriginsOfConditionTest(eventInfoDatInterConsistencyTest));
}

PossibleOriginsOfConditionTest::PossibleOriginsOfConditionTest(
    std::shared_ptr<EventInfoDatInterConsistencyTest>
        eventInfoDatInterConsistencyTest) :
    Test("List possible origins of condition",
         "PossibleOriginsOfConditionTest instance: TODO",
         {eventInfoDatInterConsistencyTest}),
    Dependency<EventInfoDatInterConsistencyTest>(
        eventInfoDatInterConsistencyTest)
{}

// std::shared_ptr<PossibleOriginsOfConditionTest>
//     PossibleOriginsOfConditionTest::create(
//         std::shared_ptr<EventNodeParseTest> eventNodeParseTest,
//         std::shared_ptr<DatParseTest> datParseTest)
// {
//     return std::shared_ptr<PossibleOriginsOfConditionTest>(
//         new PossibleOriginsOfConditionTest(eventNodeParseTest,
//         datParseTest));
// }

// PossibleOriginsOfConditionTest::PossibleOriginsOfConditionTest(
//     std::shared_ptr<EventNodeParseTest> eventNodeParseTest,
//     std::shared_ptr<DatParseTest> datParseTest) :
//     Test("List possible origins of condition",
//          eventNodeParseTest->getTestInstanceName(),
//          {eventNodeParseTest, datParseTest}),
//     Dependency<EventNodeParseTest>(eventNodeParseTest),
//     Dependency<DatParseTest>(datParseTest)
// {}

std::string PossibleOriginsOfConditionTest::getDescription() const
{
    return "TODO";
}

// std::string PossibleOriginsOfConditionTest::getDescription() const
// {
//     return std::string("Given parsed DAT and the event node '") +
//            eventNodeId(Dependency<EventNodeParseTest>::get()->getArtifact())
//            + std::string("' list all origins of condition which can be "
//                        "potentially identified by root cause tracer, "
//                        "per each device this event can occur on");
// }

// marcinw:TODO: torename
nlohmann::json tmp(const nlohmann::json& jsonEventNode,
                   event_info::EventNode& eventNode, DatType& dat)
{
    nlohmann::json nodeResult = nlohmann::json::object();
    nlohmann::json& eventDetails = nodeResult["event_details"];
    eventDetails["event"] = jsonEventNode.at("event");
    eventDetails["device_type"] = jsonEventNode.at("device_type");
    if (jsonEventNode.contains("category"))
    {
        eventDetails["category"] = jsonEventNode.at("category");
    }
    if (jsonEventNode.contains("redfish"))
    {
        if (jsonEventNode.at("redfish").contains("origin_of_condition"))
        {
            eventDetails["origin_of_condiition"] =
                jsonEventNode.at("redfish").at("origin_of_condition");
        }
    }
    device_id::DeviceIdPattern pat(eventNode.getStringifiedDeviceType());
    nlohmann::json& originsOfCondition = nodeResult["origins_of_condition"];
    if (eventNode.hasFixedOriginOfCondition())
    {
        for (const auto& arg : pat.domain())
        {
            auto deviceName = pat.eval(arg);
            event_info::EventNode eventNodeCopy = eventNode;
            eventNodeCopy.setDeviceIndexTuple(arg);
            originsOfCondition[deviceName] =
                nlohmann::json::array({*eventNodeCopy.getOriginOfCondition()});
        }
    }
    else // ! eventNode.hasFixedOriginOfCondition()
    {
        device_id::DeviceIdPattern mainDevicePattern(
            eventNode.getMainDeviceType());
        for (const auto& arg : pat.domain())
        {
            eventNode.setDeviceIndexTuple(arg);
            auto deviceName = pat.eval(arg);
            originsOfCondition[deviceName] =
                event_handler::DATTraverse::getTestLayerSubAssociations(
                    dat, mainDevicePattern.eval(arg), eventNode);
        }
    }
    return nodeResult;
}

Test::Result PossibleOriginsOfConditionTest::rawRun([
    [maybe_unused]] const nlohmann::json& resultsSoFar)
{
    DatType dat = Dependency<EventInfoDatInterConsistencyTest>::get()
                      ->Dependency<DatParseTest>::get()
                      ->getArtifact();
    nlohmann::json results = nlohmann::json::object();
    for (auto [deviceCategory, events] :
         Dependency<EventInfoDatInterConsistencyTest>::get()
             ->Dependency<EventInfoParseTest>::get()
             ->getArtifact())
    {
        for (unsigned i = 0u; i < events.size(); ++i)
        {
            results[eventNodeStringRepr(deviceCategory, i)] =
                tmp(Dependency<EventInfoDatInterConsistencyTest>::get()
                        ->Dependency<EventInfoParseTest>::get()
                        ->Dependency<EventInfoInnerConsistencyTest>::get()
                        ->Dependency<JsonSchemaTest>::get()
                        ->Dependency<JsonReadTest>::get()
                        ->getArtifact()
                        .at(deviceCategory)
                        .at(i),
                    events.at(i), dat);
        }
    }
    return Result(Status(Status::done), results);
}

// Main function //////////////////////////////////////////////////////////////

int run(const std::string& datFile, const std::string& eventInfoFile,
        std::ostream& reportStream, std::ostream& commentsStream)
{
    auto datJsonReadTest = JsonReadTest::create(datFile);
    auto datJsonSchematTest =
        JsonSchemaTest::create("DAT", eventing::datSchema(), datJsonReadTest);
    auto datParseTest = DatParseTest::create(datJsonSchematTest);

    auto eventInfoJsonReadTest = JsonReadTest::create(eventInfoFile);
    auto eventInfoJsonSchemaTest = JsonSchemaTest::create(
        "event info", eventing::eventInfoJsonSchema(), eventInfoJsonReadTest);
    auto eventInfoInnerConsistencyTest =
        EventInfoInnerConsistencyTest::create(eventInfoJsonSchemaTest);
    auto eventInfoParseTest =
        EventInfoParseTest::create(eventInfoInnerConsistencyTest);

    auto eventInfoDatInterConsistencyTest =
        EventInfoDatInterConsistencyTest::create(datParseTest,
                                                 eventInfoParseTest);
    auto possibleOriginsOfConditionTest =
        PossibleOriginsOfConditionTest::create(
            eventInfoDatInterConsistencyTest);

    // When modifying this list always make sure the order preserves
    // dependencies (if 'x' depends on 'y' then 'y' must be earlier on the
    // list). Ensuring proper ordering automatically yet to be done
    std::deque<std::shared_ptr<Test>> tests{datJsonReadTest,
                                            datJsonSchematTest,
                                            datParseTest,
                                            eventInfoJsonReadTest,
                                            eventInfoJsonSchemaTest,
                                            eventInfoInnerConsistencyTest,
                                            eventInfoParseTest,
                                            eventInfoDatInterConsistencyTest,
                                            possibleOriginsOfConditionTest};

    nlohmann::ordered_json diagnosticsResult = nlohmann::json::object();
    while (!tests.empty())
    {
        std::shared_ptr<Test> test = tests.front();
        tests.pop_front();
        commentsStream << test->toString() << "... ";
        test->run(diagnosticsResult);
        commentsStream << test->getStatus().toString() << std::endl;
        auto& resultNode =
            diagnosticsResult[test->getTestType()][test->getTestInstanceName()];
        resultNode = test->getFullResultNode();
        if (test->getStatus().requiresAttention())
        {
            // In the wild there may be no good tools to browse or query the
            // resulting json so it's good to print the critical results right
            // to the comments stream (expected to be printed in console)
            commentsStream << resultNode.dump(2) << std::endl;
        }
        for (const auto& newTest : test->getNewTests())
        {
            tests.push_back(newTest);
        }
    }

    reportStream << diagnosticsResult.dump(2) << std::endl;
    return 0;
}

} // namespace diagnostics
