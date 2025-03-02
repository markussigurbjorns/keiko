project(
  'keiko',
  'c',
  version: '0.1',
  default_options: ['warning_level=3', 'c_std=gnu11'],
)

cc = meson.get_compiler('c')

portaudio_lib = cc.find_library('portaudio', required: true)
math_lib = cc.find_library('m', required: true)

include = include_directories('include')
include_modules = include_directories('include/modules')

src_files = files(
  'src/audio_graph.c',
  'src/main.c',
  'src/modules/lowpass_filter_module.c',
  'src/modules/output_module.c',
  'src/modules/sine_osc_module.c',
)

executable(
  'main',
  src_files,
  include_directories: [include, include_modules],
  dependencies: [portaudio_lib, math_lib],
)
