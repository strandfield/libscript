

configure_file(CMakeLists.txt.in googletest-download/CMakeLists.txt)
execute_process(COMMAND "${CMAKE_COMMAND}" -G "${CMAKE_GENERATOR}" .
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/googletest-download")
execute_process(COMMAND "${CMAKE_COMMAND}" --build .
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/googletest-download" )

set(GTEST_DIR "${CMAKE_BINARY_DIR}/googletest-src/googletest" CACHE PATH "Root directory for GoogleTest")

enable_testing()

set(SRC_TESTS test_arrays.cpp test_coreutils.cpp test_namelookup.cpp test_conversions.cpp test_overloadresolution.cpp test_classes.cpp test_templates.cpp test_lexer.cpp test_parser.cpp test_eval.cpp test_compiler_components.cpp test_compiler.cpp test_compiler_errors.cpp test_modules.cpp test_string.cpp test_namespaces.cpp test_symbols.cpp test_initializer_lists.cpp test_builders.cpp test_scenarios.cpp test_typesystem.cpp ${GTEST_DIR}/src/gtest-all.cc ${GTEST_DIR}/src/gtest_main.cc)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  list(APPEND SRC_TESTS "${CMAKE_SOURCE_DIR}/vs/libscript.natvis")
endif()

add_executable(tests ${SRC_TESTS})

add_dependencies(tests libscript)
target_include_directories(tests PUBLIC "${GTEST_DIR}/include")
target_include_directories(tests PRIVATE "${GTEST_DIR}")
target_include_directories(tests PUBLIC "../include")
target_link_libraries(tests libscript)
target_compile_definitions(tests PRIVATE -D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING)

if (NOT DEFINED WIN32)
  target_link_libraries(tests pthread)
endif()

file(GLOB MODULES_TEST_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.m")
file(COPY ${MODULES_TEST_FILES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_test(Testlibscript tests)