cmake_minimum_required(VERSION 3.18)
project(ogl_module_prototype VERSION 0.1.0 LANGUAGES CXX)

if(NOT UNIX AND NOT WIN32)
    message(FATAL_ERROR "The OGL native module prototype supports Linux/Windows only")
endif()

add_library(ogl_hello_module SHARED examples/hello/hello_module.cpp)
target_include_directories(ogl_hello_module PRIVATE include)
target_compile_features(ogl_hello_module PRIVATE cxx_std_17)
set_target_properties(ogl_hello_module PROPERTIES PREFIX "" OUTPUT_NAME "ogl_hello")

if(UNIX)
    add_executable(ogl_module_probe tools/module_probe.cpp)
    target_include_directories(ogl_module_probe PRIVATE include)
    target_compile_features(ogl_module_probe PRIVATE cxx_std_17)
    target_link_libraries(ogl_module_probe PRIVATE ${CMAKE_DL_LIBS})
endif()

# Keep the Vulkan RT SDK optional: unsupported systems retain OpenGL.
set(OGL_VULKAN_HEADER_DIR "" CACHE PATH "Optional Vulkan SDK include directory")
if(NOT OGL_VULKAN_HEADER_DIR OR
   NOT EXISTS "${OGL_VULKAN_HEADER_DIR}/vulkan/vulkan_core.h")
    # Use a distinct find_path variable: a CACHE PATH pre-set to an empty
    # string is considered already defined and find_path otherwise skips
    # searching /usr/include even when libvulkan-dev is installed.
    find_path(OGL_VULKAN_DISCOVERED_INCLUDE NAMES vulkan/vulkan_core.h)
    if(OGL_VULKAN_DISCOVERED_INCLUDE)
        set(OGL_VULKAN_HEADER_DIR "${OGL_VULKAN_DISCOVERED_INCLUDE}")
    endif()
endif()
if(OGL_VULKAN_HEADER_DIR AND EXISTS "${OGL_VULKAN_HEADER_DIR}/vulkan/vulkan_core.h")
    add_library(ogl_rtx_module SHARED rtx/ogl_rtx_module.cpp)
    target_include_directories(ogl_rtx_module PRIVATE include "${OGL_VULKAN_HEADER_DIR}")
    target_compile_features(ogl_rtx_module PRIVATE cxx_std_17)
    if(CMAKE_DL_LIBS)
        target_link_libraries(ogl_rtx_module PRIVATE ${CMAKE_DL_LIBS})
    endif()
    set_target_properties(ogl_rtx_module PROPERTIES PREFIX "" OUTPUT_NAME "ogl_rtx")

    # Vulkan RT logical-device test; skips gracefully on non-RTX PCs.
    find_library(OGL_VULKAN_RUNTIME NAMES vulkan vulkan-1 vulkan.so.1 libvulkan.so.1)
    if(OGL_VULKAN_RUNTIME)
        add_executable(ogl_vulkan_rt_device_probe rtx/vulkan_device_probe.cpp)
        target_compile_features(ogl_vulkan_rt_device_probe PRIVATE cxx_std_17)
        target_include_directories(ogl_vulkan_rt_device_probe PRIVATE "${OGL_VULKAN_HEADER_DIR}")
        target_link_libraries(ogl_vulkan_rt_device_probe PRIVATE "${OGL_VULKAN_RUNTIME}")
        add_test(NAME ogl_vulkan_rt_device COMMAND ogl_vulkan_rt_device_probe)
        set_tests_properties(ogl_vulkan_rt_device PROPERTIES SKIP_RETURN_CODE 77)

        # Real Vulkan GPU BLAS+TLAS build, with one triangle and one instance.
        # A GPU/driver is optional in CI, so lack of RT is an explicit SKIP.
        add_executable(ogl_vulkan_blas_smoke rtx/vulkan_blas_smoke.cpp)
        target_compile_features(ogl_vulkan_blas_smoke PRIVATE cxx_std_17)
        target_include_directories(ogl_vulkan_blas_smoke PRIVATE
            include "${OGL_VULKAN_HEADER_DIR}")
        target_link_libraries(ogl_vulkan_blas_smoke PRIVATE
            "${OGL_VULKAN_RUNTIME}" ${CMAKE_DL_LIBS})
        add_test(NAME ogl_vulkan_blas_tlas_gpu
                 COMMAND ogl_vulkan_blas_smoke
                 "${CMAKE_CURRENT_SOURCE_DIR}/../../indra/newview/app_settings/ogl/rtx/ray_query_shadow.comp.spv")
        set_tests_properties(ogl_vulkan_blas_tlas_gpu PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 30)
        # Exercise module snapshot ABI -> GPU acceleration structures.
        # Hardware-free runners skip with 77 instead of blocking OpenGL.
        add_test(NAME ogl_vulkan_module_scene_64
                 COMMAND ogl_vulkan_blas_smoke
                 "${CMAKE_CURRENT_SOURCE_DIR}/../../indra/newview/app_settings/ogl/rtx/ray_query_shadow.comp.spv"
                 --module $<TARGET_FILE:ogl_rtx_module> 64)
        add_test(NAME ogl_vulkan_module_scene_1024
                 COMMAND ogl_vulkan_blas_smoke
                 "${CMAKE_CURRENT_SOURCE_DIR}/../../indra/newview/app_settings/ogl/rtx/ray_query_shadow.comp.spv"
                 --module $<TARGET_FILE:ogl_rtx_module> 1024)
        set_tests_properties(ogl_vulkan_module_scene_64 ogl_vulkan_module_scene_1024
                             PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 30)
    endif()

    if(UNIX)
    # GPU hardware/frame-state ABI test. Does not assert actual rays rendered.
    add_executable(ogl_rtx_frame_bridge_test rtx/frame_bridge_test.cpp)
    target_include_directories(ogl_rtx_frame_bridge_test PRIVATE include)
    target_compile_features(ogl_rtx_frame_bridge_test PRIVATE cxx_std_17)
    target_link_libraries(ogl_rtx_frame_bridge_test PRIVATE ${CMAKE_DL_LIBS})
    add_test(NAME ogl_rtx_frame_bridge
             COMMAND ogl_rtx_frame_bridge_test $<TARGET_FILE:ogl_rtx_module>)

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
    endif() # Unix-only dlopen-based tests
else()
    message(STATUS "OGL RTX disabled: Vulkan headers unavailable (OpenGL fallback).")
endif()

enable_testing()
if(UNIX)
    add_test(NAME ogl_module_handshake
             COMMAND ogl_module_probe $<TARGET_FILE:ogl_hello_module>)
endif()

# Pure C++ geometry validation for both the OpenGL collector and Vulkan.
add_executable(ogl_scene_validation_test ../tests/test_scene_validation.cpp)
target_include_directories(ogl_scene_validation_test PRIVATE include)
target_compile_features(ogl_scene_validation_test PRIVATE cxx_std_17)
add_test(NAME ogl_scene_validation COMMAND ogl_scene_validation_test)

# Pure C++ HTTP login allowlist contract. It never opens the network.
add_executable(ogl_grid_http_policy_test ../tests/test_grid_http_policy.cpp)
target_compile_features(ogl_grid_http_policy_test PRIVATE cxx_std_17)
add_test(NAME ogl_grid_http_policy COMMAND ogl_grid_http_policy_test)
