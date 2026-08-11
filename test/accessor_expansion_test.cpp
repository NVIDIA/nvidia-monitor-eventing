/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES.
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

/*
 * Data-driven regression guard over every accessor declared in a shipping
 * event_info.json.
 *
 * An accessor whose device-bearing field carries a range must resolve to a
 * concrete device for each member of its event's device_type. When it does
 * not, the unresolved pattern is handed verbatim to the consumer (a command
 * line, a D-Bus object path) and the resulting failure is silent: the command
 * cannot find the device and the event is either dropped or reported for the
 * wrong reason.
 *
 * The fixture is distilled from meta-vr-nvl-hmc's event_info.json. Point
 * EVENT_INFO_ACCESSORS at another distilled file to run the same checks
 * against a different platform.
 */

#include "device_id.hpp"
#include "log.hpp"
#include "util.hpp"

#include <nlohmann/json.hpp>

#include <cstdlib>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;
using json = nlohmann::json;

namespace
{

/** Keys whose value names a device and may therefore carry a range. */
const std::vector<std::string> deviceBearingKeys = {"arguments", "object"};

} // namespace

class AccessorExpansionTest : public Test
{
  protected:
    static void SetUpTestSuite()
    {
        log_set_level(0);
    }

    static const json& fixture()
    {
        static json doc = load();
        return doc;
    }

    static void collectFields(const json& accessor,
                              std::vector<std::string>& out)
    {
        if (!accessor.is_object())
        {
            return;
        }
        for (const auto& key : deviceBearingKeys)
        {
            if (accessor.contains(key) && accessor[key].is_string())
            {
                out.push_back(accessor[key].get<std::string>());
            }
        }
    }

    /** Fields resolved by substituting the event's own device index.
     *
     * DataAccessor::runCommandLine() and readDbus() both call
     * introduceDeviceInObjectpath() with the index of the device being
     * handled, so a range here must resolve against that index or the
     * unsubstituted pattern reaches the command line / D-Bus call.
     */
    static std::vector<std::string> indexResolvedFields(const json& event)
    {
        std::vector<std::string> out;
        for (const auto& single : {"accessor", "event_counter_reset"})
        {
            collectFields(event.value(single, json::object()), out);
        }
        for (const auto& listed : {"recovery", "parameters", "telemetries"})
        {
            for (const auto& entry : event.value(listed, json::array()))
            {
                collectFields(entry, out);
            }
        }
        return out;
    }

    /** event_trigger is matched against an incoming signal to *derive* the
     * index rather than being read with it, so it is legitimately allowed to
     * address a coarser device than device_type. It is checked for
     * well-formedness instead of substitution.
     */
    static std::vector<std::string> triggerFields(const json& event)
    {
        std::vector<std::string> out;
        collectFields(event.value("event_trigger", json::object()), out);
        return out;
    }

  private:
    static json load()
    {
        const char* override = std::getenv("EVENT_INFO_ACCESSORS");
        std::string path = override ? std::string(override)
                                    : std::string(TEST_DATA_DIR) +
                                          "/event_info_accessors_fixture.json";
        // Fail fast: continuing into the parse on a bad open raises a second,
        // less informative exception that buries the real cause.
        std::ifstream file(path);
        if (!file.good())
        {
            ADD_FAILURE() << "cannot open accessor fixture: " << path;
            return json{{"events", json::array()}};
        }

        json doc;
        try
        {
            file >> doc;
        }
        catch (const json::parse_error& e)
        {
            ADD_FAILURE() << "invalid accessor fixture: " << path << " ("
                          << e.what() << ")";
            return json{{"events", json::array()}};
        }
        return doc;
    }
};

// Every accessor of every event must resolve to a concrete device for each
// member of that event's device_type.
TEST_F(AccessorExpansionTest, EveryAccessorResolvesForEveryDevice)
{
    unsigned checked = 0;
    unsigned events = 0;

    for (const auto& event : fixture()["events"])
    {
        const auto errorId = event["error_id"].get<std::string>();
        const auto deviceType = event["device_type"].get<std::string>();
        const auto fields = indexResolvedFields(event);
        if (fields.empty())
        {
            continue;
        }
        ++events;

        device_id::DeviceIdPattern pattern(deviceType);
        auto indexes = pattern.domainVec();
        if (indexes.empty())
        {
            indexes.push_back(device_id::PatternIndex());
        }

        for (const auto& index : indexes)
        {
            const std::string device =
                pattern.dim() > 0 ? pattern.eval(index) : deviceType;

            for (const auto& field : fields)
            {
                SCOPED_TRACE(
                    errorId + " device=" + device + " field='" + field + "'");
                const std::string expanded =
                    util::introduceDeviceInObjectpath(field, index);

                EXPECT_FALSE(util::existsRange(expanded))
                    << "unresolved range: '" << field << "' -> '" << expanded
                    << "'";
                ++checked;
            }
        }
    }

    EXPECT_GT(events, 0u) << "fixture contained no accessors";
    EXPECT_GT(checked, 0u);
    std::cout << "[          ] checked " << checked
              << " accessor expansions across " << events << " events\n";
}

