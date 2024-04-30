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
#include "dat_traverse.hpp"
#include "device_id.hpp"
#include "event_info.hpp"
#include "json_schema.hpp"

#include <nlohmann/json.hpp>

#include <iostream>
#include <optional>
#include <string>
#include <vector>

/**
 * @brief A framework for writing diagnostics unit tests
 *
 * The tests are meant to be run when launching mon_evt in a special diagnostics
 * mode, where none of the usual functionalities are started and only a
 * series of tests is performed after which the program exits.
 *
 * Diagnostics mode is a middle ground between error injection and google tests.
 *
 * @code
 *                  Google     Error injection  Diagnostics
 *                  tests      (SW & HW)        mode
 *    ------------------------------------------------------
 *     Test object  Code unit  mon_evt as blackbox  Code unit
 *    ------------------------------------------------------
 *     Environment  Mocked     Real             Real
 *
 * @endcode
 *
 * ("Real environment" = running in target OS, using real configuration files).
 *
 * The goal is to allow for testing and debugging pieces of the code running in
 * a target environment while being executed in a highly controlled manner (as
 * opposed to observing their behavior "in the wild" through inspecting logs).
 *
 * After the tests are done the results are printed in a structured form of json
 * object to the standard output, a file, or any other stream of choice.
 *
 * Because the testing, by its own nature, cannot assume that neither the
 * code pieces nor the provided configuration are correct, and yet any but the
 * most trivial tests must require some level of data and algorithm correctness
 * before proceeding, the tests naturally fall under a partially ordered
 * execution scheme, defined by the dependency relations, where more fundamental
 * tests are performed first and, building on their results, the more
 * sophisticated ones are performed later. For example, checking whether a
 * subject device given in some event node definition from 'event_info.json'
 * exists in the provided 'dat.json' can only be done after the formats of both
 * 'event_info.json' and 'dat.json' are determined to be correct. This is
 * referred to by "bootstrapping testing".
 *
 * Every test can finish with a "passed" or "failed" status. If a test is
 * dependent on some other test which failed, it itself cannot be performed. In
 * this case it finishes with a "skipped" status, which also prevents any other
 * tests depending on this one from being executed. The other, successful,
 * branches are nevertheless continued.
 *
 * In this way the noise from tests reporting errors having root cause in
 * different place than what they actually test is reduced (as opposed to the
 * case of always running all the tests no matter what), while at the same time
 * a maximum possible volume of tests is performed, saving from potentially
 * costly process of addressing issues outside of current focus (as opposed to
 * the case of finishing testing on the first error).
 *
 * A "test" may also have somewhat different goal than determining whether some
 * piece of data or code is correct or not and instead focus on obtaining some
 * relevant information about the mon_evt, configuration or the system, to be
 * included in the json results and used for diagnosing problems.
 * Examples: values from the accessors found in 'dat.json' and
 * 'event_info.json', or a list of possible origins of condition given the
 * provided configuration and the OOC algorithm implemented. In this case
 * the test finishes with simply "done" status, having the same repercussions
 * for dependant tests as "passed".
 *
 * Summary:
 *
 * @code
 *  Test     Dependent test
 *  status   can be run
 * -------------------------
 *  passed   yes
 *  done     yes
 *  failed   no
 *  skipped  no
 * @endcode
 *
 * Every test has its "class" and "instance", where class specifies a testing
 * procedure and instance specifies concrete parameters of this procedure. For
 * example, testing the conformance of a given json to the provided schema is a
 * single class of tests having two instances used, one for 'dat.json' and one
 * for 'event_info.json'.
 *
 * The class and instance have direct correspondence to the C++ elements - user
 * specifies a class of tests by writing a C++ class deriving from @c Test and
 * their instances by constructing instances of this class for some given
 * paramaters. For the example given above this corresopnds to writing the @c
 * JsonSchemaTest class accepting a json and schema objects in the constructor,
 * and instantiating her for json objects read from both 'dat.json' and
 * 'event_info.json', along with the corresponding schemas.
 *
 * The entry point for running diagnostics testing is the 'run' function, where
 * all the test instances are created, put on a queue and executed in
 * succession.
 *
 * The tests results json is a json object with keys being the string
 * representations of all the test classes used (as provided in the @c type
 * argument of @c Test class constructor). The associated value is in turn also
 * an object with keys being the string representation of all the instances of
 * this class (as provided in the @c instance argument of the @c Test class
 * constructor). The value of this key is the actual unit test result.
 * Example:
 *
 * @code
 * {
 *   "Json file read": {
 *     "dat.json": <unit test results>,
 *     "event_info.json": <unit test results>
 *   },
 *   "Check json format with its schema": {
 *     "DAT": <unit test results>,
 *     "event info": <unit test results>
 *   },
 *   "Parse DAT json to native C++ object": {
 *     "Device::populateMap": <unit test results>
 *   }
 *   ...
 * }
 * @endcode
 *
 * The '<unit test results>' json has the following format
 *
 * @code
 * {
 *   "description": "<Description of what was tested>",
 *   "status": "<one of 'PASSED', 'FAILED', 'SKIPPED', 'DONE'>"
 *   "details": <arbitrary json object>
 * }
 * @endcode
 *
 * where the "details" attribute is optional. Example:
 *
 * @code
 * {
 *   "description": "Test whether the 'dat.json' file can be read properly into
 * nlohmann::json object",
 *   "status": "PASSED",
 *   "details": {
 *     "Baseboard_0": {
 *       "type": "regular",
 *       "association": [
 *         "GPU_SXM_1",
 *         "GPU_SXM_2",
 *         ...
 *       ],
 *       "device_core_api_index": "0",
 *       "power_rail": [],
 *       ...
 *     },
 *     ...
 *   }
 * }
 * @endcode
 */

