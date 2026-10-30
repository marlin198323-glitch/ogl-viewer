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

enable_testing()
add_test(
  NAME ogl_module_handshake
  COMMAND ogl_module_probe $<TARGET_FILE:ogl_hello_module>
)
