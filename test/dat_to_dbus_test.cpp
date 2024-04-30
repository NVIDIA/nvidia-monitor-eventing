#include "eventing_main.hpp"
#include "dat_traverse.hpp"

#include <nlohmann/json.hpp>

#include <iostream>
#include <set>

#include "gmock/gmock.h"

using namespace nlohmann;
using namespace std;
using namespace testing;

json getStandardDatJson()
{
    return json::parse(
        "{"
        "\"Baseboard\" : { \"association\" : [\"GPU0\", \"GPU1\", \"GPU2\", \"GPU3\","
        "  \"GPU4\", \"GPU5\", \"GPU6\", \"GPU7\","
        "  \"NVSwitch0\", \"NVSwitch1\", \"NVSwitch2\", \"NVSwitch3\"],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] },"
        "\"GPU0\" : { \"association\" : [\"HSC0\", \"GPU0_ERoT\", \"PCIeRetimer0\"],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] },"
        "\"GPU1\" : { \"association\" : [\"HSC1\", \"GPU1_ERoT\", \"PCIeRetimer1\"],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] },"
        "\"GPU2\" : { \"association\" : [\"HSC2\", \"GPU2_ERoT\", \"PCIeRetimer2\"],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] },"
        "\"GPU3\" : { \"association\" : [\"HSC3\", \"GPU3_ERoT\", \"PCIeRetimer3\"],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] },"
        "\"GPU4\" : { \"association\" : [\"HSC4\", \"GPU4_ERoT\", \"PCIeRetimer4\"],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] },"
        "\"GPU5\" : { \"association\" : [\"HSC5\", \"GPU5_ERoT\", \"PCIeRetimer5\"],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] },"
        "\"GPU6\" : { \"association\" : [\"HSC6\", \"GPU6_ERoT\", \"PCIeRetimer6\"],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] },"
        "\"GPU7\" : { \"association\" : [\"HSC7\", \"GPU7_ERoT\", \"PCIeRetimer7\"],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] },"
        "\"NVSwitch0\" : { \"association\" : ["
        "  \"VR\", \"HSC8\", \"NVSwitch0_ERoT\", \"PCIeSwitch\"],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] },"
        "\"NVSwitch1\" : { \"association\" : ["
        "  \"VR\", \"HSC8\", \"NVSwitch1_ERoT\", \"PCIeSwitch\"],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] },"
        "\"NVSwitch2\" : { \"association\" : ["
        "  \"VR\", \"HSC9\", \"NVSwitch2_ERoT\", \"PCIeSwitch\"],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] },"
        "\"NVSwitch3\" : { \"association\" : ["
        "  \"VR\", \"HSC9\", \"NVSwitch3_ERoT\", \"PCIeSwitch\"],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] },"
        "\"PCIeRetimer0\" : { \"association\" : [\"HSC8\"],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] },"
        "\"PCIeRetimer1\" : { \"association\" : [\"HSC8\"],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] },"
        "\"PCIeRetimer2\" : { \"association\" : [\"HSC8\"],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] },"
        "\"PCIeRetimer3\" : { \"association\" : [\"HSC8\"],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] },"
        "\"PCIeRetimer4\" : { \"association\" : [\"HSC9\"],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] },"
        "\"PCIeRetimer5\" : { \"association\" : [\"HSC9\"],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] },"
        "\"PCIeRetimer6\" : { \"association\" : [\"HSC9\"],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] },"
        "\"PCIeRetimer7\" : { \"association\" : [\"HSC9\"],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] },"
        "\"PCIeSwitch\" : { \"association\" : [\"HSC_STBY\", \"PCIeSwitch_ERoT\"],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] },"
        "\"FPGA\" : { \"association\" : [\"HSC_STBY\", \"FPGA_ERoT\", \"HMC\"],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] },"
        "\"HMC\" : { \"association\" : [\"HSC_STBY\", \"HMC_ERoT\"],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] },"
        "\"GPU0_ERoT\" : { \"association\" : [],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] },"
        "\"GPU1_ERoT\" : { \"association\" : [],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] },"
        "\"GPU2_ERoT\" : { \"association\" : [],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] },"
        "\"GPU3_ERoT\" : { \"association\" : [],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] },"
        "\"GPU4_ERoT\" : { \"association\" : [],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] },"
        "\"GPU5_ERoT\" : { \"association\" : [],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] },"
        "\"GPU6_ERoT\" : { \"association\" : [],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] },"
        "\"GPU7_ERoT\" : { \"association\" : [],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] },"
        "\"HSC0\" : { \"association\" : [],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] },"
        "\"HSC1\" : { \"association\" : [],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] },"
        "\"HSC2\" : { \"association\" : [],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] },"
        "\"HSC3\" : { \"association\" : [],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] },"
        "\"HSC4\" : { \"association\" : [],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] },"
        "\"HSC5\" : { \"association\" : [],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] },"
        "\"HSC6\" : { \"association\" : [],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] },"
        "\"HSC7\" : { \"association\" : [],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] },"
        "\"HSC8\" : { \"association\" : [],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] },"
        "\"HSC9\" : { \"association\" : [],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] },"
        "\"NVSwitch0_ERoT\" : { \"association\" : [],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] },"
        "\"NVSwitch1_ERoT\" : { \"association\" : [],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] },"
        "\"NVSwitch2_ERoT\" : { \"association\" : [],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] },"
        "\"NVSwitch3_ERoT\" : { \"association\" : [],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] },"
        "\"PCIeSwitch_ERoT\" : { \"association\" : [],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] },"
        "\"FPGA_ERoT\" : { \"association\" : [],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] },"
        "\"HMC_ERoT\" : { \"association\" : [],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] },"
        "\"HSC_STBY\" : { \"association\" : [],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] },"
        "\"VR\" : { \"association\" : [],"
        "  \"power_rail\" : [], \"erot_control\" : [], \"pin_status\" : [],"
        "  \"interface_status\" : [], \"protocol_status\" : [],"
        "  \"firmware_status\" : [] }"
        "}");
}