namespace diagnostics
{

/**
 * @brief Root class for all the diagnostic unit tests
 *
 * The class is abstract. To instantiate the inheriting class two functions must
 * be implemented: 'getDescription()' and 'rawRun(..)'.
 *
 * Method 'getDescription()' is expected to return a short description of the
 * test, ideally instance-unique. It will be used in constructing the result
 * json object. The 'rawRun(..)'  method is called by 'run(..)' and should carry
 * out the actual testing. It's allowed to throw an exception, in which case the
 * test run status will be "failed". The 'rawRun(..)' will never be called
 * unless all the depending tests in @c dependencies vector were executed before
 * and succeeded. The implementer can therefore assume all the conditions
 * ensured by the depending tests are met. In this case 'run(..)' method
 * finishes the test with "skipped" status never calling the 'rawRun(..)'
 * function.
 *
 * The 'rawRun(..)' should return a 'Result' object containing
 * a series of information:
 *
 * - @c status :: One of the "passed", "failed", "done".
 *
 * - @c details :: A json object containing details about the test results. For
 * "passed" status the details are rarely needed. For "failed" it's usually good
 * idea to provide some additional data which will be included in the results
 * json and presented to the user. Even more so with "done" status, of which the
 * whole point is to produce some data for the user.
 *
 * - @c newTests :: A test can produce new tests. This allows for dynamically
 * extending the test suite. Currently this feature is not used as it proved to
 * be too complex for what it offers.
 *
 * Known subclasses:
 * - Test
 *   - ArtifactTest
 *     - JsonReadTest
 *     - DatParseTest
 *     - EventInfoParseTest
 *   - JsonSchemaTest
 *   - EventInfoInnerConsistencyTest
 *   - EventInfoDatInterConsistencyTest
 *   - PossibleOriginsOfConditionTest
 */

class Test
{
  public:
    virtual ~Test() = default;

    /**
     * @brief Representation of the test's running status.
     *
     * Used directly in progress logging via its 'toString()' method.
     */
    struct Status
    {
        enum
        {
            /**
             * @brief Test passed, auxiliary data obtained, dependant tests can
             * be run
             */
            passed,
            /**
             * @brief Test failed, auxiliary data could not be obtained,
             * dependant tests cannot be run
             */
            failed,
            /**
             * @brief Test could not be run because of the depending tests
             * failing or also skipped
             */
            skipped,
            /**
             * @brief Test performed with success, auxiliary data obtained,
             * dependant tests can be run.
             *
             * Similar to 'passed' status, but reserved for tests which focus on
             * providing auxiliary data instead of providing yes / no answer to
             * whether something works.
             */
            done
        } value;

