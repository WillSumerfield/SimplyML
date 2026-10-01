cmake_minimum_required(VERSION 3.24)
project(SimplyML VERSION 0.1.0 LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON) # static libs end up in the Python extension

option(SIMPLYML_BUILD_TESTS "Build SimplyML tests" ${PROJECT_IS_TOP_LEVEL})
option(SIMPLYML_BUILD_EXAMPLES "Build SimplyML examples" ${PROJECT_IS_TOP_LEVEL})
option(SIMPLYML_BUILD_PYTHON "Build the Python extension (set by pyproject.toml)" OFF)

include(FetchContent)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(cmake/EmbedFile.cmake)

# SFML 3, static -----------------------------------------------------------------------------------
set(BUILD_SHARED_LIBS OFF)
set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
set(SFML_USE_SYSTEM_DEPS OFF CACHE BOOL "" FORCE) # bundle freetype/harfbuzz: self-contained wheel
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.1.0
    GIT_SHALLOW ON
    SYSTEM)
FetchContent_MakeAvailable(SFML)
# SFML's bundled freetype <-> harfbuzz depend on each other, but SFML only restores that link in its
# installed config. Restore it in-tree too so static links resolve.
if(TARGET freetype AND TARGET harfbuzz)
    target_link_libraries(freetype INTERFACE $<BUILD_INTERFACE:harfbuzz>)
endif()

function(simplyml_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

# util: header-only, no SFML -----------------------------------------------------------------------
add_library(simplyml_util INTERFACE)
add_library(SimplyML::util ALIAS simplyml_util)
set_target_properties(simplyml_util PROPERTIES EXPORT_NAME util)
target_include_directories(simplyml_util INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_compile_features(simplyml_util INTERFACE cxx_std_17)
if(WIN32)
    target_compile_definitions(simplyml_util INTERFACE NOMINMAX)
endif()

# core ---------------------------------------------------------------------------------------------
set(SIMPLYML_FONT_CPP ${PROJECT_BINARY_DIR}/generated/default_font.cpp)
embedFile(${PROJECT_SOURCE_DIR}/res/fonts/ShareTechMono-Regular.ttf ${SIMPLYML_FONT_CPP}
    defaultFontData simplyml/core/default_font.hpp)

add_library(simplyml_core STATIC
    src/core/app.cpp
    src/core/camera.cpp
    src/core/event_bus.cpp
    src/core/keys.cpp
    src/core/metric_store.cpp
    src/core/resources.cpp
    ${SIMPLYML_FONT_CPP})
add_library(SimplyML::core ALIAS simplyml_core)
set_target_properties(simplyml_core PROPERTIES EXPORT_NAME core OUTPUT_NAME simplyml-core)
find_package(Threads REQUIRED)
target_link_libraries(simplyml_core PUBLIC simplyml_util SFML::Graphics Threads::Threads)
simplyml_warnings(simplyml_core)

# umbrella -----------------------------------------------------------------------------------------
add_library(simplyml INTERFACE)
add_library(SimplyML::SimplyML ALIAS simplyml)
set_target_properties(simplyml PROPERTIES EXPORT_NAME SimplyML)
target_link_libraries(simplyml INTERFACE simplyml_util simplyml_core)

# python extension --------------------------------------------------------------------------------
if(SIMPLYML_BUILD_PYTHON)
    add_subdirectory(src/python)
endif()

# install / export ---------------------------------------------------------------------------------
install(TARGETS simplyml_util simplyml_core simplyml EXPORT SimplyMLTargets)
install(DIRECTORY include/simplyml DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES LICENSE res/fonts/OFL.txt DESTINATION ${CMAKE_INSTALL_DATADIR}/SimplyML)

set(SIMPLYML_CMAKE_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/SimplyML)
install(EXPORT SimplyMLTargets NAMESPACE SimplyML:: DESTINATION ${SIMPLYML_CMAKE_DIR})
configure_package_config_file(cmake/SimplyMLConfig.cmake.in
    ${PROJECT_BINARY_DIR}/SimplyMLConfig.cmake INSTALL_DESTINATION ${SIMPLYML_CMAKE_DIR})
write_basic_package_version_file(${PROJECT_BINARY_DIR}/SimplyMLConfigVersion.cmake
    COMPATIBILITY SameMinorVersion)
install(FILES ${PROJECT_BINARY_DIR}/SimplyMLConfig.cmake ${PROJECT_BINARY_DIR}/SimplyMLConfigVersion.cmake
    DESTINATION ${SIMPLYML_CMAKE_DIR})

# examples / tests ---------------------------------------------------------------------------------
if(SIMPLYML_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
if(SIMPLYML_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
