project('opengl_legacy_example', 'c',
  version : '0.1',
  default_options : ['c_std=gnu11','b_staticpic=false'])

# Compiler arguments
arg_base = ['-Wall', '-Wno-missing-braces', '-Werror=implicit-function-declaration', ]
arg_code = []
arg_linker = []

# Dependencies
cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : false)

# Options
opt_renderer = get_option('renderer')
opt_platform = get_option('platform')

inc_base = []
inc_base += include_directories('clay')
inc_base += include_directories('external')

src_files = []
src_files += ['main.c']

# Determine Platform
platform_dep = []

platform_inc = []
platform_inc += include_directories('platforms/shared')

platform_src = ['clay_impl.c']
platform_src += ['platforms/shared/input.c']

if opt_platform == 'glfw'
  glfw_dep = dependency('glfw3', fallback : ['glfw3', 'glfw3_dep'])
  glad_dep = dependency('glad', fallback : ['glad', 'glad_dep'])
  #cglm_dep = dependency('cglm', fallback : ['cglm', 'cglm_dep'])
  platform_dep += [glfw_dep , glad_dep]

  platform_src += ['platforms/glfw/clay_platform_glfw.c']
  platform_inc += include_directories('platforms/glfw/')
  opt_renderer = 'gl_legacy'

elif opt_platform == 'psp'
  arg_code += ['-DPLATFORM_PSP']
  arg_code += ['-DPATH_ASSETS=disc0:/PSP_GAME/USRDIR/', '-DPATH_USERDATA=ms0:/']
  arg_base += ['-fsingle-precision-constant', '-ffast-math',  '-fexpensive-optimizations', '-fomit-frame-pointer', '-finline-functions', '-Wdouble-promotion']

  arg_linker += [ '-Wl,--end-group', '-lpspgu', '-lpspgum', '-lpspdebug', '-lpspvfpu', '-lpspctrl', '-lpspge', '-lpspdisplay', '-lm', '-lpspsdk', '-lpsprtc', '-lpspaudio', '-lpsppower', '-Wl,--start-group', '-lc' ]

  pspsdk_link_args = []
  if get_option('buildtype') == 'debug'
    pspsdk_link_args = ['-g3', '-Wl,-Map=output.map']
  endif
  pspsdk_dep = declare_dependency(compile_args: [], link_args : pspsdk_link_args,
                                include_directories : ['/usr/local/pspdev/psp/sdk/include'] )
  platform_dep += [ pspsdk_dep ]

  opt_renderer = 'gu'

  platform_src += ['platforms/psp/clay_platform_psp.c']
  platform_inc += include_directories('platforms/psp/')

elif opt_platform == 'dc'
  arg_code += ['-DPLATFORM_DC', '-DNDEBUG']
  arg_code += ['-DPATH_ASSETS="/cd/"', '-DPATH_USERDATA="/pc/"']
  arg_base += ['-mpretend-cmove','-fsingle-precision-constant',
                 '-fomit-frame-pointer', '-finline-functions',  '-mfsrra', '-mfsca']

  dc_debug_dep = declare_dependency(compile_args: [], link_args :  ['-g3', '-Wl,-Map=output.map'])
  platform_dep += [dc_debug_dep]
  platform_inc += include_directories('platforms/dreamcast')
else
  error('No platform chosen!')
endif

if opt_renderer == 'gl_legacy'
    renderer_src = ['renderers/gl_legacy/clay_renderer_gl_legacy.c']
    renderer_inc = include_directories('renderers/gl_legacy/')

    libintrafont_dep = dependency('libintrafont', fallback : ['libintrafont', 'libintrafont_dep'])
    platform_dep += [libintrafont_dep]

    if opt_platform == 'glfw'
    elif opt_platform == 'dc'
      libgldc_dep = dependency('gldc', fallback : ['gldc', 'libgldc_dep'])
      platform_dep += [ libgldc_dep ]

      platform_src += ['platforms/dreamcast/gldc/clay_platform_dreamcast_gldc.c']
      platform_inc += include_directories('platforms/dreamcast/gldc')
    else
      error('Incorrect platform for renderer gl_legacy!')
    endif
