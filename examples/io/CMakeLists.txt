
add_executable(example_io main.cpp)
add_dependencies(example_io libscript)
target_include_directories(example_io PUBLIC "../include")
target_link_libraries(example_io libscript)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/io.script
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})