void ensureProperFormat(json& j)
{
    for (auto& [key, value] : j.items())
    {
        std::cout << key << " : " << value << "\n";
        if (!j[key].contains("association"))
        {
            j[key]["association"] = json::array();
        }
        if (!j[key].contains("power_rail"))
        {
            j[key]["power_rail"] = json::array();
        }
        if (!j[key].contains("erot_control"))
        {
            j[key]["erot_control"] = json::array();
        }
        if (!j[key].contains("pin_status"))
        {
            j[key]["pin_status"] = json::array();
        }
        if (!j[key].contains("interface_status"))
        {
            j[key]["interface_status"] = json::array();
        }
        if (!j[key].contains("protocol_status"))
        {
            j[key]["protocol_status"] = json::array();
        }
        if (!j[key].contains("firmware_status"))
        {
            j[key]["firmware_status"] = json::array();
        }
    }
}

json getDatJson_60742061()
{
    json j;
    j["Baseboard_0"]["association"] = {
        "GPU_SXM_1",  "GPU_SXM_2",  "GPU_SXM_3",  "GPU_SXM_4",
        "GPU_SXM_5",  "GPU_SXM_6",  "GPU_SXM_7",  "GPU_SXM_8",
        "NVSwitch_0", "NVSwitch_1", "NVSwitch_2", "NVSwitch_3"};
    j["GPU_SXM_1"]["association"] = {"HSC_0", "ERoT_GPU_SXM_1",
                                     "GPU_SXM_1_DRAM_0", "PCIeRetimer_0"};
    j["GPU_SXM_2"]["association"] = {"HSC_1", "ERoT_GPU_SXM_2",
                                     "GPU_SXM_2_DRAM_0", "PCIeRetimer_1"};
    j["GPU_SXM_3"]["association"] = {"HSC_2", "ERoT_GPU_SXM_3",
                                     "GPU_SXM_3_DRAM_0", "PCIeRetimer_2"};
    j["GPU_SXM_4"]["association"] = {"HSC_3", "ERoT_GPU_SXM_4",
                                     "GPU_SXM_4_DRAM_0", "PCIeRetimer_3"};
    j["GPU_SXM_5"]["association"] = {"HSC_4", "ERoT_GPU_SXM_5",
                                     "GPU_SXM_5_DRAM_0", "PCIeRetimer_4"};
    j["GPU_SXM_6"]["association"] = {"HSC_5", "ERoT_GPU_SXM_6",
                                     "GPU_SXM_6_DRAM_0", "PCIeRetimer_5"};
    j["GPU_SXM_7"]["association"] = {"HSC_6", "ERoT_GPU_SXM_7",
                                     "GPU_SXM_7_DRAM_0", "PCIeRetimer_6"};
    j["GPU_SXM_8"]["association"] = {"HSC_7", "ERoT_GPU_SXM_8",
                                     "GPU_SXM_8_DRAM_0", "PCIeRetimer_7"};
    j["GPU_SXM_1_DRAM_0"]["association"] = json::array();
    j["GPU_SXM_2_DRAM_0"]["association"] = json::array();
    j["GPU_SXM_3_DRAM_0"]["association"] = json::array();
    j["GPU_SXM_4_DRAM_0"]["association"] = json::array();
    j["GPU_SXM_5_DRAM_0"]["association"] = json::array();
    j["GPU_SXM_6_DRAM_0"]["association"] = json::array();
    j["GPU_SXM_7_DRAM_0"]["association"] = json::array();
    j["GPU_SXM_8_DRAM_0"]["association"] = json::array();
    j["NVSwitch_0"]["association"] = {"VR", "HSC_8", "ERoT_NVSwitch_0",
                                      "PCIeSwitch_0"};
    j["NVSwitch_1"]["association"] = {"VR", "HSC_8", "ERoT_NVSwitch_1",
                                      "PCIeSwitch_0"};
    j["NVSwitch_2"]["association"] = {"VR", "HSC_9", "ERoT_NVSwitch_2",
                                      "PCIeSwitch_0"};
    j["NVSwitch_3"]["association"] = {"VR", "HSC_9", "ERoT_NVSwitch_3",
                                      "PCIeSwitch_0"};
    j["PCIeRetimer_0"]["association"] = {"HSC_8"};
    j["PCIeRetimer_1"]["association"] = {"HSC_8"};
    j["PCIeRetimer_2"]["association"] = {"HSC_8"};
    j["PCIeRetimer_3"]["association"] = {"HSC_8"};
    j["PCIeRetimer_4"]["association"] = {"HSC_9"};
    j["PCIeRetimer_5"]["association"] = {"HSC_9"};
    j["PCIeRetimer_6"]["association"] = {"HSC_9"};
    j["PCIeRetimer_7"]["association"] = {"HSC_9"};
    j["PCIeSwitch_0"]["association"] = {"StandbyHSC_0", "ERoT_PCIeSwitch_0"};
    j["FPGA_0"]["association"] = {"StandbyHSC_0", "ERoT_FPGA_0", "InletSensor"};
    j["InletSensor"]["association"] = {"StandbyHSC_0"};
    j["HMC_0"]["association"] = {"StandbyHSC_0", "ERoT_HMC_0"};
    j["ERoT_GPU_SXM_1"]["association"] = {"StandbyHSC_0"};
    j["ERoT_GPU_SXM_2"]["association"] = {"StandbyHSC_0"};
    j["ERoT_GPU_SXM_3"]["association"] = {"StandbyHSC_0"};
    j["ERoT_GPU_SXM_4"]["association"] = {"StandbyHSC_0"};
    j["ERoT_GPU_SXM_5"]["association"] = {"StandbyHSC_0"};
    j["ERoT_GPU_SXM_6"]["association"] = {"StandbyHSC_0"};
    j["ERoT_GPU_SXM_7"]["association"] = {"StandbyHSC_0"};
    j["ERoT_GPU_SXM_8"]["association"] = {"StandbyHSC_0"};
    j["HSC_0"]["association"] = json::array();
    j["HSC_1"]["association"] = json::array();
    j["HSC_2"]["association"] = json::array();
    j["HSC_3"]["association"] = json::array();
    j["HSC_4"]["association"] = json::array();
    j["HSC_5"]["association"] = json::array();
    j["HSC_6"]["association"] = json::array();
    j["HSC_7"]["association"] = json::array();
    j["HSC_8"]["association"] = json::array();
    j["HSC_9"]["association"] = json::array();
    j["ERoT_NVSwitch_0"]["association"] = {"StandbyHSC_0"};
    j["ERoT_NVSwitch_1"]["association"] = {"StandbyHSC_0"};
    j["ERoT_NVSwitch_2"]["association"] = {"StandbyHSC_0"};
    j["ERoT_NVSwitch_3"]["association"] = {"StandbyHSC_0"};
    j["ERoT_PCIeSwitch_0"]["association"] = {"StandbyHSC_0"};
    j["ERoT_FPGA_0"]["association"] = {"StandbyHSC_0"};
    j["ERoT_HMC_0"]["association"] = {"StandbyHSC_0"};
    j["StandbyHSC_0"]["association"] = json::array();
    j["VR"]["association"] = json::array();
    ensureProperFormat(j);
    return j;
}

