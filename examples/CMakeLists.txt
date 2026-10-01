add_executable(threaded_training cpp/threaded_training/main.cpp)
target_link_libraries(threaded_training PRIVATE SimplyML::SimplyML)
simplyml_warnings(threaded_training)

add_executable(layout cpp/layout/main.cpp)
target_link_libraries(layout PRIVATE SimplyML::SimplyML)
simplyml_warnings(layout)