// event_trigger patterns are resolved by matching an incoming signal, so they
// are not required to substitute against the event's index. They must still be
// well-formed and no finer-grained than the device_type they belong to -- a
// trigger with more dimensions than its event could never be matched.
TEST_F(AccessorExpansionTest, EveryTriggerPatternIsWellFormed)
{
    unsigned checked = 0;

    for (const auto& event : fixture()["events"])
    {
        const auto errorId = event["error_id"].get<std::string>();
        const auto deviceType = event["device_type"].get<std::string>();
        device_id::DeviceIdPattern devicePattern(deviceType);

        for (const auto& field : triggerFields(event))
        {
            SCOPED_TRACE(errorId + " trigger='" + field + "'");
            device_id::DeviceIdPattern triggerPattern(field);

            EXPECT_LE(triggerPattern.dim(), devicePattern.dim())
                << "trigger is finer-grained than its device_type";
            if (triggerPattern.dim() > 0)
            {
                EXPECT_FALSE(triggerPattern.domainVec().empty())
                    << "trigger pattern has an empty domain";
            }
            ++checked;
        }
    }

    EXPECT_GT(checked, 0u) << "fixture contained no event_trigger accessors";
    std::cout << "[          ] checked " << checked << " trigger patterns\n";
}

// The qualifier path specifically: standaloneQualifierPassed() builds its
// CheckAccessor from the device_type pattern plus the instantiated index.
// Every standalone-only event must resolve under exactly that construction.
TEST_F(AccessorExpansionTest, StandaloneQualifierResolvesEveryRangedAccessor)
{
    unsigned checked = 0;

    for (const auto& event : fixture()["events"])
    {
        if (!event.value("standalone", false))
        {
            continue;
        }
        const auto accessor = event.value("accessor", json::object());
        if (!accessor.is_object() || !accessor.contains("arguments"))
        {
            continue;
        }

        const auto errorId = event["error_id"].get<std::string>();
        const auto deviceType = event["device_type"].get<std::string>();
        const auto arguments = accessor["arguments"].get<std::string>();

        device_id::DeviceIdPattern pattern(deviceType);
        for (const auto& index : pattern.domainVec())
        {
            // Mirrors what standaloneQualifierPassed() now constructs.
            util::DeviceIdData devIdData(deviceType, index);
            SCOPED_TRACE(errorId + " device=" + pattern.eval(index));

            const std::string expanded =
                util::introduceDeviceInObjectpath(arguments, devIdData.index);
            EXPECT_FALSE(util::existsRange(expanded))
                << "qualifier would exec: '" << expanded << "'";
            ++checked;
        }
    }

    EXPECT_GT(checked, 0u) << "no standalone ranged accessors in fixture";
}

// Pins the reason the qualifier takes device_type + index rather than the
// concrete device name: a name alone carries no index, so a ranged accessor
// is handed to the command verbatim.
TEST_F(AccessorExpansionTest, DeviceNameOnlyConstructionCannotResolveRanges)
{
    util::DeviceIdData fromName("ProcessorModule_0");
    EXPECT_EQ(fromName.index.dim(), 0u);

    const std::string ranged = "GPU_SMA_[0-1] 128 reason";
    EXPECT_TRUE(util::existsRange(
        util::introduceDeviceInObjectpath(ranged, fromName.index)))
        << "a rangeless index unexpectedly resolved the pattern";

    util::DeviceIdData fromType(
        "ProcessorModule_[0-1]",
        device_id::DeviceIdPattern("ProcessorModule_[0-1]")
            .match("ProcessorModule_0")
            .at(0));
    EXPECT_EQ(fromType.index.dim(), 1u);
    EXPECT_EQ(util::introduceDeviceInObjectpath(ranged, fromType.index),
              "GPU_SMA_0 128 reason");
}

// Guards the fixture itself: if a regeneration drops a whole category, the
// coverage tests above would still pass while silently checking less.
TEST_F(AccessorExpansionTest, FixtureCoversEveryAccessorCategory)
{
    std::map<std::string, unsigned> seen;
    for (const auto& event : fixture()["events"])
    {
        for (const auto& single :
             {"accessor", "event_trigger", "event_counter_reset"})
        {
            const auto& block = event.value(single, json::object());
            if (block.is_object() && block.contains("type") &&
                !block["type"].get<std::string>().empty())
            {
                seen[single]++;
            }
        }
        for (const auto& listed : {"recovery", "parameters", "telemetries"})
        {
            for (const auto& entry : event.value(listed, json::array()))
            {
                if (entry.is_object() && entry.contains("type"))
                {
                    seen[listed]++;
                }
            }
        }
    }

    for (const auto& category :
         {"accessor", "event_trigger", "recovery", "parameters", "telemetries"})
    {
        EXPECT_GT(seen[category], 0u)
            << "fixture has no '" << category << "' accessors";
    }

    unsigned total = 0;
    for (const auto& [name, count] : seen)
    {
        std::cout << "[          ] " << name << ": " << count << "\n";
        total += count;
    }
    std::cout << "[          ] total accessor blocks: " << total << "\n";
}

// Each device resolves to its own value; nothing collapses to index 0.
TEST_F(AccessorExpansionTest, EachDeviceResolvesDistinctly)
{
    device_id::DeviceIdPattern deviceType("ProcessorModule_[0-1]");
    const std::string ranged = "GPU_SMA_[0-1] 192 reason";

    EXPECT_EQ(util::introduceDeviceInObjectpath(
                  ranged, deviceType.match("ProcessorModule_0").at(0)),
              "GPU_SMA_0 192 reason");
    EXPECT_EQ(util::introduceDeviceInObjectpath(
                  ranged, deviceType.match("ProcessorModule_1").at(0)),
              "GPU_SMA_1 192 reason");
}
