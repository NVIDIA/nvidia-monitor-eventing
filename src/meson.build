eventinglib_sources = [
    'config_schemas.cpp',
    'check_accessor.cpp',
    'config_parser.cpp',
    'dat_traverse.cpp',
    'data_accessor.cpp',
    'dbus_accessor.cpp',
    'device_id.cpp',
    'device_util.cpp',
    'diagnostics.cpp',
    'event_detection.cpp',
    'event_handler.cpp',
    'event_info.cpp',
    'json_proc.cpp',
    'json_schema.cpp',
    'log.cpp',
    'message_composer.cpp',
    'message_dispatcher.cpp',
    'property_accessor.cpp',
    'util.cpp']

eventinglib = shared_library('eventing',
                        eventinglib_sources,
                        include_directories : inc,
                        dependencies : mon_evt_dependencies,
                        version : '0.0.0',
                        install : true,
                        install_dir : get_option('libdir'))

selftestlib_sources = ['selftest.cpp']
selftestlib = shared_library('selftest',
                        selftestlib_sources,
                        include_directories : inc,
                        dependencies : [
                                        declare_dependency(link_with : eventinglib),
                                        sdbusplus_dep,
                                        phosphor_logging_dep
                                      ],
                        version : '0.0.0',
                        install : true,
                        install_dir : get_option('libdir'))

monitor_eventingd_sources = ['eventing_main.cpp']
monitor_eventingd_deps = declare_dependency(link_with : [eventinglib, selftestlib],
  include_directories : inc)

monitor_eventingd = executable(
    'monitor-eventingd',
    monitor_eventingd_sources,
    include_directories: inc,
    dependencies: [
        monitor_eventingd_deps,
        sdbusplus_dep,
        sdeventplus_dep,
        phosphor_logging_dep,
        meson.get_compiler('cpp').find_library('pthread'),
        meson.get_compiler('cpp').find_library('rt')
    ],
    install: true,
)
