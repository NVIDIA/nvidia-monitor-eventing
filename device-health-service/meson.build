
sdbusplus = dependency('sdbusplus', fallback: ['sdbusplus', 'sdbusplus_dep'])

sdeventplus = dependency(
    'sdeventplus',
    fallback: ['sdeventplus', 'sdeventplus_dep'],
)

device_health_service_args = []

if get_option('deassertion_path_enabled').enabled()
    device_health_service_args += ['-DDEASSERTION_PATH_ENABLED']
endif

if get_option('deassertion_mode_deassert_log').enabled()
    device_health_service_args += ['-DDEASSERTION_MODE_DEASSERT_LOG']
endif

if get_option('deassertion_mode_log_resolved').enabled()
    device_health_service_args += ['-DDEASSERTION_MODE_LOG_RESOLVED']
endif

device_health_service_args += [
    '-DMAPPED_FILE_NAME="' + get_option('mapped_file_name') + '"',
]
device_health_service_args += [
    '-DINITIAL_SHMEM_SIZE=' + get_option('initial_shmem_size').to_string(),
]
device_health_service_args += [
    '-DINCREMENT_SHMEM_SIZE=' + get_option('increment_shmem_size').to_string(),
]
device_health_service_args += [
    '-DMAXIMUM_SHMEM_SIZE=' + get_option('maximum_shmem_size').to_string(),
]
device_health_service_args += [
    '-DDEFERRED_UPDATE_INTERVAL=' + get_option('deferred_update_interval').to_string(),
]
device_health_service_args += [
    '-DMAX_ERROR_ID_LENGTH=' + get_option('max_error_id_length').to_string(),
]
device_health_service_args += [
    '-DMAX_ASSERTED_ERROR_IDS_PER_DEV_SEV=' + get_option(
        'max_asserted_error_ids_per_dev_sev',
    ).to_string(),
]

device_health_serviced = executable(
    'device-health-serviced',
    'data_structures.cpp',
    'dbus.cpp',
    'device_health_service.cpp',
    implicit_include_directories: true,
    cpp_args: device_health_service_args,
    dependencies: [sdbusplus, sdeventplus, phosphor_logging_dep],
    install_dir: bindir,
    install: true,
)