        /**
         * @brief Return true for status which allows the dependant tests to be
         * run
         *
         * Currently 'done' and 'passed'.
         */
        bool providesResult() const;

        /**
         * @brief Return true for status for which it's useful to print the json
         * details in right in the logging stream for quick checks
         *
         * Currently 'failed'.
         */
        bool requiresAttention() const;

        /**
         * @brief Convert the status to string representation
         */
        std::string toString() const;
    };

    /**
     * @brief Struct defining the sum of all results a test produces
     */
    struct Result
    {
        /**
         * @brief Status with which the test finished
         *
         * If 'failed' the @c details field should also be set to some non-null
         * json object containing details about the failure.
         */
        Status status;

        /**
         * @brief Json object containing details about running the tests
         *
         * It's inserted directly into the "details" attribute of the json
         * result entry
         *
         * @code
         * {
         *   "description": "...",
         *   "status": "..."
         *   "details": <here the 'details' value is inserted>
         * }
         * @endcode
         *
         */
        nlohmann::ordered_json details;

        /**
         * @brief The list of new tests to be spawned from the recently finished
         * one
         */
        std::vector<std::shared_ptr<Test>> newTests;

        Result(Status status, nlohmann::ordered_json details = nullptr,
               std::vector<std::shared_ptr<Test>> newTests =
                   std::vector<std::shared_ptr<Test>>());
    };

    /**
     * @brief Construct the diagnostics unit test
     *
     * @param[in] type The type of test. Generally should correspond directly to
     * the class inheriting from @c Test and be the same for all its instances.
     * Should be human-readable and short, as it will be used as a key in the
     * json object grouping the test results.
     *
     * @param[in] instance The name of the instance of a test. Generally should
     * correspond directly to the instance of the class inheriting from @c Test
     * and identify it uniquely. Should be human-readable and short, as it will
     * be used as a key for the test result object.
     *
     * @param[in] dependencies A list of other tests which must be successfully
     * finished (their status returning true for 'providesResult()' method)
     * before this one can be run.
     */
    Test(const std::string& type, const std::string& instance,
         const std::vector<std::shared_ptr<Test>>& dependencies);

    /** @brief Return what was passed as @c type in the constructor **/
    std::string getTestType() const;

    /** @brief Return what was passed as @c instance in the constructor **/
    std::string getTestInstanceName() const;

    /**
     * @brief Return a string uniquely identifying this test
     *
     * The result is some composition of the 'getTestType()' and
     * 'getTestInstanceName()'. Used in logging the progress of testing.
     */
    std::string toString() const;

    /**
     * @brief Run the test
     *
     * After it's run the methods 'getStatus()', 'getDetails()', 'hasDetails()',
     * 'getNewTests()', 'getFullResultNode()' can be used to gather information
     * about the results.
     *
     * After the test was run it cannot be re-run.
     *
     * @param[in] resultsSoFar Full testing results json element built so far
     * before running this test
     */
    void run(const nlohmann::json& resultsSoFar);

    /** @brief Return @c true if 'run()' was called, @c false otherwise  **/
    bool isPerformed() const;

    /**
     * @brief Return the status of running the test.
     *
     * Equals to the @c status field of the @c Result object returned by
     * 'rawRun(..)'
     */
    Status getStatus() const;

    /**
     * @brief Return the details of running the tests (issues found, auxiliary
     * data computed, etc)
     *
     * Equals to the @c details field of the @c Result object returned by
     * 'rawRun(..)'
     */
    const nlohmann::ordered_json& getDetails() const;

    /**
     * @brief Return @c true if 'getDetails(..)' returns null json, @c false
     * otherwise
     */
    bool hasDetails() const;

