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

