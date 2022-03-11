#include "dat_traverse.hpp"
#include "nlohmann/json.hpp"

#include <exception>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "algorithm"

#include "gmock/gmock.h"

TEST(DatTraverseTest, FullTraversal)
{
    nlohmann::json j;
    j["name"] = "GPU0";
    j["association"] = {"Retimer0"};
    j["power_rail"] = nlohmann::json::array();
    j["erot_control"] = nlohmann::json::array();
    j["pin_status"] = nlohmann::json::array();
    j["interface_status"] = nlohmann::json::array();
    j["firmware_status"] = nlohmann::json::array();
    j["protocol_status"] = nlohmann::json::array();
    dat_traverse::Device gpu0("GPU0", j);

    nlohmann::json j2;
    j2["name"] = "Retimer0";
    j2["association"] = {"HSC8"};
    j2["power_rail"] = nlohmann::json::array();
    j2["erot_control"] = nlohmann::json::array();
    j2["pin_status"] = nlohmann::json::array();
    j2["interface_status"] = nlohmann::json::array();
    j2["firmware_status"] = nlohmann::json::array();
    j2["protocol_status"] = nlohmann::json::array();
    dat_traverse::Device retimer0("Retimer0", j2);
    retimer0.parents.push_back("GPU0");

    nlohmann::json j3;
    j3["name"] = "HSC8";
    j3["association"] = nlohmann::json::array();
    j3["power_rail"] = nlohmann::json::array();
    j3["erot_control"] = nlohmann::json::array();
    j3["pin_status"] = nlohmann::json::array();
    j3["interface_status"] = nlohmann::json::array();
    j3["firmware_status"] = nlohmann::json::array();
    j3["protocol_status"] = nlohmann::json::array();
    dat_traverse::Device hsc8("HSC8", j3);
    hsc8.parents.push_back("Retimer0");
    hsc8.healthStatus.health = "Critical";
    hsc8.healthStatus.triState = "Error";

    std::map<std::string, dat_traverse::Device> dat;
    dat.insert(std::pair<std::string, dat_traverse::Device>(gpu0.name, gpu0));
    dat.insert(
        std::pair<std::string, dat_traverse::Device>(retimer0.name, retimer0));
    dat.insert(std::pair<std::string, dat_traverse::Device>(hsc8.name, hsc8));

    event_handler::DATTraverse datTraverser("DatTraverser1");

    std::vector<std::function<void(dat_traverse::Device & device,
                                   const dat_traverse::Status& status)>>
        parentCallbacks;
    parentCallbacks.push_back(datTraverser.setHealthProperties);
    parentCallbacks.push_back(datTraverser.setOriginOfCondition);

    EXPECT_EQ(dat.at("GPU0").healthStatus.triState, "Active");

    datTraverser.parentTraverse(dat, hsc8.name, datTraverser.hasParents,
                                parentCallbacks);

    EXPECT_EQ(dat.at("Retimer0").healthStatus.healthRollup, "Critical");
    EXPECT_EQ(dat.at("GPU0").healthStatus.triState, "Error");
    EXPECT_EQ(dat.at("Retimer0").healthStatus.originOfCondition, hsc8.name);
    EXPECT_EQ(dat.at("GPU0").healthStatus.originOfCondition, hsc8.name);
    EXPECT_EQ(dat.at("Retimer0").healthStatus.health, "OK");
    EXPECT_EQ(dat.at("HSC8").healthStatus.health, "Critical");
}