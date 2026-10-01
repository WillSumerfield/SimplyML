FetchContent_Declare(doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.4.12
    GIT_SHALLOW ON
    SYSTEM)
FetchContent_MakeAvailable(doctest)
include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)

# util: must build without SFML
add_executable(test_util
    main.cpp
    util/test_math.cpp
    util/test_format.cpp
    util/test_ring_buffer.cpp
    util/test_easing.cpp
    util/test_smooth_value.cpp)
target_link_libraries(test_util PRIVATE SimplyML::util doctest::doctest)
simplyml_warnings(test_util)
doctest_discover_tests(test_util)

add_executable(test_core
    main.cpp
    core/test_app.cpp
    core/test_camera.cpp
    core/test_clock.cpp
    core/test_default_font.cpp
    core/test_event_bus.cpp
    core/test_metric_store.cpp
    core/test_resources.cpp
    core/test_snapshot.cpp)
target_link_libraries(test_core PRIVATE SimplyML::core doctest::doctest)
simplyml_warnings(test_core)
doctest_discover_tests(test_core PROPERTIES TIMEOUT 60) # App tests open windows

# Package test: install to a temp prefix, then build + run a consumer using find_package(SimplyML).
set(pkg_prefix ${CMAKE_CURRENT_BINARY_DIR}/pkg-install)
add_test(NAME package.install
    COMMAND ${CMAKE_COMMAND} --install ${PROJECT_BINARY_DIR} --prefix ${pkg_prefix} --config $<CONFIG>)
add_test(NAME package.configure
    COMMAND ${CMAKE_COMMAND} -S ${CMAKE_CURRENT_SOURCE_DIR}/package -B ${CMAKE_CURRENT_BINARY_DIR}/pkg-build
        -DCMAKE_PREFIX_PATH=${pkg_prefix} -DCMAKE_BUILD_TYPE=$<CONFIG>
        # forward local udev hints, if any (SFML Window needs udev on Linux)
        $<$<BOOL:${UDEV_PATH_INCLUDES}>:-DUDEV_PATH_INCLUDES=${UDEV_PATH_INCLUDES}>
        $<$<BOOL:${UDEV_PATH_LIB}>:-DUDEV_PATH_LIB=${UDEV_PATH_LIB}>)
add_test(NAME package.build
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR}/pkg-build --config $<CONFIG>)
add_test(NAME package.run COMMAND ${CMAKE_CURRENT_BINARY_DIR}/pkg-build/consumer)
set_tests_properties(package.install PROPERTIES FIXTURES_SETUP pkg_installed)
set_tests_properties(package.configure PROPERTIES FIXTURES_REQUIRED pkg_installed FIXTURES_SETUP pkg_configured)
set_tests_properties(package.build PROPERTIES FIXTURES_REQUIRED pkg_configured FIXTURES_SETUP pkg_built)
set_tests_properties(package.run PROPERTIES FIXTURES_REQUIRED pkg_built)
