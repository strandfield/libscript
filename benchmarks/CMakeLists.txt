
set(LIBSCRIPT_WITH_BENCHMARKS FALSE CACHE BOOL "Check if you want to build the benchmarks")


if(LIBSCRIPT_WITH_BENCHMARKS)

set(GBENCHMARK_INCLUDES "" CACHE PATH "Google Benchmark include directory")
set(GBENCHMARK_LIB_DEBUG "" CACHE FILEPATH "Path to Google Benchmark library (debug)")
set(GBENCHMARK_LIB_RELEASE "" CACHE FILEPATH "Path to Google Benchmark library (release)")

add_executable(benchmarks benchmarks.cpp)

target_include_directories(benchmarks PRIVATE ${GBENCHMARK_INCLUDES})
target_include_directories(benchmarks PUBLIC "../include")

target_link_libraries(benchmarks libscript)
target_link_libraries(benchmarks debug ${GBENCHMARK_LIB_DEBUG})
target_link_libraries(benchmarks general ${GBENCHMARK_LIB_RELEASE})

if(WIN32)
target_link_libraries(benchmarks "shlwapi.lib")
endif()

endif()


