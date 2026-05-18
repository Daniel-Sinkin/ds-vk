cmake_minimum_required(VERSION 3.20)

if(APPLE AND NOT DEFINED CMAKE_C_COMPILER AND NOT DEFINED CMAKE_CXX_COMPILER)
  foreach(DS_VK_LLVM_PREFIX IN ITEMS /opt/homebrew/opt/llvm /usr/local/opt/llvm)
    if(EXISTS "${DS_VK_LLVM_PREFIX}/bin/clang" AND EXISTS "${DS_VK_LLVM_PREFIX}/bin/clang++")
      set(CMAKE_C_COMPILER "${DS_VK_LLVM_PREFIX}/bin/clang" CACHE FILEPATH "C compiler" FORCE)
      set(CMAKE_CXX_COMPILER "${DS_VK_LLVM_PREFIX}/bin/clang++" CACHE FILEPATH "C++ compiler" FORCE)
      message(STATUS "Using LLVM toolchain from ${DS_VK_LLVM_PREFIX}")
      break()
    endif()
  endforeach()
endif()

project(ds_vk VERSION 0.1.0 LANGUAGES C CXX)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(DS_VK_IS_TOP_LEVEL ON)
else()
  set(DS_VK_IS_TOP_LEVEL OFF)
endif()

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(DS_VK_BUILD_RUNTIME "Build the Vulkan runtime target" ON)
option(DS_VK_BUILD_EXAMPLES "Build ds_vk example applications" ${DS_VK_IS_TOP_LEVEL})
option(DS_VK_BUILD_TESTS "Build ds_vk tests" ${DS_VK_IS_TOP_LEVEL})
option(DS_VK_USE_SYSTEM_VULKAN "Build Vulkan runtime if Vulkan is discoverable" ON)
option(DS_VK_ENABLE_PCH "Build Vulkan runtime targets with precompiled headers" ON)

add_library(ds_vk_warnings INTERFACE)
target_compile_options(ds_vk_warnings INTERFACE
  $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:
    -Wall
    -Wextra
    -Wpedantic
    -Werror
    -Wshadow
    -Wconversion
    -Wfloat-conversion
    -Wsign-conversion
    -Wcast-qual
    -Wcast-align
    -Wformat=2
    -Wundef
    -Wnull-dereference
    -Wnon-virtual-dtor
    -Woverloaded-virtual
    -Wold-style-cast
    -Wzero-as-null-pointer-constant
    -Wdouble-promotion
    -Wimplicit-fallthrough
    -Wreturn-type>
  $<$<CXX_COMPILER_ID:Clang,AppleClang>:
    -Wconditional-uninitialized
    -Wextra-semi
    -Wimplicit-int-conversion
    -Wimplicit-int-float-conversion
    -Wnewline-eof
    -Wshorten-64-to-32
    -Wswitch-enum
    -Wunreachable-code>
  $<$<CXX_COMPILER_ID:GNU>:
    -Wduplicated-branches
    -Wduplicated-cond
    -Wlogical-op
    -Wsign-compare
    -Wuseless-cast>
)

if(NOT TARGET glm::glm)
  add_subdirectory(external/glm EXCLUDE_FROM_ALL)
endif()

set(DS_VK_CORE_SOURCES
  ds_vk/assets.cpp
  ds_vk/camera.cpp
  ds_vk/geometry.cpp
  ds_vk/mesh.cpp
)

