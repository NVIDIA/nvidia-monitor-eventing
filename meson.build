project(
    'c++ nvidia-monitor-eventing',
    'cpp',
    version: '0.1',
    license: 'MIT',
    meson_version: '>=1.1.1',
    default_options: ['cpp_std=c++23'],
)

# Project related links

project_pretty_name = 'nvidia-monitor-eventing'
project_url = 'https://gitlab-master.nvidia.com/dgx/nvidia-monitor-eventing'
project_issues_url = project_url + '/issues/new'
lang = 'cpp'

#summary('Issues',project_issues_url, section: 'Report Issues')

cxx = meson.get_compiler(lang)

## Set global arguments
add_global_arguments('-DSOME_TOKEN=value', language: lang)

# Validate the c++ Standard
if get_option('cpp_std') != 'c++23'
    error('This project requires c++20 support ' + get_option('cpp_std'))
endif

## Fetch all the project options and set the project arguments
debug_log_level = get_option('debug_log')
if debug_log_level != 0
    add_project_arguments(
        '-DDEF_DBG_LEVEL=' + debug_log_level.to_string(),
        language: lang,
    )
endif
if get_option('sandbox_mode').enabled()
    add_project_arguments('-DSANDBOX_MODE', language: lang)
endif

if get_option('log_elapsed_time').enabled()
    add_project_arguments('-DLOG_ELAPSED_TIME', language: lang)
endif

if get_option('profile_selftest_and_recovery').enabled()
    add_project_arguments(
        '-DPROFILING_SELFTEST_AND_RECOVERY_FLOW',
        language: lang,
    )
endif

if get_option('eventing_feature_only').enabled()
    add_project_arguments('-DEVENTING_FEATURE_ONLY', language: lang)
endif

# Device Health Service options
if (get_option('device_health_service').enabled())
    add_project_arguments('-DEVENTING_SERVICE_NO_DEVICE_HEALTH', language: lang)
endif


if get_option('device-status-fs').enabled()
    add_project_arguments('-DEVENTING_SERVICE_DEVICE_STATUS_FS', language: lang)
endif

if get_option('device-status-shmem').enabled()
    add_project_arguments(
        '-DEVENTING_SERVICE_DEVICE_STATUS_NVIDIA_SHMEM',
        language: lang,
    )
endif


# Get the options and enable the respective features
## create a MAP of  "options : feature_flag"

feature_map = {}

# Get the options status and build a project summary to show which flags are
# being enabled during the configuration time.

foreach option_key, option_value : feature_map
    if (get_option(option_key).enabled())
        summary(option_key, option_value, section: 'Enabled Features')
        add_global_arguments(option_value, language: lang)
    endif
endforeach

systemd_dep = dependency('systemd')

sdbusplus_dep = dependency('sdbusplus', required: false)
sdbusplus_proj = dependency('', required: false)
if not sdbusplus_dep.found() or sdbusplus_dep.type_name() == 'internal'
    sdbusplus_proj = subproject('sdbusplus')
endif
if not sdbusplus_dep.found()
    sdbusplus_dep = sdbusplus_proj.get_variable('sdbusplus_dep')
endif

if sdbusplus_proj.found()
    sdbusplusplus_prog = sdbusplus_proj.get_variable('sdbusplusplus_prog')
    sdbuspp_gen_meson_prog = sdbusplus_proj.get_variable(
        'sdbuspp_gen_meson_prog',
    )
else
    sdbusplusplus_prog = find_program('sdbus++', native: true)
    sdbuspp_gen_meson_prog = find_program('sdbus++-gen-meson', native: true)
endif

sdeventplus_dep = dependency(
    'sdeventplus',
    fallback: ['sdeventplus', 'sdeventplus_dep'],
)

phosphor_logging_dep = dependency(
    'phosphor-logging',
    fallback: ['phosphor-logging', 'phosphor_logging_dep'],
)

# Add all the dependencies
mon_evt_dependencies = []

mon_evt_dependencies += sdbusplus_dep
mon_evt_dependencies += phosphor_logging_dep
#mon_evt_dependencies += dependency('glib-2.0')
mon_evt_dependencies += dependency('threads')
mon_evt_dependencies += meson.get_compiler('cpp').find_library('pthread')
mon_evt_dependencies += meson.get_compiler('cpp').find_library('rt')

