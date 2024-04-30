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

    if (dat.count("GPU0") > 0)
    {
        EXPECT_EQ(dat.at("GPU0").healthStatus.triState, "Active");
    }
    else
    {
        std::cerr << "Error: deviceName:"
                  << "GPU0"
                  << "is an invalid key!" << std::endl;
        return;
    }

    datTraverser.parentTraverse(dat, hsc8.name, datTraverser.hasParents,
                                parentCallbacks);

    if (dat.count("Retimer0") > 0)
    {
        EXPECT_EQ(dat.at("Retimer0").healthStatus.healthRollup, "Critical");
    }
    else
    {
        std::cerr << "Error: deviceName:"
                  << "Retimer0"
                  << "is an invalid key!" << std::endl;
        return;
    }

    if (dat.count("GPU0") > 0)
    {
        EXPECT_EQ(dat.at("GPU0").healthStatus.triState, "Error");
        EXPECT_EQ(dat.at("GPU0").healthStatus.originOfCondition, hsc8.name);
    }
    else
    {
        std::cerr << "Error: deviceName:"
                  << "GPU0"
                  << "is an invalid key!" << std::endl;
        return;
    }

    if (dat.count("Retimer0") > 0)
    {
        EXPECT_EQ(dat.at("Retimer0").healthStatus.originOfCondition, hsc8.name);
        EXPECT_EQ(dat.at("Retimer0").healthStatus.health, "OK");
    }
    else
    {
        std::cerr << "Error: deviceName:"
                  << "Retimer0"
                  << "is an invalid key!" << std::endl;
        return;
    }

    if (dat.count("HSC8") > 0)
    {
        EXPECT_EQ(dat.at("HSC8").healthStatus.health, "Critical");
    }
    else
    {
        std::cerr << "Error: deviceName:"
                  << "HSC8"
                  << "is an invalid key!" << std::endl;
    }
}

TEST(DatTraverseTest, gettingAssociations)
{
    /*  GPU0 -> HSC0, GPU0-ERot, Retimer0
        HSC0 -> none
        GPU0-ERot -> none
        Retimer0 -> HSC8
        HSC8 -> none        */
    nlohmann::json j;
    j["power_rail"] = nlohmann::json::array();
    j["erot_control"] = nlohmann::json::array();
    j["pin_status"] = nlohmann::json::array();
    j["interface_status"] = nlohmann::json::array();
    j["firmware_status"] = nlohmann::json::array();
    j["protocol_status"] = nlohmann::json::array();

    nlohmann::json jDevTp;
    jDevTp["name"] = "";
    jDevTp["expected_value"] = "pass";
    jDevTp["accessor"]["type"] = "DEVICE";
    jDevTp["accessor"]["device_name"] = "";

    nlohmann::json jgpu0 = j;
    jgpu0["name"] = "GPU0";
    jgpu0["association"] = {"HSC0", "GPU0-ERoT", "Retimer0"};

    jDevTp["name"] = "1";
    jDevTp["accessor"]["device_name"] = "HSC0";
    jgpu0["power_rail"] += jDevTp;

    jDevTp["name"] = "2";
    jDevTp["accessor"]["device_name"] = "GPU0-ERoT";
    jgpu0["power_rail"] += jDevTp;

    jDevTp["name"] = "3";
    jDevTp["accessor"]["device_name"] = "Retimer0";
    jgpu0["power_rail"] += jDevTp;
    dat_traverse::Device gpu0("GPU0", jgpu0);

    nlohmann::json jhsc0 = j;
    jhsc0["name"] = "HSC0";
    jhsc0["association"] = nlohmann::json::array();
    dat_traverse::Device hsc0("HSC0", jhsc0);

    nlohmann::json jgpu0erot = j;
    jgpu0erot["name"] = "GPU0-ERoT";
    jgpu0erot["association"] = nlohmann::json::array();
    dat_traverse::Device gpu0erot("GPU0-ERoT", jgpu0erot);

    nlohmann::json jretimer0 = j;
    jretimer0["name"] = "Retimer0";
    jretimer0["association"] = {"HSC8"};
    jDevTp["name"] = "1";
    jDevTp["accessor"]["device_name"] = "HSC8";
    jretimer0["power_rail"] += jDevTp;
    dat_traverse::Device retimer0("Retimer0", jretimer0);

    nlohmann::json jhsc8 = j;
    jhsc8["name"] = "HSC8";
    jhsc8["association"] = nlohmann::json::array();
    dat_traverse::Device hsc8("HSC8", jhsc8);

    std::map<std::string, dat_traverse::Device> dat;
    dat.insert(std::pair<std::string, dat_traverse::Device>(gpu0.name, gpu0));
    dat.insert(std::pair<std::string, dat_traverse::Device>(hsc0.name, hsc0));
    dat.insert(
        std::pair<std::string, dat_traverse::Device>(gpu0erot.name, gpu0erot));
    dat.insert(
        std::pair<std::string, dat_traverse::Device>(retimer0.name, retimer0));
    dat.insert(std::pair<std::string, dat_traverse::Device>(hsc8.name, hsc8));

    event_handler::DATTraverse datTraverser("DatTraverser1");

    std::vector<std::string> childVecAssoc =
        datTraverser.getSubAssociations(dat, gpu0.name);

    EXPECT_EQ(childVecAssoc.size(), 5);
    EXPECT_EQ(childVecAssoc[0], gpu0.name);
    EXPECT_EQ(childVecAssoc[1], hsc0.name);
    EXPECT_EQ(childVecAssoc[2], gpu0erot.name);
    EXPECT_EQ(childVecAssoc[3], retimer0.name);
    EXPECT_EQ(childVecAssoc[4], hsc8.name);

    std::vector<std::string> childVecTps =
        datTraverser.getSubAssociations(dat, gpu0.name, true);

    EXPECT_EQ(childVecTps.size(), 5);
    EXPECT_EQ(childVecTps[0], gpu0.name);
    EXPECT_EQ(childVecTps[1], hsc0.name);
    EXPECT_EQ(childVecTps[2], gpu0erot.name);
    EXPECT_EQ(childVecTps[3], retimer0.name);
    EXPECT_EQ(childVecTps[4], hsc8.name);
}

