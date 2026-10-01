add_executable(threaded_training cpp/threaded_training/main.cpp)
target_link_libraries(threaded_training PRIVATE SimplyML::SimplyML)
simplyml_warnings(threaded_training)

add_executable(layout cpp/layout/main.cpp)
target_link_libraries(layout PRIVATE SimplyML::SimplyML)
simplyml_warnings(layout)

add_executable(dashboard cpp/dashboard/main.cpp)
target_link_libraries(dashboard PRIVATE SimplyML::SimplyML)
simplyml_warnings(dashboard)

add_executable(controls cpp/controls/main.cpp)
target_link_libraries(controls PRIVATE SimplyML::SimplyML)
simplyml_warnings(controls)

add_executable(network_view cpp/network_view/main.cpp)
target_link_libraries(network_view PRIVATE SimplyML::SimplyML)
simplyml_warnings(network_view)

add_executable(cart_pendulum cpp/cart_pendulum/main.cpp)
target_link_libraries(cart_pendulum PRIVATE SimplyML::SimplyML)
simplyml_warnings(cart_pendulum)

# The original Pendulum-NEAT app (NEAT + pbd physics), ported onto SimplyML. Its own code keeps the
# original style, so the library's warning flags aren't applied.
add_executable(pendulum_neat
    pendulum_neat/main.cpp
    pendulum_neat/dashboard.cpp
    pendulum_neat/training/trainer.cpp)
target_include_directories(pendulum_neat PRIVATE pendulum_neat)
target_compile_definitions(pendulum_neat PRIVATE PENDULUM_RES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/pendulum_neat/res")
target_link_libraries(pendulum_neat PRIVATE SimplyML::SimplyML)