    /**
     * @brief Return the list of new tests to be spawned from this one
     *
     * Equals to the @c newTests field of the @c Result object returned by
     * 'rawRun(..)'
     */
    std::vector<std::shared_ptr<Test>> getNewTests() const;

    /**
     * @brief Return the json object describing the test results
     *
     * The object follows the format
     *
     * @code
     * {
     *   "description": <this->getDescription()>,
     *   "status": <this->getStatus().toString()>
     *   "details": <this->getDetails()>
     * }
     * @endcode
     *
     * The attribute "details" is omitted if 'this->hasDetails()' returns @c
     * false.
     */
    nlohmann::ordered_json getFullResultNode() const;

  protected:
    std::string type;
    std::string instance;
    std::vector<std::shared_ptr<Test>> dependencies;

    /**
     * @brief A short description of the test
     *
     * Ideally instance-unique. Should be human readable and descriptive,
     * ideally a full, one sentence. Will be used in constructing the json
     * result object.
     *
     * Example from some instance of the @c EventInfoParseTest class: "Parse
     * event info json object from file '${datadir}/event_info.json' to
     * the native C++ object 'event_info::EventMap', using
     * 'event_info::loadFromJson(...)' function"
     */
    virtual std::string getDescription() const = 0;

    /**
     * @brief Core function performing the tests
     *
     * The function is called by 'run()', ensuring the following:
     *
     * 1. 'rawRun()' is never called more than once.
     *
     * 2. All tests present in the @c dependencies were run before and finished
     * with a status for which 'providesResult()' returns @c true.
     *
     * 3. Parameter @c resultsSoFar contains the whole testing results json
     * element built so far by other tests.
     *
     * @param[in] resultsSoFar Full testing results json element built so far
     * before running this test. This parameter is NOT for communicating the
     * testing results
     *
     * @return A testing result object. See the documentation of @c Result for
     * more details.
     */
    virtual Result rawRun(const nlohmann::json& resultsSoFar) = 0;

  private:
    std::optional<Result> result;

    void checkIsPerformed() const;
    std::string alreadyPerformedErrMsg() const;
};

/**
 * @brief A storage class
 *
 * It's an often occuring pattern for a test to store pointers to other tests,
 * either the tests the one in question is dependent on, or test which would be
 * needed by the spawned ones and needs to be stored before forwarding.
 *
 * In most of the cases the pointers stored are all of different type. So
 * instead of creating fields for each of them in every such class it can simply
 * inherit from 'Result<T>' for all test types 'T' it needs, then access them in
 * methods via constructs like 'DependentOn<JsonReadTest>::test',
 * 'DependentOn<DatParseTest>::test', or 'DependentOn<DatParseTest>::get()'
 *
 * As a rule, a class should never initialize the 'test' field in its
 * 'DependentOn<T>' parent to a test that its not dependent on.
 */
template <typename TestType>
class Dependency
{
  protected:
    std::shared_ptr<TestType> test;

    Dependency(std::shared_ptr<TestType> test) : test(test)
    {}

  public:
    std::shared_ptr<TestType> get() const
    {
        return test;
    }
};

/**
 * @brief A form of test which, as a side effect, calculates some data that can
 * be useful for other tests
 *
 * This piece of data is called "artifact". A good example of an artifact is a
 * memory object of type 'event_info::EventMap' resulting from testing whether
 * the given event info json object can be properly parsed. The obtained
 * 'event_info::EventMap' object can then be used by some higher-level tests.
 *
 * The class remains abstract like the base @c Test, but the user is expected to
 * override the 'rawRunWithArtifact()' function instead of 'rawRun()'. The
 * 'rawRun()' calls 'rawRunWithArtifact()', and handles the extended result
 * type, saving the artifact in @c artifact field.
 */
template <typename ArtifactType>
class ArtifactTest : public Test
{
  public:
    /** @brief An extension of the standard @c Result type, incorporating also
     * the optional artifact object **/
    using ResultExt = std::pair<Result, std::optional<ArtifactType>>;

