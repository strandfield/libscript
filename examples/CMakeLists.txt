
file(GLOB EXAMPLE_HDR_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB EXAMPLE_SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_executable(examples ${EXAMPLE_HDR_FILES} ${EXAMPLE_SRC_FILES})

add_dependencies(examples libscript)
target_include_directories(examples PUBLIC "../include")
target_link_libraries(examples libscript)

file(GLOB EXAMPLE_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/*.script")
file(COPY ${EXAMPLE_SCRIPTS} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

