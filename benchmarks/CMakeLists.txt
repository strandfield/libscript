
set(LIBSCRIPT_WITH_BENCHMARKS FALSE CACHE BOOL "Check if you want to build the benchmarks")

if(LIBSCRIPT_WITH_BENCHMARKS)

configure_file(CMakeLists.txt.in googlebenchmark-download/CMakeLists.txt)
execute_process(COMMAND "${CMAKE_COMMAND}" -G "${CMAKE_GENERATOR}" .
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/googlebenchmark-download")
execute_process(COMMAND "${CMAKE_COMMAND}" --build .
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/googlebenchmark-download" )

set(GBENCHMARK_INCLUDES "${CMAKE_BINARY_DIR}/googlebenchmark-src/include" CACHE PATH "Google Benchmark include directory")

#add_subdirectory("${CMAKE_BINARY_DIR}/googlebenchmark-src/" "${CMAKE_BINARY_DIR}/googlebenchmark-src/build")

file(GLOB_RECURSE GBENCH_SRC_FILES "${CMAKE_BINARY_DIR}/googlebenchmark-src/src/*.cc")
file(GLOB_RECURSE GBENCH_HDR_FILES 
  "${CMAKE_BINARY_DIR}/googlebenchmark-src/include/*.h"
  "${CMAKE_BINARY_DIR}/googlebenchmark-src/src/*.h"
)
add_library(google-benchmark ${GBENCH_HDR_FILES} ${GBENCH_SRC_FILES}) 
target_include_directories(google-benchmark PRIVATE ${GBENCHMARK_INCLUDES})

add_executable(benchmarks benchmarks.cpp)
target_link_libraries(benchmarks google-benchmark)

target_include_directories(benchmarks PRIVATE ${GBENCHMARK_INCLUDES})
target_include_directories(benchmarks PUBLIC "../include")

target_link_libraries(benchmarks libscript)

if(WIN32)
target_link_libraries(benchmarks "shlwapi.lib")
endif()

endif()