elif opt_renderer == 'gu' and opt_platform == 'psp'
    renderer_src = ['renderers/gu/clay_renderer_gu.c']
    renderer_inc = include_directories('renderers/gu/')

    libintrafont_dep = dependency('libintrafont', fallback : ['libintrafont', 'libintrafont_dep'])
    platform_dep = [ libintrafont_dep ]

elif opt_renderer == 'pvr' and opt_platform == 'dc'
    renderer_src = [ 'renderers/pvr/clay_renderer_pvr.c', 'renderers/pvr/simple_texture_allocator.c', 'renderers/pvr/font_bmf.c',
      'renderers/pvr/pvr_texture.c', 'renderers/pvr/draw_pvr.c' ]
    renderer_inc = include_directories('renderers/pvr/')

    platform_src += ['platforms/dreamcast/pvr/clay_platform_dreamcast_pvr.c']
    platform_inc += include_directories('platforms/dreamcast/pvr')
else
    error('No renderer chosen with compatible platform!')
endif

src_files += [platform_src, renderer_src]

inc = [ inc_base, platform_inc, renderer_inc ]
src = [ src_files ]
dep = [ m_dep, platform_dep ]

arg_c = [ arg_base, arg_code ]

exe_menu = executable('opengl_legacy_example',
           c_args: arg_c,
           link_args: arg_linker,
           sources: src,
           include_directories : inc,
           dependencies: dep,
           install : true)


exe_raylib = executable('raylib_example',
           c_args: arg_c,
           link_args: arg_linker,
           sources: ['raylib_main.c', platform_src, renderer_src],
           include_directories : inc,
           dependencies: dep,
           install : true)

exe_clayculator = executable('clayculator',
           c_args: arg_c,
           link_args: arg_linker,
           sources: ['clayculator.c', platform_src, renderer_src],
           include_directories : inc,
           dependencies: dep,
           install : true)


fs = import('fs')
copy = fs.copyfile('resources/ltn4.pgf', 'ltn4.pgf')
copy = fs.copyfile('resources/ltn8.pgf', 'ltn8.pgf')

# Post processing
# Dreamcast
if opt_platform == 'dc'

  copy = fs.copyfile('resources/icon_black.pvr', 'icon_black.pvr')

  dc_binary = '1ST_READ.BIN'
  dc_cdi = 'clay.cdi'

  target_to_pkg = exe_menu

  bin_script_path = join_paths(meson.current_source_dir(), 'pkg_dreamcast.sh')
  custom_target(dc_binary,
    depends: target_to_pkg,
    input : target_to_pkg,
    output : dc_binary,
    command : [bin_script_path, '@INPUT@', '@OUTPUT@'],
    build_by_default : true)

  cdi_script_path = join_paths(meson.current_source_dir(), 'pkg_dreamcast_cdi.sh')
  custom_target(dc_cdi,
    depends: target_to_pkg,
    input : target_to_pkg,
    output : dc_cdi,
    command : [cdi_script_path, '@INPUT@', '@OUTPUT@'],
    build_by_default : true)

# Psp
elif opt_platform == 'psp'
  psp_binary = 'EBOOT.PBP'

  target_to_pkg = exe_menu
  copy = fs.copyfile('resources/ICON0.png', 'ICON0.png')

  script_path = join_paths(meson.current_source_dir(), 'pkg_psp.sh')

  custom_target(psp_binary,
    depends: target_to_pkg,
    input : target_to_pkg,
    output : psp_binary,
    command : [script_path, '@INPUT@', '@OUTPUT@'],
    build_by_default : true)
endif

#fs = import('fs')
#copy = fs.copyfile('resources/Roboto-Regular.ttf', 'Roboto-Regular.ttf')

