
add_executable(example_units main.cpp)
add_dependencies(example_units libscript)
target_include_directories(example_units PUBLIC "../include")
target_link_libraries(example_units libscript)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/units.script
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})