if cxx.has_header('nlohmann/json.hpp')
    mon_evt_dependencies += declare_dependency()
endif

# Add Include Directory
inc = include_directories('include', 'config')

# subdir for meson project
subdir('include')
subdir('src')
subdir('tools')
#subdir('test')

subdir('config')
mon_evt_dependencies += conf_h_dep

# Pkg-config
pkg_mod = import('pkgconfig')
pkg_mod.generate(
    libraries: eventinglib,
    version: '0.1',
    name: 'monitor-eventing',
    filebase: 'monitor-eventing',
    description: 'nvidia-monitor-eventing',
)

pkg_mod.generate(
    libraries: selftestlib,
    version: '0.1',
    name: 'selftest',
    filebase: 'selftest',
    description: 'nvidia-monitor-eventing-selftest',
)

if get_option('tests').enabled()
    gtest_dep = dependency('gtest', main: true, disabler: true, required: false)
    gmock_dep = dependency('gmock', disabler: true, required: false)
    if not gtest_dep.found() or not gmock_dep.found()
        gtest_proj = import('cmake').subproject('googletest', required: false)
        if gtest_proj.found()
            gtest_dep = declare_dependency(
                dependencies: [
                    dependency('threads'),
                    gtest_proj.dependency('gtest'),
                    gtest_proj.dependency('gtest_main'),
                ],
            )
            gmock_dep = gtest_proj.dependency('gmock')
        else
            assert(
                not get_option('tests').enabled(),
                'Googletest is required if tests are enabled',
            )
        endif
    endif
endif

srcfiles_unittest = [
    'test/accessor_expansion_test.cpp',
    'test/accessor_test.cpp',
    'test/dat_test.cpp',
    'test/dat_to_dbus_test.cpp',
    'test/dat_traverse_test.cpp',
    'test/dbus_accessor_test.cpp',
    'test/device_id_test.cpp',
    'test/event_detection_test.cpp',
    'test/event_instantiator_test.cpp',
    'test/event_test.cpp',
    'test/tests_common_defs.cpp',
    'test/json_proc_test.cpp',
    'test/propertyvalue_string_test.cpp',
    'test/propertyvalue_variant_test.cpp',
    'test/selftest_test.cpp',
    'test/standalone_qualifier_test.cpp',
    'test/util_test.cpp',
]

eventinglib_sources = [
    'src/config_schemas.cpp',
    'src/check_accessor.cpp',
    'src/config_parser.cpp',
    'src/dat_traverse.cpp',
    'src/data_accessor.cpp',
    'src/dbus_accessor.cpp',
    'src/device_id.cpp',
    'src/device_util.cpp',
    'src/diagnostics.cpp',
    'src/event_detection.cpp',
    'src/event_handler.cpp',
    'src/event_info.cpp',
    'src/json_proc.cpp',
    'src/json_schema.cpp',
    'src/log.cpp',
    'src/message_composer.cpp',
    'src/message_dispatcher.cpp',
    'src/event_instantiator.cpp',
    'src/property_accessor.cpp',
    'src/standalone_qualifier.cpp',
    'src/util.cpp',
]

selftestlib_sources = ['src/selftest.cpp']

bindir = get_option('prefix') + '/' + get_option('bindir')

## install memory watcher service
install_data(
    'monitor-eventing-memory-watcher/monitor-eventing-memory-watcher',
    install_dir: bindir,
)

# generate the test executable
if (get_option('tests').enabled())
    ut_bin = executable(
        'eventing_unit_test',
        srcfiles_unittest + eventinglib_sources + selftestlib_sources,
        include_directories: inc,
        install_dir: bindir,
        cpp_args: [
            '-DTEST_DATA_DIR="' + meson.current_source_dir() + '/test"',
        ],
        dependencies: mon_evt_dependencies + [gtest_dep, gmock_dep],
    )
    test('eventing_unit_test', ut_bin)
endif

if (get_option('device_health_service').enabled())
    subdir('device-health-service')
endif
