cmake_minimum_required(VERSION 3.18)
project(ogl_module_prototype VERSION 0.1.0 LANGUAGES CXX)

if(NOT UNIX)
    message(FATAL_ERROR "The experimental OGL module probe currently supports Unix only")
endif()

add_library(ogl_hello_module SHARED examples/hello/hello_module.cpp)
target_include_directories(ogl_hello_module PRIVATE include)
target_compile_features(ogl_hello_module PRIVATE cxx_std_17)
set_target_properties(ogl_hello_module PROPERTIES PREFIX "" OUTPUT_NAME "ogl_hello")

add_executable(ogl_module_probe tools/module_probe.cpp)
target_include_directories(ogl_module_probe PRIVATE include)
target_compile_features(ogl_module_probe PRIVATE cxx_std_17)
target_link_libraries(ogl_module_probe PRIVATE ${CMAKE_DL_LIBS})

# Keep NVIDIA / Vulkan SDK optional: unsupported systems retain OpenGL.
set(OGL_VULKAN_HEADER_DIR "" CACHE PATH "Vulkan SDK include directory")
if(NOT OGL_VULKAN_HEADER_DIR)
    find_path(OGL_VULKAN_HEADER_DIR vulkan/vulkan.h)
endif()
if(OGL_VULKAN_HEADER_DIR AND EXISTS "${OGL_VULKAN_HEADER_DIR}/vulkan/vulkan_core.h")
    add_library(ogl_rtx_module SHARED rtx/ogl_rtx_module.cpp)
    target_include_directories(ogl_rtx_module PRIVATE include "${OGL_VULKAN_HEADER_DIR}")
    target_compile_features(ogl_rtx_module PRIVATE cxx_std_17)
    target_link_libraries(ogl_rtx_module PRIVATE ${CMAKE_DL_LIBS})
    set_target_properties(ogl_rtx_module PROPERTIES PREFIX "" OUTPUT_NAME "ogl_rtx")

    add_test(
      NAME ogl_rtx_handshake
      COMMAND ogl_module_probe $<TARGET_FILE:ogl_rtx_module>
    )
    # Disable Linux ICD selection: the module must fall back without crashing.
    add_test(
      NAME ogl_rtx_no_gpu_fallback
      COMMAND ${CMAKE_COMMAND} -E env VK_DRIVER_FILES=/nonexistent-ogl-vulkan-icd.json
              $<TARGET_FILE:ogl_module_probe> $<TARGET_FILE:ogl_rtx_module>
    )
else()
    message(STATUS "OGL RTX disabled: Vulkan headers unavailable (OpenGL fallback).")
endif()

enable_testing()
add_test(
  NAME ogl_module_handshake
  COMMAND ogl_module_probe $<TARGET_FILE:ogl_hello_module>
)

# Pure C++ HTTP login allowlist contract. It never opens the network.
add_executable(ogl_grid_http_policy_test ../tests/test_grid_http_policy.cpp)
target_compile_features(ogl_grid_http_policy_test PRIVATE cxx_std_17)
add_test(NAME ogl_grid_http_policy COMMAND ogl_grid_http_policy_test)
