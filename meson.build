project('tinyev', 'c', meson_version: '>=0.61.4')
cc = meson.get_compiler('c')

cflags = ['-O0']

incdir = include_directories('include')

tinyev_srcs = ['src/tinyev.c']

debug_mode = get_option('debug_mode')
if debug_mode
    warning('Compiling debug mode')
    cflags += ['-DDEBUG', '-g', '-ggdb']
    tinyev_srcs += ['src/log.c']
endif

add_global_arguments(cflags, language: 'c')

dynamic_load = get_option('dynamic_load')
if dynamic_load
    warning('Creating .so file')
    # Create .so to dynamically load
    libtinyev = shared_library('tinyev',
                               tinyev_srcs,
                               include_directories: incdir)
else
    # Create .a to link
    libtinyev = static_library('tinyev',
                               tinyev_srcs,
                               include_directories: incdir)
    libtinyev_dep = declare_dependency(link_with: libtinyev)
endif

tests = get_option('tests')
if tests
    tests_deps = libtinyev_dep
    if not cc.has_function('strlcpy', prefix: '#include <string.h>')
        test_server_src = ['tests/strlcpy.c']
        test_client_src = ['tests/strlcpy.c']
    endif

    # Main test, simple usage
    executable('tinyev_test',
               ['tests/main.c'],
               dependencies: tests_deps,
               include_directories : incdir)

    # Client-server netcat type of thing
    test_server_src += ['tests/test_server.c']
    executable('test_server',
               test_server_src,
               dependencies: tests_deps,
               include_directories : incdir)

    test_client_src += ['tests/test_client.c']
    executable('test_client',
               test_client_src,
               dependencies: tests_deps,
               include_directories : incdir)
endif