add_library(ds_vk_core STATIC ${DS_VK_CORE_SOURCES})
add_library(ds_vk::core ALIAS ds_vk_core)
target_include_directories(ds_vk_core PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
target_include_directories(ds_vk_core SYSTEM PRIVATE
  external
)
target_link_libraries(ds_vk_core PUBLIC glm::glm)
target_link_libraries(ds_vk_core PRIVATE ds_vk_warnings)

add_library(ds_vk_picker STATIC ds_vk/plugins/picker.cpp)
add_library(ds_vk::picker ALIAS ds_vk_picker)
target_include_directories(ds_vk_picker PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_link_libraries(ds_vk_picker PUBLIC ds_vk_core)
target_link_libraries(ds_vk_picker PRIVATE ds_vk_warnings)

add_library(ds_vk_viz STATIC ds_vk/plugins/viz.cpp)
add_library(ds_vk::viz ALIAS ds_vk_viz)
target_include_directories(ds_vk_viz PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_link_libraries(ds_vk_viz PUBLIC ds_vk_core)
target_link_libraries(ds_vk_viz PRIVATE ds_vk_warnings)

add_library(ds_vk_manipulator STATIC ds_vk/plugins/manipulator.cpp)
add_library(ds_vk::manipulator ALIAS ds_vk_manipulator)
target_include_directories(ds_vk_manipulator PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_link_libraries(ds_vk_manipulator PUBLIC ds_vk_core)
target_link_libraries(ds_vk_manipulator PRIVATE ds_vk_warnings)

function(ds_vk_enable_pch target_name)
  if(DS_VK_ENABLE_PCH AND TARGET ${target_name})
    target_precompile_headers(${target_name} PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/ds_vk/pch.hpp
    )
  endif()
endfunction()

if(DS_VK_BUILD_TESTS)
  enable_testing()
  add_executable(ds_vk_tests tests/test_main.cpp)
  target_link_libraries(ds_vk_tests PRIVATE ds_vk_picker ds_vk_viz ds_vk_manipulator ds_vk_warnings)
  add_test(NAME ds_vk_tests COMMAND ds_vk_tests)
endif()

if(DS_VK_BUILD_RUNTIME AND DS_VK_USE_SYSTEM_VULKAN)
  find_package(Vulkan QUIET)
endif()

if(DS_VK_BUILD_RUNTIME AND Vulkan_FOUND)
  find_program(DS_VK_GLSLC_EXECUTABLE glslc HINTS "$ENV{VULKAN_SDK}/bin")
  if(NOT DS_VK_GLSLC_EXECUTABLE)
    message(FATAL_ERROR "Vulkan was found, but glslc was not. Set VULKAN_SDK or install Vulkan shader tools.")
  endif()

  set(DS_VK_SHADER_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
  set(DS_VK_SHADER_SOURCES
    ds_vk/shaders/environment.vert
    ds_vk/shaders/environment.frag
    ds_vk/shaders/mesh.vert
    ds_vk/shaders/mesh_position_normal.vert
    ds_vk/shaders/mesh_quantized_position_normal.vert
    ds_vk/shaders/mesh.frag
    ds_vk/shaders/shadow.vert
    ds_vk/shaders/shadow_quantized_position.vert
    ds_vk/shaders/debug_line.vert
    ds_vk/shaders/debug_line.frag
  )
  set(DS_VK_SHADER_OUTPUTS)
  foreach(DS_VK_SHADER ${DS_VK_SHADER_SOURCES})
    get_filename_component(DS_VK_SHADER_NAME "${DS_VK_SHADER}" NAME)
    set(DS_VK_SHADER_OUTPUT "${DS_VK_SHADER_OUTPUT_DIR}/${DS_VK_SHADER_NAME}.spv")
    add_custom_command(
      OUTPUT "${DS_VK_SHADER_OUTPUT}"
      COMMAND ${CMAKE_COMMAND} -E make_directory "${DS_VK_SHADER_OUTPUT_DIR}"
      COMMAND "${DS_VK_GLSLC_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/${DS_VK_SHADER}" -o "${DS_VK_SHADER_OUTPUT}"
      DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${DS_VK_SHADER}"
      VERBATIM
    )
    list(APPEND DS_VK_SHADER_OUTPUTS "${DS_VK_SHADER_OUTPUT}")
  endforeach()
  add_custom_target(ds_vk_shaders DEPENDS ${DS_VK_SHADER_OUTPUTS})

  set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
  set(SDL_TESTS OFF CACHE BOOL "" FORCE)
  set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(SDL_OPENGL OFF CACHE BOOL "" FORCE)
  set(SDL_OPENGLES OFF CACHE BOOL "" FORCE)
  if(NOT TARGET SDL3::SDL3)
    add_subdirectory(external/SDL EXCLUDE_FROM_ALL)
  endif()

  add_library(ds_vk_imgui_vulkan STATIC
    external/imgui/imgui.cpp
    external/imgui/imgui_demo.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/backends/imgui_impl_sdl3.cpp
    external/imgui/backends/imgui_impl_vulkan.cpp
  )
  target_include_directories(ds_vk_imgui_vulkan SYSTEM PUBLIC
    external/imgui
    external/imgui/backends
  )
  target_link_libraries(ds_vk_imgui_vulkan PUBLIC SDL3::SDL3 Vulkan::Vulkan)

  add_library(ds_vk_vma STATIC ds_vk/vma_support.cpp)
  target_include_directories(ds_vk_vma SYSTEM PUBLIC external/vma/include)
  target_link_libraries(ds_vk_vma PUBLIC Vulkan::Vulkan)

  add_library(ds_vk STATIC
    ds_vk/assets_write.cpp
    ds_vk/runtime.cpp
    ds_vk/stb_image_impl.cpp
    ds_vk/stb_image_write_impl.cpp
  )
  add_library(ds_vk::runtime ALIAS ds_vk)
  set_source_files_properties(
    ds_vk/stb_image_impl.cpp
    ds_vk/stb_image_write_impl.cpp
    PROPERTIES SKIP_PRECOMPILE_HEADERS ON
  )
  add_dependencies(ds_vk ds_vk_shaders)
  target_include_directories(ds_vk PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  )
  target_include_directories(ds_vk SYSTEM PRIVATE
    external
    external/stb
  )
  target_compile_definitions(ds_vk PRIVATE
    DS_VK_SHADER_DIR="${DS_VK_SHADER_OUTPUT_DIR}"
  )
  target_link_libraries(ds_vk PUBLIC
    ds_vk_core
    ds_vk_imgui_vulkan
    ds_vk_vma
    SDL3::SDL3
    Vulkan::Vulkan
  )
  target_link_libraries(ds_vk PRIVATE ds_vk_warnings)
  ds_vk_enable_pch(ds_vk)

  if(DS_VK_BUILD_EXAMPLES)
    add_executable(ds_vk_basic_app app/main.cpp)
    target_link_libraries(ds_vk_basic_app PRIVATE ds_vk ds_vk_viz ds_vk_warnings)
    ds_vk_enable_pch(ds_vk_basic_app)
  endif()

  if(DS_VK_BUILD_TESTS)
    add_executable(ds_vk_runtime_tests tests/test_runtime.cpp)
    target_link_libraries(ds_vk_runtime_tests PRIVATE ds_vk ds_vk_warnings)
    ds_vk_enable_pch(ds_vk_runtime_tests)
    add_test(NAME ds_vk_runtime_tests COMMAND ds_vk_runtime_tests)
  endif()
elseif(DS_VK_BUILD_RUNTIME)
  if(DS_VK_IS_TOP_LEVEL OR DS_VK_BUILD_EXAMPLES)
    message(WARNING "Vulkan runtime was requested but Vulkan was not found; building core targets only.")
  endif()
endif()
