find_package(Python 3.10 REQUIRED COMPONENTS Interpreter Development.Module)
find_package(nanobind CONFIG REQUIRED)

nanobind_add_module(simplyml_python NB_STATIC module.cpp)
set_target_properties(simplyml_python PROPERTIES OUTPUT_NAME _core)
target_link_libraries(simplyml_python PRIVATE simplyml_core)
install(TARGETS simplyml_python LIBRARY DESTINATION simplyml COMPONENT python)

nanobind_add_stub(simplyml_python_stub
    MODULE _core
    OUTPUT _core.pyi
    PYTHON_PATH $<TARGET_FILE_DIR:simplyml_python>
    DEPENDS simplyml_python)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/_core.pyi DESTINATION simplyml COMPONENT python)
