cmake_minimum_required(VERSION 3.16)

project(teacup
    VERSION 0.1.0
    DESCRIPTION "Bubbletea-inspired functional TUI library for C++17"
    LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakeDependentOption)

set(TEACUP_IS_TOP_LEVEL OFF)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(TEACUP_IS_TOP_LEVEL ON)
endif()

option(TEACUP_BUILD_EXAMPLES "Build teacup example applications" ${TEACUP_IS_TOP_LEVEL})
option(TEACUP_BUILD_TESTS "Build teacup unit tests" ${TEACUP_IS_TOP_LEVEL})
option(TEACUP_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

add_library(teacup
    src/canvas.cpp
    src/color.cpp
    src/input_decoder.cpp
    src/layout.cpp
    src/program.cpp
    src/renderer.cpp
    src/style.cpp
    src/surface.cpp
    src/terminal_posix.cpp
    src/utf8.cpp
    src/widgets_text.cpp
)
add_library(teacup::teacup ALIAS teacup)

target_compile_features(teacup PUBLIC cxx_std_17)
set_target_properties(teacup PROPERTIES
    CXX_EXTENSIONS OFF
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})

target_include_directories(teacup
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

find_package(Threads REQUIRED)
target_link_libraries(teacup PUBLIC Threads::Threads)

if(MSVC)
    target_compile_options(teacup PRIVATE /W4 /permissive- $<$<BOOL:${TEACUP_WARNINGS_AS_ERRORS}>:/WX>)
else()
    target_compile_options(teacup PRIVATE
        -Wall -Wextra -Wpedantic -Wshadow -Wconversion
        $<$<BOOL:${TEACUP_WARNINGS_AS_ERRORS}>:-Werror>)
endif()

if(TEACUP_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(TEACUP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

install(TARGETS teacup EXPORT teacupTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT teacupTargets
    FILE teacupTargets.cmake
    NAMESPACE teacup::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/teacup)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/teacupConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/teacupConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/teacup)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/teacupConfigVersion.cmake
    COMPATIBILITY SameMajorVersion)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/teacupConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/teacupConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/teacup)
