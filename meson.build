project('Scarlett Mixer', 'c', version: '0.1.0', license : 'GPL-3.0-only')
package = 'scarlettmixer'
subdir('src')

# Build doxygen documentation
doxygen = find_program('doxygen', required : false)
if doxygen.found()
    doc_config = configuration_data()
    doc_config.set('PACKAGE_NAME', meson.project_name())
    doc_config.set('PACKAGE_VERSION', meson.project_version())
    doc_config.set('abs_top_srcdir', meson.current_source_dir())
    doc_config.set('abs_top_builddir', meson.current_build_dir())

    doxyfile = configure_file(input: 'Doxyfile.in',
        output: 'Doxyfile',
        configuration: doc_config,
        install: false)

    custom_target('doc',
        input: doxyfile,
        output: [ 'html' ],
        command: [ doxygen, doxyfile ],
        install: true,
        install_dir: get_option('datadir') / 'doc' / package,
        build_by_default: true)
    
else
    message('doxygen not found: Documentation generation disabled.')
endif

# Install data files
install_data(
    'scalable/apps/ScarlettMixer.svg',
    install_dir: get_option('datadir') / 'icons' / 'hicolor'
)

install_data(
    'ScarlettMixer.desktop',
    install_dir: get_option('datadir') / 'applications'
)

# Post install
if not get_option('debug')
    gnome = import('gnome')
    gnome.post_install(glib_compile_schemas: true, gtk_update_icon_cache: true)
endif
