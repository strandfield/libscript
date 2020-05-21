
set(SRC_TESTS main.cpp)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  list(APPEND SRC_TESTS "${CMAKE_SOURCE_DIR}/vs/libscript.natvis")
endif()

add_executable(TEST_libscript_language_tests ${SRC_TESTS})

add_dependencies(TEST_libscript_language_tests libscript)
target_include_directories(TEST_libscript_language_tests PUBLIC "../../include")
target_link_libraries(TEST_libscript_language_tests libscript)
target_compile_definitions(TEST_libscript_language_tests PRIVATE -D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING)

file(GLOB LIBSCRIPT_LANGUAGE_TEST_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.script")
file(COPY ${LIBSCRIPT_LANGUAGE_TEST_FILES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_test(TEST_libscript_language_tests TEST_libscript_language_tests)