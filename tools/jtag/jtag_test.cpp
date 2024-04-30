/**
 * Copyright (c) 2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#include "jtag.h"
#include <iostream>
#include <sstream>
#include <fcntl.h>
#include <vector>
#include <bitset>
#include <sys/ioctl.h>
#include <unistd.h>
#include <chrono>
#include <thread>

const int max_ir_size = 1024;
const std::vector<int> trst_tms = {1,1,1,1,1,1,1,1,1,1};
const std::vector<int> shiftdr_tms = {0,1,0,0};
const std::vector<int> shiftir_tms = {0,1,1,0,0};
const std::vector<int> rti_from_exit1_tms = {0,1,1,0};
const std::vector<int> shiftdr_from_rti_tms = {1,0,0};

typedef struct jtag_device {
    int number_of_devices;
    int ir_length;
    int sleep;
    std::string expected_idcode;
    bool verbose;
    uint32_t freq;
} jtag_device;

void printHelp()
{
    std::cout << "Read idcode from a device over jtag" << std::endl;
    std::cout << "jtag_test -j DEV_PATH -i EXPECTED_IDCODE_IN_HEX" << std::endl;
    std::cout << "Example: jtag_test -j /dev/jtag0 -i 0x0318A0DD" << std::endl;
    std::cout << "Additional options:" << std::endl<< "-v for verbose" << std::endl << "-s <N> how many miliseconds to sleep in between each jtag clock cycle" << std::endl;
    std::cout << "-f <N> jtag frequency in Hz. 1MHz default" << std::endl;
}

int goto_state(const jtag_device device, const std::vector<int> &tms, int fd)
{
    struct tck_bitbang *data;
    struct bitbang_packet bb_packet;
    int ret = 0;
    int cnt = 0;

    data = (tck_bitbang*)malloc(tms.size() * sizeof(struct tck_bitbang));
    if (!data)
        return -1;

    for (int tms_sample : tms)
    {
        data[cnt].tdi = 0;
        data[cnt].tms = 0;
        if (tms_sample != 0)
            data[cnt].tms = 1;
        cnt++;
    }
    bb_packet.length = tms.size();
    bb_packet.data = data;
    if (device.sleep > 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(device.sleep));
    ret = ioctl(fd, JTAG_IOCBITBANG, &bb_packet);

    free(data);
    return ret;
}

int shift_data_in_out(const jtag_device &device, std::vector<int> &tdi, std::vector<int> &tdo, int fd)
{
    struct tck_bitbang *data;
    struct bitbang_packet bb_packet;
    int ret = 0;

    data = (tck_bitbang*)malloc(tdi.size() * sizeof(struct tck_bitbang));
    if (!data)
        return -1;

    for (size_t i = 0; i < tdi.size(); i++)
    {
        data[i].tdi = 0;
        if (tdi[i] == 1)
            data[i].tdi = 1;
        data[i].tms = 0;
        if (i == (tdi.size() -1))
            data[i].tms = 1;
    }
    bb_packet.length = tdi.size();
    bb_packet.data = data;
    if (device.sleep > 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(device.sleep));
    ret = ioctl(fd, JTAG_IOCBITBANG, &bb_packet);
    for (size_t i = 0; i < tdi.size(); i++)
        tdo[i] = data[i].tdo;
    free(data);
    return ret;
}

int find_bitset_pattern(std::vector<int> large, std::vector<int> pattern)
{
    for (size_t i = 0; i < (large.size() - pattern.size()); i++)
    {
        for (size_t j = 0; j < pattern.size(); j++)
        {
            if (large[i + j] != pattern[j])
                break;
            if (j == pattern.size() -1)
                return i;
        }
    }
    return -1;
}

int get_number_of_devices(int fd, jtag_device &device)
{
    std::vector<int> all_ones;
    std::bitset<32> patt(0xDECAFBAD);
    std::vector<int> patt_search;
    std::vector<int> pattern;
    std::vector<int> tdo;

    all_ones.resize(max_ir_size, 1);
    pattern.resize(max_ir_size, 1);
    tdo.resize(max_ir_size);
    patt_search.resize(32);

    for (size_t i = 0; i < patt.size(); i++)
    {
        pattern[i] = 0;
        patt_search[i] = 0;
        if (patt[i])
        {
            pattern[i] = 1;
            patt_search[i] = 1;
        }
    }

    if (goto_state(device, trst_tms, fd))
    {
        std::cout << "Failed to reach trst state" << std::endl;
        return -1;
    }
    if (goto_state(device, shiftir_tms, fd))
    {
        std::cout << "Failed to reach shift dr state" << std::endl;
        return -1;
    }

    shift_data_in_out(device, all_ones, tdo, fd);

    if (goto_state(device, rti_from_exit1_tms, fd))
    {
        std::cout << "Failed to reach rti state" << std::endl;
        return -1;
    }

    if (goto_state(device, shiftdr_from_rti_tms, fd))
    {
        std::cout << "Failed to reach shiftdr state" << std::endl;
        return -1;
    }

    shift_data_in_out(device, pattern, tdo, fd);

    if (goto_state(device, trst_tms, fd))
    {
        std::cout << "Failed to reach trst state" << std::endl;
        return -1;
    }

    int pos = find_bitset_pattern(tdo, patt_search);
    if (pos != -1) {
        device.number_of_devices = pos;
        if (device.verbose)
            std::cout << "Found number of devices:" <<device.number_of_devices << std::endl;
    }
    else
    {
        std::cout << "Failed to find number of devices" << std::endl;
        return -2;
    }
    return 0;
}

int get_ir_length(int fd, jtag_device &device)
{
    std::vector<int> pattern;
    std::vector<int> patt_search;
    std::vector<int> tdo;
    std::bitset<32> patt(0xDECAFBAD);

    pattern.resize(max_ir_size, 1);
    tdo.resize(max_ir_size, 1);
    patt_search.resize(32);
    for (size_t i = 0; i < patt.size(); i++)
    {
        pattern[i] = 0;
        patt_search[i] = 0;
        if (patt[i])
        {
            pattern[i] = 1;
            patt_search[i] = 1;
        }
    }

    if (goto_state(device, trst_tms, fd))
    {
        std::cout << "Failed to reach trst state" << std::endl;
        return -1;
    }
    if (goto_state(device, shiftir_tms, fd))
    {
        std::cout << "Failed to reach shift dr state" << std::endl;
        return -1;
    }

    shift_data_in_out(device, pattern, tdo, fd);

    int pos = find_bitset_pattern(tdo, patt_search);
    if (pos != -1) {
        device.ir_length = pos;
        if (device.verbose)
            std::cout << "Found ir length devices:" <<device.ir_length << std::endl;
    }
    else
    {
        std::cout << "Failed to find ir length" << std::endl;
        return -2;
    }
    return 0;

}

int read_idcodes(int fd, jtag_device &device)
{
    std::vector<int> tdo;
    std::vector<int> tdi;
    std::bitset<32> idcodes;

    tdi.resize(device.number_of_devices * 32, 0);
    tdo.resize(device.number_of_devices * 32);

    if (goto_state(device, trst_tms, fd))
    {
        std::cout << "Failed to reach trst state" << std::endl;
        return -1;
    }
    if (goto_state(device, shiftdr_tms, fd))
    {
        std::cout << "Failed to reach shift dr state" << std::endl;
        return -1;
    }

    shift_data_in_out(device, tdi, tdo, fd);
    int cnt = 0;
    for (size_t i = 0; i < tdo.size(); i++)
    {
        if (cnt == 32)
        {
            std::stringstream ss;
            ss << "0x" << std::hex << idcodes.to_ulong();

            std::string hex_str = ss.str();
            if (device.verbose)
                std::cout << "idcode:" << hex_str << std::endl;
            if (device.expected_idcode.compare(ss.str()) != 0)
                return -1;
            cnt = 0;
        }
        idcodes[cnt] = false;
        if (tdo[i] == 1)
            idcodes[cnt] = true;
        cnt++;
    }
    std::stringstream ss;
    ss << "0x" << std::hex << idcodes.to_ulong();
    std::string hex_str = ss.str();
    if (device.verbose)
        std::cout << "idcode:" << hex_str << std::endl;
    if (device.expected_idcode.compare(ss.str()) != 0)
        return -1;

    return 0;
}

int main(int argc, char** argv)
{
    int c = 0;
    char *dev = NULL;
    int fd = -1;
    int ret = 0;
    jtag_device device;

    device.verbose = false;
    device.sleep = 0;
    device.freq = 1000000;

    while ((c = getopt(argc, argv, "j:i:s:hvf:")) != -1) {
        switch (c) {
            case 's':
                device.sleep = atoi(optarg);
                break;
            case 'f':
                device.freq = atoi(optarg);
                break;
            case 'i':
                device.expected_idcode = optarg;
                break;
            case 'v':
                device.verbose = true;
                break;
            case 'j':
                dev = optarg;
                break;
            case 'h':
                printHelp();
                break;
            default:
                printHelp();
        }
    }

    if (!dev || device.expected_idcode.empty()){
        std::cout << "Device path and idcode must be provided" << std::endl;
        printHelp();
        ret = 1;
        goto err;
    }

    fd = open(dev, O_RDWR);
    if (fd < 0) {
        std::cout << "Could not open device:" << dev << std::endl;
        ret = 2;
        goto err;
    }

    if (ioctl(fd, JTAG_SIOCFREQ, &device.freq))
    {
        std::cout << "Could not set frequency" << std::endl;
        ret = 3;
        goto err;
    }

    if (get_number_of_devices(fd, device))
    {
        std::cout << "Could not read jtag chain to get number of devices" << std::endl;
        ret = 4;
        goto err;
    }
    if (get_ir_length(fd, device))
    {
        std::cout << "Could not read jtag chain to get ir length" << std::endl;
        ret = 5;
        goto err;
    }
    if (read_idcodes(fd, device))
    {
        std::cout << "id code obtained did not match expected value" << std::endl;
        ret = 6;
        goto err;
    }
    close(fd);
    std::cout<<"0"<<std::endl;
    return 0;
err:
    close(fd);
    return ret;
}