  protected:
    std::optional<ArtifactType> artifact;

    Result rawRun(const nlohmann::json& resultsSoFar);
    virtual ResultExt
        rawRunWithArtifact(const nlohmann::json& resultsSoFar) = 0;

  public:
    /** @brief All arguments are passed as-is to the @c Test constructor, which
     * see **/
    ArtifactTest(const std::string& type, const std::string& instance,
                 const std::vector<std::shared_ptr<Test>>& dependencies);

    /**
     * @brief Return @c true if the optional field @c artifact is non-null
     *
     * The artifact can be empty either because the test hasn't been run yet or
     * because the 'rawRunWithArtifact(..)' didn't return any, eg. in the case
     * of test failing.
     */
    bool hasArtifact() const;

    /**
     * @brief Return the computed artifact
     *
     * If 'hasArtifact()' returns @c false this function will raise a runtime
     * error. This means, in particular, this function will always throw
     * exception when called on testing object that hasn't been run yet
     * ('isPerformed()' returning @c false).
     */
    const ArtifactType& getArtifact() const;
};

/**
 * @brief Test whether the given file can be read as a valid nlohmann::json
 * object
 *
 * If successfult save the json object as the artifact.
 */
class JsonReadTest : public ArtifactTest<nlohmann::ordered_json>
{
  public:
    std::string getDescription() const;
    ResultExt rawRunWithArtifact(const nlohmann::json& resultsSoFar);
    static std::shared_ptr<JsonReadTest>
        create(const std::string& jsonFileName);

  private:
    std::string jsonFileName;

    JsonReadTest(const std::string& jsonFileName);
};

/**
 * @brief Test if the given json element matches the provided schema
 */
class JsonSchemaTest : public Test, public Dependency<JsonReadTest>
{
  public:
    std::string getDescription() const;
    Result rawRun(const nlohmann::json& resultsSoFar);
    static std::shared_ptr<JsonSchemaTest>
        create(const std::string& instanceName,
               std::shared_ptr<json_schema::JsonSchema> schema,
               std::shared_ptr<JsonReadTest> jsonReadTest);

  protected:
    std::shared_ptr<json_schema::JsonSchema> schema;