std::map<std::string, dat_traverse::Device> jsonToDeviceMap(json jsonConfig)
{
    std::map<std::string, dat_traverse::Device> result;
    for (const auto& el : jsonConfig.items())
    {
        auto deviceName = el.key();
        dat_traverse::Device device(deviceName, el.value());
        result.insert(
            std::pair<std::string, dat_traverse::Device>(deviceName, device));
    }
    // fill out parents of all child devices on 2nd pass
    for (const auto& entry : result)
    {
        for (const auto& child : entry.second.association)
        {
            result.at(child).parents.push_back(entry.first);
        }
    }
    return result;
}

template <typename T>
void showVector(const std::vector<T>& vec, bool printSize = true,
                bool printIndexes = true)
{
    if (printSize)
    {
        cout << "size = " << vec.size() << endl;
    }
    for (auto i = 0u; i < vec.size(); ++i)
    {
        if (printIndexes)
        {
            std::cout << "[" << i << "]: " << vec.at(i) << std::endl;
        }
        else // ! printIndexes
        {
            std::cout << vec.at(i) << std::endl;
        }
    }
    std::cout << std::endl;
}

template <typename T>
std::vector<T> subVector(const std::vector<T>& target,
                         const std::vector<int>& indexes)
{
    std::vector<T> result;
    for (auto i : indexes)
    {
        result.push_back(target[i]);
    }
    return result;
}

