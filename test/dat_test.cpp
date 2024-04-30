/**
 * Copyright (c) 2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#include "dat_traverse.hpp"
#include "nlohmann/json.hpp"

#include <exception>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "algorithm"

#include "gmock/gmock.h"

TEST(DatTest, AssociationPopulated)
{
    nlohmann::json j;
    j["association"] = {"GPU1", "GPU2"};
    j["power_rail"] = nlohmann::json::array();
    j["erot_control"] = nlohmann::json::array();
    j["pin_status"] = nlohmann::json::array();
    j["interface_status"] = nlohmann::json::array();
    j["firmware_status"] = nlohmann::json::array();
    j["protocol_status"] = nlohmann::json::array();
    dat_traverse::Device device("Baseboard", j);
    EXPECT_EQ(device.association.size(), 2);
}

TEST(DatTest, NamePopulated)
{
    nlohmann::json j;
    j["association"] = {"GPU1", "GPU2"};
    j["power_rail"] = nlohmann::json::array();
    j["erot_control"] = nlohmann::json::array();
    j["pin_status"] = nlohmann::json::array();
    j["interface_status"] = nlohmann::json::array();
    j["firmware_status"] = nlohmann::json::array();
    j["protocol_status"] = nlohmann::json::array();
    dat_traverse::Device device("Baseboard", j);
    EXPECT_EQ(device.name, "Baseboard");
}

TEST(DatTest, MapPopulated)
{
    nlohmann::json j;
    j["Baseboard"]["association"] = {"GPU1", "GPU2"};
    j["GPU1"]["association"] = {"HSC1", "HSC2"};

    std::vector<std::string> v =
        j["Baseboard"]["association"].get<std::vector<std::string>>();
    EXPECT_EQ(std::find(v.begin(), v.end(), "GPU1") != v.end(), true);
}

TEST(DatTest, optObjTest)
{
    nlohmann::json j;
    j["association"] = {"GPU1", "GPU2"};
    j["power_rail"] = nlohmann::json::array();
    j["erot_control"] = nlohmann::json::array();
    j["pin_status"] = nlohmann::json::array();
    j["interface_status"] = nlohmann::json::array();
    j["firmware_status"] = nlohmann::json::array();
    j["protocol_status"] = nlohmann::json::array();
    dat_traverse::Device device_without_opt("Baseboard", j);
    j["dbus_objects"]["primary"] = "primary_dbus_path";
    j["dbus_objects"]["ooc_context"] = "OOC_dbus_path";
    j["dbus_set_health"] = false;
    dat_traverse::Device device_with_opt("Baseboard", j);

    EXPECT_EQ(device_without_opt.name, "Baseboard");
    EXPECT_EQ(device_without_opt.getDbusObjectOocSpecificExplicit(),
              std::nullopt);
    EXPECT_EQ(device_without_opt.getDbusObjectOocSpecificExplicit(),
              std::nullopt);
    EXPECT_EQ(device_without_opt.canSetHealthOnDbus(), true);

    EXPECT_EQ(device_with_opt.getDbusObjectPrimaryExplicit(),
              std::string{"primary_dbus_path"});
    EXPECT_EQ(device_with_opt.getDbusObjectOocSpecificExplicit(),
              std::string{"OOC_dbus_path"});
    EXPECT_EQ(device_with_opt.canSetHealthOnDbus(), false);
}

#include <iostream>
TEST(DeviceType, Ctor)
{
    dat_traverse::DeviceType type_default{};
    EXPECT_EQ(type_default.get(),
              dat_traverse::DeviceType::types::UNKNOWN_TYPE);

    for (auto& val : dat_traverse::DeviceType::valuesAllowed)
    {
        std::cout << val.first << " " << val.second << "\r\n";
        nlohmann::json j;
        j["Baseboard"]["type"] = val.first;
        EXPECT_NO_THROW(dat_traverse::DeviceType type{j["Baseboard"]["type"]});
    }
    nlohmann::json j;
    j["Baseboard"]["type"] = "non_supported_type";
    EXPECT_ANY_THROW(dat_traverse::DeviceType type{j["Baseboard"]["type"]});
}