TEST(DatTraverseTest, testTypeAndCount)
{
    /*  GPU0 no TP type regular
        GPU1 5 TP type regular
        GPU2 no type
        GPU3_NVLINK_2 type port
    */
    nlohmann::json j;
    j["power_rail"] = nlohmann::json::array();
    j["erot_control"] = nlohmann::json::array();
    j["pin_status"] = nlohmann::json::array();
    j["interface_status"] = nlohmann::json::array();
    j["firmware_status"] = nlohmann::json::array();
    j["protocol_status"] = nlohmann::json::array();

    nlohmann::json jDevTp;
    jDevTp["name"] = "DUMMY";
    jDevTp["expected_value"] = "pass";
    jDevTp["accessor"]["type"] = "DEVICE";
    jDevTp["accessor"]["device_name"] = "DUMMY";

    /* gpu0 no tp configured type regular */
    nlohmann::json jgpu0 = j;
    jgpu0["name"] = "GPU0";
    jgpu0["type"] = "regular";
    jgpu0["association"] = nlohmann::json::array();
    dat_traverse::Device gpu0("GPU0", jgpu0);

    /* gpu1 3 tp configured type regular */
    nlohmann::json jgpu1 = j;
    jgpu1["name"] = "GPU1";
    jgpu1["type"] = "regular";
    jgpu1["association"] = nlohmann::json::array();

    jDevTp["name"] = "DUMMY_NAME1";
    jgpu1["power_rail"] += jDevTp;
    jDevTp["name"] = "DUMMY_NAME2";
    jgpu1["interface_status"] += jDevTp;
    jDevTp["name"] = "DUMMY_NAME3";
    jgpu1["protocol_status"] += jDevTp;
    dat_traverse::Device gpu1("GPU1", jgpu1);

    /* gpu2 no type */
    nlohmann::json jgpu2 = j;
    jgpu2["name"] = "GPU2";
    // jgpu2["type"] = "regular";
    jgpu2["association"] = nlohmann::json::array();
    dat_traverse::Device gpu2("GPU2", jgpu2);

    /* gpu3 type port */
    nlohmann::json jgpu3 = j;
    jgpu3["name"] = "GPU_SXM_1/NVLink_0";
    jgpu3["type"] = "port";
    jgpu3["association"] = nlohmann::json::array();
    dat_traverse::Device gpu3("GPU3", jgpu3);

    EXPECT_EQ(gpu0.hasTestpoints(), false);
    EXPECT_EQ(gpu1.hasTestpoints(), true);
    EXPECT_EQ(gpu2.hasTestpoints(), false);
    EXPECT_EQ(gpu3.hasTestpoints(), false);
    EXPECT_EQ(gpu0.getType(), dat_traverse::DeviceType::types::REGULAR);
    EXPECT_EQ(gpu1.getType(), dat_traverse::DeviceType::types::REGULAR);
    EXPECT_EQ(gpu2.getType(), dat_traverse::DeviceType::types::UNKNOWN_TYPE);
    EXPECT_EQ(gpu3.getType(), dat_traverse::DeviceType::types::PORT);
}