TEST(DatToDbusTest, JsonTest)
{
    std::map<std::string, dat_traverse::Device> devMap =
        jsonToDeviceMap(getStandardDatJson());

    event_handler::DATTraverse datTraverser("");
    datTraverser.setDAT(devMap);

    EXPECT_THAT(datTraverser.getAssociationConnectedDevices("Baseboard"),
                UnorderedElementsAre(
                    "Baseboard", "NVSwitch3", "PCIeSwitch", "PCIeSwitch_ERoT",
                    "HSC_STBY", "NVSwitch3_ERoT", "HSC9", "VR", "NVSwitch2",
                    "NVSwitch2_ERoT", "NVSwitch1", "NVSwitch1_ERoT", "HSC8",
                    "NVSwitch0", "NVSwitch0_ERoT", "GPU7", "PCIeRetimer7",
                    "GPU7_ERoT", "HSC7", "GPU6", "PCIeRetimer6", "GPU6_ERoT",
                    "HSC6", "GPU5", "PCIeRetimer5", "GPU5_ERoT", "HSC5", "GPU4",
                    "PCIeRetimer4", "GPU4_ERoT", "HSC4", "GPU3", "PCIeRetimer3",
                    "GPU3_ERoT", "HSC3", "GPU2", "PCIeRetimer2", "GPU2_ERoT",
                    "HSC2", "GPU1", "PCIeRetimer1", "GPU1_ERoT", "HSC1", "GPU0",
                    "PCIeRetimer0", "GPU0_ERoT", "HSC0"));
    EXPECT_THAT(datTraverser.getAssociationConnectedDevices("FPGA"),
                UnorderedElementsAre("FPGA", "HMC", "HMC_ERoT", "HSC_STBY",
                                     "FPGA_ERoT"));
    EXPECT_THAT(datTraverser.getAssociationConnectedDevices("FPGA_ERoT"),
                UnorderedElementsAre("FPGA_ERoT"));
    EXPECT_THAT(datTraverser.getAssociationConnectedDevices("GPU0"),
                UnorderedElementsAre("GPU0", "PCIeRetimer0", "HSC8",
                                     "GPU0_ERoT", "HSC0"));
    EXPECT_THAT(datTraverser.getAssociationConnectedDevices("GPU0_ERoT"),
                UnorderedElementsAre("GPU0_ERoT"));
    EXPECT_THAT(datTraverser.getAssociationConnectedDevices("GPU1"),
                UnorderedElementsAre("GPU1", "PCIeRetimer1", "HSC8",
                                     "GPU1_ERoT", "HSC1"));
    EXPECT_THAT(datTraverser.getAssociationConnectedDevices("GPU1_ERoT"),
                UnorderedElementsAre("GPU1_ERoT"));
    EXPECT_THAT(datTraverser.getAssociationConnectedDevices("GPU2"),
                UnorderedElementsAre("GPU2", "PCIeRetimer2", "HSC8",
                                     "GPU2_ERoT", "HSC2"));
    EXPECT_THAT(datTraverser.getAssociationConnectedDevices("GPU2_ERoT"),
                UnorderedElementsAre("GPU2_ERoT"));
    EXPECT_THAT(datTraverser.getAssociationConnectedDevices("GPU3"),
                UnorderedElementsAre("GPU3", "PCIeRetimer3", "HSC8",
                                     "GPU3_ERoT", "HSC3"));
    EXPECT_THAT(datTraverser.getAssociationConnectedDevices("GPU3_ERoT"),
                UnorderedElementsAre("GPU3_ERoT"));
    EXPECT_THAT(datTraverser.getAssociationConnectedDevices("GPU4"),
                UnorderedElementsAre("GPU4", "PCIeRetimer4", "HSC9",
                                     "GPU4_ERoT", "HSC4"));
    EXPECT_THAT(datTraverser.getAssociationConnectedDevices("GPU4_ERoT"),
                UnorderedElementsAre("GPU4_ERoT"));
    EXPECT_THAT(datTraverser.getAssociationConnectedDevices("GPU5"),
                UnorderedElementsAre("GPU5", "PCIeRetimer5", "HSC9",
                                     "GPU5_ERoT", "HSC5"));
    EXPECT_THAT(datTraverser.getAssociationConnectedDevices("GPU5_ERoT"),
                UnorderedElementsAre("GPU5_ERoT"));
    EXPECT_THAT(datTraverser.getAssociationConnectedDevices("GPU6"),
                UnorderedElementsAre("GPU6", "PCIeRetimer6", "HSC9",
                                     "GPU6_ERoT", "HSC6"));
    EXPECT_THAT(datTraverser.getAssociationConnectedDevices("GPU6_ERoT"),
                UnorderedElementsAre("GPU6_ERoT"));
    EXPECT_THAT(datTraverser.getAssociationConnectedDevices("GPU7"),
                UnorderedElementsAre("GPU7", "PCIeRetimer7", "HSC9",
                                     "GPU7_ERoT", "HSC7"));
    EXPECT_THAT(datTraverser.getAssociationConnectedDevices("GPU7_ERoT"),
                UnorderedElementsAre("GPU7_ERoT"));
    EXPECT_THAT(datTraverser.getAssociationConnectedDevices("HMC"),
                UnorderedElementsAre("HMC", "HMC_ERoT", "HSC_STBY"));
    EXPECT_THAT(datTraverser.getAssociationConnectedDevices("HMC_ERoT"),
                UnorderedElementsAre("HMC_ERoT"));
    EXPECT_THAT(datTraverser.getAssociationConnectedDevices("HSC0"),
                UnorderedElementsAre("HSC0"));
    EXPECT_THAT(datTraverser.getAssociationConnectedDevices("HSC1"),
                UnorderedElementsAre("HSC1"));
    EXPECT_THAT(datTraverser.getAssociationConnectedDevices("HSC2"),
                UnorderedElementsAre("HSC2"));
    EXPECT_THAT(datTraverser.getAssociationConnectedDevices("HSC3"),
                UnorderedElementsAre("HSC3"));
    EXPECT_THAT(datTraverser.getAssociationConnectedDevices("HSC4"),
                UnorderedElementsAre("HSC4"));
    EXPECT_THAT(datTraverser.getAssociationConnectedDevices("HSC5"),
                UnorderedElementsAre("HSC5"));
    EXPECT_THAT(datTraverser.getAssociationConnectedDevices("HSC6"),
                UnorderedElementsAre("HSC6"));
    EXPECT_THAT(datTraverser.getAssociationConnectedDevices("HSC7"),
                UnorderedElementsAre("HSC7"));
    EXPECT_THAT(datTraverser.getAssociationConnectedDevices("HSC8"),
                UnorderedElementsAre("HSC8"));
    EXPECT_THAT(datTraverser.getAssociationConnectedDevices("HSC9"),
                UnorderedElementsAre("HSC9"));
    EXPECT_THAT(datTraverser.getAssociationConnectedDevices("HSC_STBY"),
                UnorderedElementsAre("HSC_STBY"));
    EXPECT_THAT(datTraverser.getAssociationConnectedDevices("NVSwitch0"),
                UnorderedElementsAre("NVSwitch0", "PCIeSwitch",
                                     "PCIeSwitch_ERoT", "HSC_STBY",
                                     "NVSwitch0_ERoT", "HSC8", "VR"));
    EXPECT_THAT(datTraverser.getAssociationConnectedDevices("NVSwitch0_ERoT"),
                UnorderedElementsAre("NVSwitch0_ERoT"));
    EXPECT_THAT(datTraverser.getAssociationConnectedDevices("NVSwitch1"),
                UnorderedElementsAre("NVSwitch1", "PCIeSwitch",
                                     "PCIeSwitch_ERoT", "HSC_STBY",
                                     "NVSwitch1_ERoT", "HSC8", "VR"));
    EXPECT_THAT(datTraverser.getAssociationConnectedDevices("NVSwitch1_ERoT"),
                UnorderedElementsAre("NVSwitch1_ERoT"));
    EXPECT_THAT(datTraverser.getAssociationConnectedDevices("NVSwitch2"),
                UnorderedElementsAre("NVSwitch2", "PCIeSwitch",
                                     "PCIeSwitch_ERoT", "HSC_STBY",
                                     "NVSwitch2_ERoT", "HSC9", "VR"));
    EXPECT_THAT(datTraverser.getAssociationConnectedDevices("NVSwitch2_ERoT"),
                UnorderedElementsAre("NVSwitch2_ERoT"));
    EXPECT_THAT(datTraverser.getAssociationConnectedDevices("NVSwitch3"),
                UnorderedElementsAre("NVSwitch3", "PCIeSwitch",
                                     "PCIeSwitch_ERoT", "HSC_STBY",
                                     "NVSwitch3_ERoT", "HSC9", "VR"));
    EXPECT_THAT(datTraverser.getAssociationConnectedDevices("NVSwitch3_ERoT"),
                UnorderedElementsAre("NVSwitch3_ERoT"));
    EXPECT_THAT(
        datTraverser.getAssociationConnectedDevices("PCIeSwitch"),
        UnorderedElementsAre("PCIeSwitch", "PCIeSwitch_ERoT", "HSC_STBY"));
    EXPECT_THAT(datTraverser.getAssociationConnectedDevices("PCIeSwitch_ERoT"),
                UnorderedElementsAre("PCIeSwitch_ERoT"));
    EXPECT_THAT(datTraverser.getAssociationConnectedDevices("PCIeRetimer0"),
                UnorderedElementsAre("PCIeRetimer0", "HSC8"));
    EXPECT_THAT(datTraverser.getAssociationConnectedDevices("PCIeRetimer1"),
                UnorderedElementsAre("PCIeRetimer1", "HSC8"));
    EXPECT_THAT(datTraverser.getAssociationConnectedDevices("PCIeRetimer2"),
                UnorderedElementsAre("PCIeRetimer2", "HSC8"));
    EXPECT_THAT(datTraverser.getAssociationConnectedDevices("PCIeRetimer3"),
                UnorderedElementsAre("PCIeRetimer3", "HSC8"));
    EXPECT_THAT(datTraverser.getAssociationConnectedDevices("PCIeRetimer4"),
                UnorderedElementsAre("PCIeRetimer4", "HSC9"));
    EXPECT_THAT(datTraverser.getAssociationConnectedDevices("PCIeRetimer5"),
                UnorderedElementsAre("PCIeRetimer5", "HSC9"));
    EXPECT_THAT(datTraverser.getAssociationConnectedDevices("PCIeRetimer6"),
                UnorderedElementsAre("PCIeRetimer6", "HSC9"));
    EXPECT_THAT(datTraverser.getAssociationConnectedDevices("PCIeRetimer7"),
                UnorderedElementsAre("PCIeRetimer7", "HSC9"));
    EXPECT_THAT(datTraverser.getAssociationConnectedDevices("VR"),
                UnorderedElementsAre("VR"));
}
