if get_option('eventing_feature_only').disabled()

    selftest_tool_sources = ['selftest_tool.cpp']
    selftest_tool_deps = declare_dependency(link_with : [eventinglib, selftestlib],
    include_directories : inc)

    selftest_tool = executable('selftest_tool',
        selftest_tool_sources,
        include_directories: inc,
        dependencies: [
            selftest_tool_deps,
            phosphor_logging_dep,
        ],
        install: true,
    )

    dbus_tester_sources = [
        'dbus_tester.cpp']

    dbus_tester_bin = executable(
        'dbus_tester',
        dbus_tester_sources,
        dependencies: mon_evt_dependencies + [
            sdbusplus_dep,
            meson.get_compiler('cpp').find_library('rt')
        ],
        install: false,
    )
endif

subdir('jtag')

bindir = get_option('prefix') + '/' + get_option('bindir')
scripts = [
    'mctp-error-detection',
    'wrapper_fpga_register_table/fpga_regtbl_wrapper',
    'wrapper_pcie_status/pcie_wrapper',
    'wrapper_i2c_status/i2c_wrapper',
    'wrapper_nvlink_wp_status/nvlink-wp-status-wrapper',
    'device_id_mapping/device-id-norm.sh',

    'fpga_regtbl',
    'glacier_boot_status_parser',
    'ocp_recovery_status_parser',
    'gb200_pwrfail_parser'
]

install_data(scripts, install_dir: bindir)
