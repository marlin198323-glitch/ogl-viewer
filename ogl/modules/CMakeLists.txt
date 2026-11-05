cmake_minimum_required(VERSION 3.18)
project(ogl_module_prototype VERSION 0.1.0 LANGUAGES CXX)

# ABI handshake remains available on Linux, Windows, and macOS.
if(NOT WIN32 AND NOT UNIX)
    message(FATAL_ERROR "OGL core modules require Windows, Linux or macOS")
endif()

add_library(ogl_hello_module SHARED examples/hello/hello_module.cpp)
target_include_directories(ogl_hello_module PRIVATE include)
target_compile_features(ogl_hello_module PRIVATE cxx_std_17)
set_target_properties(ogl_hello_module PROPERTIES PREFIX "" OUTPUT_NAME "ogl_hello")

# All OSes must load the same trusted C-ABI test module.
add_executable(ogl_module_probe tools/module_probe.cpp)
target_include_directories(ogl_module_probe PRIVATE include)
target_compile_features(ogl_module_probe PRIVATE cxx_std_17)
if(CMAKE_DL_LIBS)
    target_link_libraries(ogl_module_probe PRIVATE ${CMAKE_DL_LIBS})
endif()

# Portable CPU dispatch foundation. An optimized kernel must be selected at
# runtime, never inferred from "AMD"/"Intel" or from the developer's CPU.
add_executable(ogl_cpu_probe tools/cpu_probe.cpp)
target_include_directories(ogl_cpu_probe PRIVATE include)
target_compile_features(ogl_cpu_probe PRIVATE cxx_std_17)
add_executable(ogl_cpu_features_test ../tests/test_cpu_features.cpp)
target_include_directories(ogl_cpu_features_test PRIVATE include)
target_compile_features(ogl_cpu_features_test PRIVATE cxx_std_17)
# These foundation binaries MUST remain runnable on baseline CPUs. Never
# silently compile them with -march=native or global AVX2 optimization.
if((CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang") AND
   CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64|i[3-6]86)$")
    target_compile_options(ogl_cpu_probe PRIVATE -march=x86-64)
    target_compile_options(ogl_cpu_features_test PRIVATE -march=x86-64)
endif()
add_test(NAME ogl_cpu_dispatch_safety COMMAND ogl_cpu_features_test)
add_test(NAME ogl_cpu_capability_report COMMAND ogl_cpu_probe)

# Reuse the exact CPU backends and compile flags for tests and Viewer.
include("${CMAKE_CURRENT_LIST_DIR}/cpu/ogl_cpu_buffer_targets.cmake")
ogl_add_cpu_buffer_target(ogl_cpu_buffer)
add_executable(ogl_cpu_buffer_test ../tests/test_cpu_buffer.cpp)
target_link_libraries(ogl_cpu_buffer_test PRIVATE ogl_cpu_buffer)
target_compile_features(ogl_cpu_buffer_test PRIVATE cxx_std_17)
add_test(NAME ogl_cpu_buffer_dispatch_correctness COMMAND ogl_cpu_buffer_test)
add_executable(ogl_fastcache_compare_test ../tests/test_fastcache_compare.cpp)
target_link_libraries(ogl_fastcache_compare_test PRIVATE ogl_cpu_buffer)
target_compile_features(ogl_fastcache_compare_test PRIVATE cxx_std_17)
add_test(NAME ogl_fastcache_duplicate_guard COMMAND ogl_fastcache_compare_test)
add_executable(ogl_fastcache_microbenchmark tools/fastcache_benchmark.cpp)
target_link_libraries(ogl_fastcache_microbenchmark PRIVATE ogl_cpu_buffer)
target_compile_features(ogl_fastcache_microbenchmark PRIVATE cxx_std_17)
# Microbenchmarks are never pass/fail CI wall-time thresholds.

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
    find_package(Threads REQUIRED)
    target_link_libraries(ogl_rtx_module PRIVATE Threads::Threads)
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

        # Strictly opt-in GPU scene probe. Shared library loading is
        # separated from the always-available OpenGL capability module.
        add_library(ogl_gpu_probe SHARED rtx/vulkan_blas_smoke.cpp)
        target_compile_definitions(ogl_gpu_probe PRIVATE OGL_GPU_PROBE_LIBRARY=1)
        target_compile_features(ogl_gpu_probe PRIVATE cxx_std_17)
        target_include_directories(ogl_gpu_probe PRIVATE
            include "${OGL_VULKAN_HEADER_DIR}")
        target_link_libraries(ogl_gpu_probe PRIVATE
            "${OGL_VULKAN_RUNTIME}" ${CMAKE_DL_LIBS})
        set_target_properties(ogl_gpu_probe PROPERTIES
            PREFIX "" OUTPUT_NAME "ogl_gpu_probe")

        # Development-only runtime layout. Never touch a user's settings.
        if(UNIX)
            add_executable(ogl_dynamic_registry_test rtx/ogl_dynamic_registry_test.cpp)
            target_include_directories(ogl_dynamic_registry_test PRIVATE include)
            target_compile_features(ogl_dynamic_registry_test PRIVATE cxx_std_17)
            target_link_libraries(ogl_dynamic_registry_test PRIVATE ${CMAKE_DL_LIBS})
            add_test(NAME ogl_live_uuid_object_registry
                COMMAND ${CMAKE_COMMAND} -E env OGL_RTX_SCENE_PROBE=1
                    $<TARGET_FILE:ogl_dynamic_registry_test>
                    $<TARGET_FILE:ogl_rtx_module>)
            set_tests_properties(ogl_live_uuid_object_registry
                PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 15)
            add_executable(ogl_gpu_transform_test rtx/ogl_gpu_transform_test.cpp)
            target_include_directories(ogl_gpu_transform_test PRIVATE include)
            target_compile_features(ogl_gpu_transform_test PRIVATE cxx_std_17)
            target_link_libraries(ogl_gpu_transform_test PRIVATE ${CMAKE_DL_LIBS})
            add_test(NAME ogl_vulkan_tlas_instance_refit
                COMMAND ogl_gpu_transform_test
                    $<TARGET_FILE:ogl_gpu_probe>
                    "${CMAKE_CURRENT_SOURCE_DIR}/../../indra/newview/app_settings/ogl/rtx/ray_query_shadow.comp.spv")
            set_tests_properties(ogl_vulkan_tlas_instance_refit
                PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 30)
            add_executable(ogl_dense_sun_shadow_test
                rtx/ogl_dense_sun_shadow_test.cpp)
            target_include_directories(ogl_dense_sun_shadow_test PRIVATE include)
            target_compile_features(ogl_dense_sun_shadow_test PRIVATE cxx_std_17)
            target_link_libraries(ogl_dense_sun_shadow_test PRIVATE ${CMAKE_DL_LIBS})
            add_test(NAME ogl_gpu_dense_depth_guided_sun_shadow
                COMMAND ogl_dense_sun_shadow_test
                    $<TARGET_FILE:ogl_gpu_probe>
                    "${CMAKE_CURRENT_SOURCE_DIR}/../../indra/newview/app_settings/ogl/rtx/dense_depth_guided_sun.comp.spv")
            set_tests_properties(ogl_gpu_dense_depth_guided_sun_shadow
                PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 30)
            add_executable(ogl_depth_sun_shadow_test
                rtx/ogl_depth_sun_shadow_test.cpp)
            target_include_directories(ogl_depth_sun_shadow_test PRIVATE include)
            target_compile_features(ogl_depth_sun_shadow_test PRIVATE cxx_std_17)
            target_link_libraries(ogl_depth_sun_shadow_test PRIVATE ${CMAKE_DL_LIBS})
            add_test(NAME ogl_gpu_depth_guided_sun_shadow
                COMMAND ogl_depth_sun_shadow_test
                    $<TARGET_FILE:ogl_gpu_probe>
                    "${CMAKE_CURRENT_SOURCE_DIR}/../../indra/newview/app_settings/ogl/rtx/depth_guided_sun.comp.spv")
            set_tests_properties(ogl_gpu_depth_guided_sun_shadow
                PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 30)
            add_executable(ogl_camera_shadow_test
                rtx/ogl_camera_shadow_test.cpp)
            target_include_directories(ogl_camera_shadow_test PRIVATE include)
            target_compile_features(ogl_camera_shadow_test PRIVATE cxx_std_17)
            target_link_libraries(ogl_camera_shadow_test PRIVATE ${CMAKE_DL_LIBS})
            add_test(NAME ogl_gpu_perspective_sun_shadow
                COMMAND ogl_camera_shadow_test
                    $<TARGET_FILE:ogl_gpu_probe>
                    "${CMAKE_CURRENT_SOURCE_DIR}/../../indra/newview/app_settings/ogl/rtx/camera_sun_shadow.comp.spv")
            set_tests_properties(ogl_gpu_perspective_sun_shadow
                PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 30)
            add_executable(ogl_gpu_persistent_test
                rtx/ogl_gpu_persistent_test.cpp)
            target_include_directories(ogl_gpu_persistent_test PRIVATE include)
            target_compile_features(ogl_gpu_persistent_test PRIVATE cxx_std_17)
            target_link_libraries(ogl_gpu_persistent_test PRIVATE ${CMAKE_DL_LIBS})
            add_test(NAME ogl_gpu_device_persistent_and_scene_cache
                COMMAND ogl_gpu_persistent_test
                    $<TARGET_FILE:ogl_gpu_probe>
                    "${CMAKE_CURRENT_SOURCE_DIR}/../../indra/newview/app_settings/ogl/rtx/ray_query_shadow.comp.spv")
            set_tests_properties(ogl_gpu_device_persistent_and_scene_cache
                PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 30)
            add_custom_command(TARGET ogl_gpu_probe POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory
                    "${CMAKE_CURRENT_BINARY_DIR}/lib"
                    "${CMAKE_CURRENT_BINARY_DIR}/app_settings/ogl/rtx"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<TARGET_FILE:ogl_gpu_probe>"
                    "${CMAKE_CURRENT_BINARY_DIR}/lib/ogl_gpu_probe.so"
                VERBATIM)
            # Stale SPIR-V was the root cause of missing depth-match guards.
            # Source shaders must stage even when the compiled GPU C++ code
            # has not changed (i.e. on incremental CMake builds).
            add_custom_target(ogl_rtx_stage_shaders ALL
                COMMAND ${CMAKE_COMMAND} -E make_directory
                    "${CMAKE_CURRENT_BINARY_DIR}/app_settings/ogl/rtx"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CMAKE_CURRENT_SOURCE_DIR}/../../indra/newview/app_settings/ogl/rtx/ray_query_shadow.comp.spv"
                    "${CMAKE_CURRENT_BINARY_DIR}/app_settings/ogl/rtx/ray_query_shadow.comp.spv"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CMAKE_CURRENT_SOURCE_DIR}/../../indra/newview/app_settings/ogl/rtx/camera_sun_shadow.comp.spv"
                    "${CMAKE_CURRENT_BINARY_DIR}/app_settings/ogl/rtx/camera_sun_shadow.comp.spv"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CMAKE_CURRENT_SOURCE_DIR}/../../indra/newview/app_settings/ogl/rtx/depth_guided_sun.comp.spv"
                    "${CMAKE_CURRENT_BINARY_DIR}/app_settings/ogl/rtx/depth_guided_sun.comp.spv"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CMAKE_CURRENT_SOURCE_DIR}/../../indra/newview/app_settings/ogl/rtx/dense_depth_guided_sun.comp.spv"
                    "${CMAKE_CURRENT_BINARY_DIR}/app_settings/ogl/rtx/dense_depth_guided_sun.comp.spv"
                VERBATIM)
            add_dependencies(ogl_depth_sun_shadow_test ogl_rtx_stage_shaders)
            add_dependencies(ogl_dense_sun_shadow_test ogl_rtx_stage_shaders ogl_gpu_probe)
            add_executable(ogl_live_gpu_probe_test
                rtx/ogl_live_gpu_probe_test.cpp)
            target_include_directories(ogl_live_gpu_probe_test PRIVATE include)
            target_compile_features(ogl_live_gpu_probe_test PRIVATE cxx_std_17)
            target_link_libraries(ogl_live_gpu_probe_test PRIVATE ${CMAKE_DL_LIBS})
            add_test(NAME ogl_live_opt_in_gpu_probe
                COMMAND ${CMAKE_COMMAND} -E env OGL_RTX_SCENE_PROBE=1
                    $<TARGET_FILE:ogl_live_gpu_probe_test>
                    $<TARGET_FILE:ogl_rtx_module>)
            set_tests_properties(ogl_live_opt_in_gpu_probe PROPERTIES TIMEOUT 30)
            add_test(NAME ogl_live_opt_in_camera_sun_shadow
                COMMAND ${CMAKE_COMMAND} -E env
                    OGL_RTX_SCENE_PROBE=1 OGL_RTX_CAMERA_PROBE=1
                    $<TARGET_FILE:ogl_live_gpu_probe_test>
                    $<TARGET_FILE:ogl_rtx_module>)
            set_tests_properties(ogl_live_opt_in_camera_sun_shadow
                PROPERTIES TIMEOUT 30)
            add_test(NAME ogl_live_camera_reprojection_refresh
                COMMAND ${CMAKE_COMMAND} -E env
                    OGL_RTX_SCENE_PROBE=1 OGL_RTX_CAMERA_PROBE=1
                    OGL_RTX_DEV_INTERVAL_MS=15
                    $<TARGET_FILE:ogl_live_gpu_probe_test>
                    $<TARGET_FILE:ogl_rtx_module> --test-camera-refresh)
            set_tests_properties(ogl_live_camera_reprojection_refresh
                PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 15)
            add_test(NAME ogl_live_depth_guided_gpu_bridge
                COMMAND ${CMAKE_COMMAND} -E env
                    OGL_RTX_SCENE_PROBE=1 OGL_RTX_CAMERA_PROBE=1
                    OGL_RTX_DEPTH_PROBE=1 OGL_RTX_DEPTH_GUIDED_PROBE=1
                    OGL_RTX_DEV_INTERVAL_MS=15
                    $<TARGET_FILE:ogl_live_gpu_probe_test>
                    $<TARGET_FILE:ogl_rtx_module> --test-depth-guided)
            set_tests_properties(ogl_live_depth_guided_gpu_bridge
                PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 15)
            add_test(NAME ogl_live_dense_gpu_world_bridge
                COMMAND ${CMAKE_COMMAND} -E env
                    OGL_RTX_SCENE_PROBE=1 OGL_RTX_CAMERA_PROBE=1
                    OGL_RTX_DENSE_PREVIEW=1
                    $<TARGET_FILE:ogl_live_gpu_probe_test>
                    $<TARGET_FILE:ogl_rtx_module> --test-dense-guided)
            set_tests_properties(ogl_live_dense_gpu_world_bridge
                PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 15)
            add_test(NAME ogl_live_dense_gpu_moving_world
                COMMAND ${CMAKE_COMMAND} -E env
                    OGL_RTX_SCENE_PROBE=1 OGL_RTX_CAMERA_PROBE=1
                    OGL_RTX_DENSE_PREVIEW=1
                    $<TARGET_FILE:ogl_live_gpu_probe_test>
                    $<TARGET_FILE:ogl_rtx_module> --test-dense-drift)
            set_tests_properties(ogl_live_dense_gpu_moving_world
                PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 15)
            file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/missing_gpu_probe")
            add_test(NAME ogl_live_gpu_probe_missing_library
                COMMAND ${CMAKE_COMMAND} -E env OGL_RTX_SCENE_PROBE=1
                    $<TARGET_FILE:ogl_live_gpu_probe_test>
                    $<TARGET_FILE:ogl_rtx_module> --no-gpu-library)
            set_tests_properties(ogl_live_gpu_probe_missing_library
                PROPERTIES TIMEOUT 30
                WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/missing_gpu_probe")
        endif()

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
        add_test(NAME ogl_vulkan_module_scene_4096
                 COMMAND ogl_vulkan_blas_smoke
                 "${CMAKE_CURRENT_SOURCE_DIR}/../../indra/newview/app_settings/ogl/rtx/ray_query_shadow.comp.spv"
                 --module $<TARGET_FILE:ogl_rtx_module> 4096)
        add_test(NAME ogl_vulkan_module_scene_8192
                 COMMAND ogl_vulkan_blas_smoke
                 "${CMAKE_CURRENT_SOURCE_DIR}/../../indra/newview/app_settings/ogl/rtx/ray_query_shadow.comp.spv"
                 --module $<TARGET_FILE:ogl_rtx_module> 8192)
        set_tests_properties(ogl_vulkan_module_scene_64 ogl_vulkan_module_scene_1024
                             ogl_vulkan_module_scene_4096 ogl_vulkan_module_scene_8192
                             PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 30)
    endif()

    if(UNIX)
    # GPU hardware/frame-state ABI test. Does not assert actual rays rendered.
    add_executable(ogl_rtx_frame_bridge_test rtx/frame_bridge_test.cpp)
    target_include_directories(ogl_rtx_frame_bridge_test PRIVATE include)
    target_compile_features(ogl_rtx_frame_bridge_test PRIVATE cxx_std_17)
    target_link_libraries(ogl_rtx_frame_bridge_test PRIVATE ${CMAKE_DL_LIBS})
    # Only native X11 QA links Xlib, not Windows/macOS or the Viewer.
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        find_package(X11 QUIET)
        if(X11_FOUND)
            target_include_directories(ogl_rtx_frame_bridge_test PRIVATE ${X11_INCLUDE_DIR})
            target_link_libraries(ogl_rtx_frame_bridge_test PRIVATE ${X11_LIBRARIES})
            target_compile_definitions(ogl_rtx_frame_bridge_test PRIVATE OGL_TEST_X11_WSI=1)
        endif()
    endif()
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
add_test(NAME ogl_module_handshake
         COMMAND ogl_module_probe $<TARGET_FILE:ogl_hello_module>)
set_tests_properties(ogl_module_handshake PROPERTIES TIMEOUT 15)

# Async queue contract is GPU- and vendor-independent: latest-wins,
# no render-thread blocking, no callbacks after module unload.
find_package(Threads REQUIRED)
add_executable(ogl_async_gpu_queue_test ../tests/test_async_gpu_queue.cpp)
target_include_directories(ogl_async_gpu_queue_test PRIVATE include)
target_compile_features(ogl_async_gpu_queue_test PRIVATE cxx_std_17)
target_link_libraries(ogl_async_gpu_queue_test PRIVATE Threads::Threads)
add_test(NAME ogl_async_gpu_queue COMMAND ogl_async_gpu_queue_test)
set_tests_properties(ogl_async_gpu_queue PROPERTIES TIMEOUT 10)

# Projection ABI tests run on every platform without any graphics hardware.
add_executable(ogl_camera_frame_validation_test
    ../tests/test_camera_frame_validation.cpp)
target_include_directories(ogl_camera_frame_validation_test PRIVATE include)
target_compile_features(ogl_camera_frame_validation_test PRIVATE cxx_std_17)
add_test(NAME ogl_camera_frame_validation
    COMMAND ogl_camera_frame_validation_test)

# CPU reconstruction agrees with the depth-guided Vulkan shader and
# explains zero-shadow results by separating background from surfaces.
add_executable(ogl_depth_diagnostics_test ../tests/test_depth_diagnostics.cpp)
target_include_directories(ogl_depth_diagnostics_test PRIVATE include)
target_compile_features(ogl_depth_diagnostics_test PRIVATE cxx_std_17)
add_test(NAME ogl_depth_reconstruction_and_coverage
    COMMAND ogl_depth_diagnostics_test)

# Full-view deferred depth grid validation needs no GPU or GL driver.
add_executable(ogl_depth_grid_validation_test ../tests/test_depth_grid_validation.cpp)
target_include_directories(ogl_depth_grid_validation_test PRIVATE include)
target_compile_features(ogl_depth_grid_validation_test PRIVATE cxx_std_17)
add_test(NAME ogl_depth_grid_validation COMMAND ogl_depth_grid_validation_test)

# Terrain patch geometry validation is portable and GPU independent.
add_executable(ogl_terrain_patch_test ../tests/test_terrain_patch.cpp)
target_include_directories(ogl_terrain_patch_test PRIVATE include)
target_compile_features(ogl_terrain_patch_test PRIVATE cxx_std_17)
add_test(NAME ogl_terrain_patch_geometry COMMAND ogl_terrain_patch_test)

# Stable per-object identity and independent geometry/pose revisions.
add_executable(ogl_dynamic_scene_test ../tests/test_dynamic_scene.cpp)
target_include_directories(ogl_dynamic_scene_test PRIVATE include)
target_compile_features(ogl_dynamic_scene_test PRIVATE cxx_std_17)
add_test(NAME ogl_dynamic_scene_updates COMMAND ogl_dynamic_scene_test)

# Deterministic bounded selection over full OpenSim mesh-face index ranges.
add_executable(ogl_scene_sampling_test ../tests/test_scene_sampling.cpp)
target_include_directories(ogl_scene_sampling_test PRIVATE include)
target_compile_features(ogl_scene_sampling_test PRIVATE cxx_std_17)
add_test(NAME ogl_scene_priority_and_mesh_sampling COMMAND ogl_scene_sampling_test)
add_executable(ogl_scene_snapshot_policy_test ../tests/test_scene_snapshot_policy.cpp)
target_include_directories(ogl_scene_snapshot_policy_test PRIVATE include)
target_compile_features(ogl_scene_snapshot_policy_test PRIVATE cxx_std_17)
add_test(NAME ogl_partial_scene_snapshot_policy COMMAND ogl_scene_snapshot_policy_test)
add_executable(ogl_scene_depth_compare_test ../tests/test_scene_depth_compare.cpp)
target_include_directories(ogl_scene_depth_compare_test PRIVATE include)
target_compile_features(ogl_scene_depth_compare_test PRIVATE cxx_std_17)
add_test(NAME ogl_independent_cpu_depth_reference COMMAND ogl_scene_depth_compare_test)
add_executable(ogl_scene_ray_visibility_test ../tests/test_scene_ray_visibility.cpp)
target_include_directories(ogl_scene_ray_visibility_test PRIVATE include)
target_compile_features(ogl_scene_ray_visibility_test PRIVATE cxx_std_17)
add_test(NAME ogl_exact_camera_ray_geometry_selection COMMAND ogl_scene_ray_visibility_test)

# Startup diagnostic geometry must not wait minutes for 180 frames
# while the OpenSim scene is slowly loading at sub-1-FPS.
add_executable(ogl_scene_scan_schedule_test ../tests/test_scene_scan_schedule.cpp)
target_include_directories(ogl_scene_scan_schedule_test PRIVATE include)
target_compile_features(ogl_scene_scan_schedule_test PRIVATE cxx_std_17)
add_test(NAME ogl_scene_startup_scan_schedule COMMAND ogl_scene_scan_schedule_test)

# GPU->world shadow input must reject stale camera, scene and depth,
# independently of the future OpenGL compositor.
add_executable(ogl_world_shadow_policy_test ../tests/test_world_shadow_policy.cpp)
target_include_directories(ogl_world_shadow_policy_test PRIVATE include)
target_compile_features(ogl_world_shadow_policy_test PRIVATE cxx_std_17)
add_test(NAME ogl_world_shadow_frame_policy COMMAND ogl_world_shadow_policy_test)

# Pure C++ geometry validation for both the OpenGL collector and Vulkan.
add_executable(ogl_scene_validation_test ../tests/test_scene_validation.cpp)
target_include_directories(ogl_scene_validation_test PRIVATE include)
target_compile_features(ogl_scene_validation_test PRIVATE cxx_std_17)
add_test(NAME ogl_scene_validation COMMAND ogl_scene_validation_test)

# Pure C++ HTTP login allowlist contract. It never opens the network.
add_executable(ogl_grid_http_policy_test ../tests/test_grid_http_policy.cpp)
target_compile_features(ogl_grid_http_policy_test PRIVATE cxx_std_17)
add_test(NAME ogl_grid_http_policy COMMAND ogl_grid_http_policy_test)
