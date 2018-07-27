
set(GTEST_DIR "" CACHE PATH "Root directory for GoogleTest")

enable_testing()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  add_executable(libscript_tests "${CMAKE_SOURCE_DIR}/vs/debug.natvis" test_arrays.cpp test_coreutils.cpp test_namelookup.cpp test_conversions.cpp test_overloadresolution.cpp test_classes.cpp test_templates.cpp test_lexer.cpp test_parser.cpp test_eval.cpp test_compiler_components.cpp test_compiler.cpp test_compiler_errors.cpp test_modules.cpp test_string.cpp test_namespaces.cpp test_symbols.cpp ${GTEST_DIR}/src/gtest-all.cc ${GTEST_DIR}/src/gtest_main.cc)
else()
  add_executable(libscript_tests test_arrays.cpp test_coreutils.cpp test_namelookup.cpp test_conversions.cpp test_overloadresolution.cpp test_classes.cpp test_templates.cpp test_lexer.cpp test_parser.cpp test_eval.cpp test_compiler_components.cpp test_compiler.cpp test_compiler_errors.cpp test_modules.cpp test_string.cpp test_namespaces.cpp test_symbols.cpp ${GTEST_DIR}/src/gtest-all.cc ${GTEST_DIR}/src/gtest_main.cc)
endif()

add_dependencies(libscript_tests libscript)
target_include_directories(libscript_tests PUBLIC "${GTEST_DIR}/include")
target_include_directories(libscript_tests PRIVATE "${GTEST_DIR}")
target_include_directories(libscript_tests PUBLIC "../include")
target_link_libraries(libscript_tests libscript)
target_compile_definitions(libscript_tests PRIVATE -D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING)


file(GLOB MODULES_TEST_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.m")
file(COPY ${MODULES_TEST_FILES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_test(Testlibscript libscript_tests)