gnome = import('gnome')

# Dependencies
cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : false)
gtk_dep = [
    dependency('gtk+-3.0', version: '>= 3.16'),
    dependency('gobject-2.0'),
    dependency('json-glib-1.0', version: '>= 1.0')
]
alsa_dep = dependency('alsa')

# Generate config.h
sm_conf = configuration_data()
sm_conf.set_quoted('PACKAGE', package, description: 'Name of package')
sm_conf.set_quoted('PACKAGE_NAME', meson.project_name(), description: 'Full name of this package')
sm_conf.set_quoted('PACKAGE_VERSION', meson.project_version(), description: 'Version of this package')
sm_conf.set10('DEBUG', get_option('debug'), description: 'Version of this package')
sm_conf.set_quoted('GSETTINGS_SCHEMA_DIR', meson.current_build_dir(), description: 'GSettings schema directory')
configure_file(output: 'config.h',
    configuration: sm_conf
)

# Compile GLib resources
sm_resources = gnome.compile_resources(
    'sm-app-resources', 'sm-app.gresource.xml'
)

sm_sources = [
    'scarlettmixer.c',
    'sm-channel.c', 'sm-channel.h',
    'sm-source.c', 'sm-source.h',
    'sm-switch.c', 'sm-switch.h',
    'sm-app.c', 'sm-app.h',
    'sm-appwin.c', 'sm-appwin.h',
    'sm-prefs.c', 'sm-prefs.h',
    'sm-strip.c', 'sm-strip.h',
    'sm-mix-strip.c', 'sm-mix-strip.h'
]

executable(package, [sm_sources, sm_resources], dependencies: [m_dep, gtk_dep, alsa_dep], install: true)

# Compile GSetting schema
if get_option('debug')
    gnome.compile_schemas(build_by_default: true, depend_files: 'org.alsa.scarlettmixer.gschema.xml')
endif
