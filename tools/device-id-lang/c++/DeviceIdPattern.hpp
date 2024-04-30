/**
 * Copyright (c) 2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

class PatternIndex
{
    PatternIndex();
    void addIndex(unsigned i);
    unsigned dimensions();
};

class DeviceIdPattern
{
  public:
    DeviceIdPattern(string devIdPattern);

    string eval(PatternIndex index);
    iterator<PatternIndex> domain();
    iterator<string> values();
    PatternIndex match(string txt);
};

class JsonPattern
{
    JsonPattern(json js);

    json eval(PatternIndex index);
};

int main(int argc, char** argv)
{
    //
    // Constructing pattern index
    //
    PatternIndex pi;
    pi.addIndex(3);
    pi.addIndex(23);

    //
    // Constructing basic pattern
    //

    string device_type = "GPU_SXM_[0|1-8]/NVlink_[1|0-39]";
    DeviceIdPattern deviceTypePattern(device_type);

    //
    // Evaluating pattern at concrete point
    //

    string deviceTypeInstance = deviceTypePattern.eval(pi);
    // => "GPU_SXM_3/NVlink_23"

    //
    //
    //

    DeviceIdPattern foo("GPU_SXM_[1-8]");

    //
    // Generating accessors
    //

    json accessor(
        {{"type", "DBUS"},
         {"object",
          "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Endpoints/GPU_SXM_[0|1-8]"},
         {"interface", "xyz.openbmc_project.Inventory.Item.Port"},
         {"property", "FlitCRCCount"},
         {"check", {{"not_equal", "0"}}}});
    JsonPattern accessorsPattern(accessor);

    PatternIndex eventDeviceIndex;
    for (PatternIndex index : deviceTypePattern.domain())
    {
        json accessorInstance = accessorsPattern.eval(index);
        // Create data accessor, read value

        if (/* device id located */)
        {
            eventDeviceIndex = index;
        }
    }

    //
    // Reading telemetries
    //

    vector<json> telemetries(
        {{{"name", "NVLink flit CRC error count"},
          {"type", "DBUS"},
          {"object",
           "/xyz/openbmc_project/inventory/system/fabrics/HGX_NVLinkFabric_0/Endpoints/GPU_SXM_[0|1-8]"},
          {"interface", "xyz.openbmc_project.Inventory.Item.Port"},
          {"property", "FlitCRCCount"}}});

    for (json telemetry : telemetries)
    {
        JsonPattern telemetryPattern(telemetry);
        json telemetryInstance = telemetryPattern.eval(eventDeviceIndex);
        // Create accessor, read value
    }

    return 0;
}
