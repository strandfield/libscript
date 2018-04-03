
set(GTEST_DIR "" CACHE PATH "Root directory for GoogleTest")

enable_testing()

add_executable(libscript_tests test_coreutils.cpp test_namelookup.cpp test_conversions.cpp test_overloadresolution.cpp test_classes.cpp test_templates.cpp test_lexer.cpp test_parser.cpp test_eval.cpp test_compiler.cpp test_compiler_errors.cpp ${GTEST_DIR}/src/gtest-all.cc ${GTEST_DIR}/src/gtest_main.cc)
add_dependencies(libscript_tests libscript)
target_include_directories(libscript_tests PUBLIC "${GTEST_DIR}/include")
target_include_directories(libscript_tests PRIVATE "${GTEST_DIR}")
target_include_directories(libscript_tests PUBLIC "../include")
target_link_libraries(libscript_tests libscript)


add_test(Testlibscript libscript_tests)