TEST(DatTraverseTest, testLoadSeverity)
{
    /* test loading of severity + throwing on unknown severity + default when
     * none specified
     */
    nlohmann::json j;
    j["power_rail"] = nlohmann::json::array();
    j["erot_control"] = nlohmann::json::array();
    j["pin_status"] = nlohmann::json::array();
    j["interface_status"] = nlohmann::json::array();
    j["firmware_status"] = nlohmann::json::array();
    j["protocol_status"] = nlohmann::json::array();

    nlohmann::json jDevTp;
    jDevTp["name"] = "DUMMY";
    jDevTp["expected_value"] = "pass";
    jDevTp["accessor"]["type"] = "DEVICE";
    jDevTp["accessor"]["device_name"] = "DUMMY";

    nlohmann::json jgpu1 = j;
    jgpu1["name"] = "GPU1";
    jgpu1["type"] = "regular";
    jgpu1["association"] = nlohmann::json::array();
    jDevTp["name"] = "DUMMY_NAME1";
    jgpu1["power_rail"] += jDevTp;
    jDevTp["name"] = "DUMMY_NAME2";
    jDevTp["severity"] = "Warning";
    jgpu1["power_rail"] += jDevTp;
    jDevTp["name"] = "DUMMY_NAME3";
    jDevTp["severity"] = "Critical";
    jgpu1["power_rail"] += jDevTp;
    dat_traverse::Device gpu1("GPU1", jgpu1);
    auto& tps = gpu1.test["power_rail"].testPoints;
    EXPECT_EQ(tps["DUMMY_NAME1"].severity.string(), "Critical");
    EXPECT_EQ(tps["DUMMY_NAME2"].severity.string(), "Warning");
    EXPECT_EQ(tps["DUMMY_NAME3"].severity.string(), "Critical");

    jDevTp["severity"] = "unknown severity expecting to throw";
    jgpu1["protocol_status"] += jDevTp;
    EXPECT_ANY_THROW(dat_traverse::Device gpu_throw("GPUthrow", jgpu1););
}

TEST(DatTraverseTest, testFindMaxSeverity)
{
    using namespace std::string_literals;
    EXPECT_EQ("Critical", util::Severity::findMaxSeverity({
                              "Critical"s, "OK"s, "Warning"s}));
    EXPECT_EQ("Critical"s, util::Severity::findMaxSeverity({
                              "Critical"s, "Critical"s, "Critical"s}));
    EXPECT_EQ("Critical"s, util::Severity::findMaxSeverity({
                              "Critical"s, "Critical"s, "Warning"s}));
    EXPECT_EQ("Critical"s, util::Severity::findMaxSeverity({
                              "Critical"s, "Warning"s, "OK"s}));
    EXPECT_EQ("Critical"s, util::Severity::findMaxSeverity({
                              "Critical"s, "Warning"s, "Warning"s}));
    EXPECT_EQ("Critical"s, util::Severity::findMaxSeverity({
                              "Critical"s, "OK"s, "Warning"s}));
    EXPECT_EQ("Critical"s, util::Severity::findMaxSeverity({
                              "Critical"s, "OK"s, "OK"s}));
    EXPECT_EQ("Critical"s, util::Severity::findMaxSeverity({
                              "Warning"s, "Critical"s, "Warning"s}));
    EXPECT_EQ("Critical"s, util::Severity::findMaxSeverity({
                              "Warning"s, "Critical"s, "OK"s}));
    EXPECT_EQ("Critical"s, util::Severity::findMaxSeverity({
                              "Warning"s, "Warning"s, "Critical"s}));
    EXPECT_EQ("Critical"s, util::Severity::findMaxSeverity({
                              "Warning"s, "OK"s, "Critical"s}));
    EXPECT_EQ("Critical"s, util::Severity::findMaxSeverity({
                              "Warning"s, "Warning"s, "Critical"s}));
    EXPECT_EQ("Warning"s, util::Severity::findMaxSeverity({
                             "OK"s, "Warning"s, "Warning"s}));
    EXPECT_EQ("Warning"s, util::Severity::findMaxSeverity({
                             "OK"s, "Warning"s, "OK"s}));
    EXPECT_EQ("Warning"s, util::Severity::findMaxSeverity({
                            "OK"s, "OK"s, "Warning"s}));
    EXPECT_EQ("Warning"s, util::Severity::findMaxSeverity({
                             "Warning"s, "Warning"s, "Warning"s}));
    EXPECT_EQ("Warning"s, util::Severity::findMaxSeverity({
                             "Warning"s, "Warning"s, "OK"s}));
    EXPECT_EQ("Warning"s, util::Severity::findMaxSeverity({
                             "Warning"s, "OK"s, "Warning"s}));
    EXPECT_EQ("OK"s, util::Severity::findMaxSeverity({
                             "OK"s, "OK"s, "OK"s}));
}
