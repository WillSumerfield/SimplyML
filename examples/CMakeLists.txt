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
