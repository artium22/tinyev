project('tinyev', 'c', meson_version: '>=0.61.4')
cc = meson.get_compiler('c')
incdir = include_directories('include')
cflags = ['-O0']
cflags += ['-DNDEBUG', '-g', '-ggdb']

add_global_arguments(cflags, language: 'c')
if not cc.has_function('strlcpy', prefix: '#include <string.h>')
    test_server_src = ['tests/strlcpy.c']
    test_client_src = ['tests/strlcpy.c']
endif

# Main test, simple usage
executable('tinyev_test', ['tests/main.c', 'src/tinyev.c'], include_directories : incdir)

test_server_src += ['tests/test_server.c', 'src/tinyev.c']
executable('test_server', test_server_src, include_directories : incdir)

test_client_src += ['tests/test_client.c', 'src/tinyev.c']
executable('test_client', test_client_src, include_directories : incdir)
