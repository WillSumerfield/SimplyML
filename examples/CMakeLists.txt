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
