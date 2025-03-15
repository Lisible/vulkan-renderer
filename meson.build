project('vkguide', 'c', default_options: ['c_std=c17', 'warning_level=3'])

cc = meson.get_compiler('c')
sdl3_dep = dependency('SDL3')

if host_machine.system() == 'darwin'
moltenvk_library_path = '/Users/clements/dev/VulkanSDK/1.4.309.0/macOS/lib'
moltenvk_include_path = '/Users/clements/dev/VulkanSDK/1.4.309.0/macOS/include'
vulkan_dep = declare_dependency(
  link_args: ['-L' + moltenvk_library_path, '-lvulkan'],
  include_directories: include_directories(moltenvk_include_path)
)
else
vulkan_dep = dependency('vulkan')
endif

executable(
  'vkguide',
  ['src/main.c'],
  build_rpath: moltenvk_library_path,
  install_rpath: moltenvk_library_path,
  dependencies: [sdl3_dep, vulkan_dep],
)
