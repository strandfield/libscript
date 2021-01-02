

configure_file(CMakeLists.txt.in googletest-download/CMakeLists.txt)
execute_process(COMMAND "${CMAKE_COMMAND}" -G "${CMAKE_GENERATOR}" .
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/googletest-download")
execute_process(COMMAND "${CMAKE_COMMAND}" --build .
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/googletest-download" )

set(GTEST_DIR "${CMAKE_BINARY_DIR}/googletest-src/googletest" CACHE PATH "Root directory for GoogleTest")


set(SRC_TESTS test_arrays.cpp test_coreutils.cpp test_namelookup.cpp test_overloadresolution.cpp test_templates.cpp test_lexer.cpp test_parser.cpp test_eval.cpp test_compiler_components.cpp test_compiler.cpp test_modules.cpp test_initializer_lists.cpp test_builders.cpp test_scenarios.cpp test_typesystem.cpp test_runtime.cpp ${GTEST_DIR}/src/gtest-all.cc ${GTEST_DIR}/src/gtest_main.cc)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  list(APPEND SRC_TESTS "${LIBSCRIPT_PROJECT_DIR}/vs/libscript.natvis")
endif()

add_executable(TEST_libscript_unit_tests ${SRC_TESTS})

add_dependencies(TEST_libscript_unit_tests libscript)
target_include_directories(TEST_libscript_unit_tests PUBLIC "${GTEST_DIR}/include")
target_include_directories(TEST_libscript_unit_tests PRIVATE "${GTEST_DIR}")
target_include_directories(TEST_libscript_unit_tests PUBLIC "../include")
target_link_libraries(TEST_libscript_unit_tests libscript)
target_compile_definitions(TEST_libscript_unit_tests PRIVATE -D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING)

if (NOT DEFINED WIN32)
  target_link_libraries(TEST_libscript_unit_tests pthread)
endif()

file(GLOB MODULES_TEST_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.m")
file(COPY ${MODULES_TEST_FILES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_test(TEST_libscript_unit_tests TEST_libscript_unit_tests)