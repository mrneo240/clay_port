project('glad', 'c', version: '1.0.0')

cc = meson.get_compiler('c')

glad_src = ['src/glad.c']
inc_base = include_directories('include')

glad_deps = []
glad_deps += cc.find_library('m', required: false)

src = [ glad_src ]
inc = [ inc_base ]

#glad_lib = static_library('glad', sources: src ,
#                include_directories: inc,
#                dependencies: glad_deps)

glad_dep = declare_dependency(
    sources: [src],
    include_directories: inc,
    dependencies: glad_deps)
