# Gather the Configuration data

conf_data = configuration_data()
conf_data.set_quoted('MONEVT_DEVICESTATUSFS_PATH', get_option('device-status-fs-path'))

conf_h_dep = declare_dependency(
    include_directories: include_directories('.'),
    sources: configure_file(
        input: 'eventing_config.h.in',
        output: 'eventing_config.h',
        configuration: conf_data
    )
)