    JsonSchemaTest(const std::string& instanceName,
                   std::shared_ptr<json_schema::JsonSchema> schema,
                   std::shared_ptr<JsonReadTest> jsonReadTest);
    // The function will be called by 'rawRun' if and only if the json object
    // held by 'jsonReadTest' conforms to the 'schema'.
    virtual std::vector<std::shared_ptr<Test>> newTests() const;
};

using DatType = std::map<std::string, dat_traverse::Device>;

/**
 * @brief Test whether the given json element parses as the @c DatType memory
 * object
 *
 * If successful save this object as the artifact.
 */
class DatParseTest :
    public ArtifactTest<DatType>,
    public Dependency<JsonSchemaTest>
{
  public:
    std::string getDescription() const;
    ResultExt rawRunWithArtifact(const nlohmann::json& resultsSoFar);
    static std::shared_ptr<DatParseTest>
        create(std::shared_ptr<JsonSchemaTest> datJsonSchemaTestDep);

  private:
    DatParseTest(std::shared_ptr<JsonSchemaTest> datJsonSchemaTestDep);
};

/**
 * @brief Test if the json object representing event info is internally
 * consistent
 *
 * Some criteria for inner consistency:
 *
 * - Proper use of brackets in all the fields (correct syntax, domains
 * consistent with each other)
 *
 * - Correctly formed "message_args" attribute (number of accessors matching the
 * number of placeholders in the patterns)
 */
class EventInfoInnerConsistencyTest :
    public Test,
    public Dependency<JsonSchemaTest>
{
  public:
    std::string getDescription() const;
    Test::Result rawRun(const nlohmann::json& resultsSoFar);
    static std::shared_ptr<EventInfoInnerConsistencyTest>
        create(std::shared_ptr<JsonSchemaTest> eventInfoJsonSchemaTest);

  private:
    EventInfoInnerConsistencyTest(
        std::shared_ptr<JsonSchemaTest> eventInfoJsonSchemaTest);
};

/**
 * @brief Test whether the given json element parses as the @c
 * event_info::EventMap memory object
 *
 * If successful save this object as the artifact.
 */
class EventInfoParseTest :
    public ArtifactTest<event_info::EventMap>,
    public Dependency<EventInfoInnerConsistencyTest>
{
  public:
    std::string getDescription() const;
    ResultExt rawRunWithArtifact(const nlohmann::json& resultsSoFar);
    static std::shared_ptr<EventInfoParseTest>
        create(std::shared_ptr<EventInfoInnerConsistencyTest>
                   eventInfoInnerConsistencyTest);

  private:
    EventInfoParseTest(std::shared_ptr<EventInfoInnerConsistencyTest>
                           eventInfoInnerConsistencyTest);
};

/**
 * @brief Check if the DAT and event info are consistent with each other
 *
 * Examples of consistency checks:
 *
 * - Whether all the device references in event info (fields "device_type",
 * "origin_of_conditon") can be, after bracket expansion, found in DAT.
 *
 * - Whether the fixed origin of condition, if given, could also be reached by
 * the dynamic OOC resolving algorithm, given event's category (or lack thereof)
 */
class EventInfoDatInterConsistencyTest :
    public Test,
    public Dependency<DatParseTest>,
    public Dependency<EventInfoParseTest>
{
  public:
    std::string getDescription() const;
    Test::Result rawRun(const nlohmann::json& resultsSoFar);
    static std::shared_ptr<EventInfoDatInterConsistencyTest>
        create(std::shared_ptr<DatParseTest> datParseTest,
               std::shared_ptr<EventInfoParseTest> eventInfoParseTest);

  private:
    EventInfoDatInterConsistencyTest(
        std::shared_ptr<DatParseTest> datParseTest,
        std::shared_ptr<EventInfoParseTest> eventInfoParseTest);
};

/**
 * @brief Produce a list of all possible origins of conditions
 *
 * The list is obtained for each event and each device it occurs on.
 */
class PossibleOriginsOfConditionTest :
    public Test,
    public Dependency<EventInfoDatInterConsistencyTest>
{
  public:
    std::string getDescription() const;
    Test::Result rawRun(const nlohmann::json& resultsSoFar);
    static std::shared_ptr<PossibleOriginsOfConditionTest>
        create(std::shared_ptr<EventInfoDatInterConsistencyTest>
                   eventInfoDatInterConsistencyTest);

  private:
    PossibleOriginsOfConditionTest(
        std::shared_ptr<EventInfoDatInterConsistencyTest>
            eventInfoDatInterConsistencyTest);
};

/**
 * @brief Main function carrying out all the tests
 *
 * The function puts all is output into two streams. One is for structured test
 * results in the form of json object (@c reportStream). This will usually be a
 * standard output or a file. The json object is written in batch, only after
 * all the tests are finished. For details about the format see the description
 * of the diagnostics namespace.
 *
 * The other stream is for loging and progress tracking purposes. This will
 * usually be some console-visible stream like stdout or stderr. Information
 * about the currently performed test and the status of all the finished ones
 * is included. Example:
 *
 * @code
 * Check json format with its schema: DAT... PASSED
 * Parse DAT json to native C++ object: Device::populateMap... PASSED
 * Json file read: event_info.json... FAILED
 * Check json format with its schema: event info... SKIPPED
 * @endcode
 *
 * @param[in] datFile Path to a json file containg DAT definition
 *
 * @param[in] eventInfoFile Path to a json file containing event nodes
 * definition
 *
 * @param[out] reportStream The stream into which the resulting json object with
 * structured test results will be put.
 *
 * @param[out] commentStream The stream into which the progress tracking
 * messages will be put
 *
 * @return Zero if finished with success, non-zero otherwise (WIP)
 */
int run(const std::string& datFile, const std::string& eventInfoFile,
        std::ostream& reportStream = std::cout,
        std::ostream& commentsStream = std::cerr);

} // namespace diagnostics
