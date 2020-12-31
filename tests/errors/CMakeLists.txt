
set(SRC_TESTS main.cpp)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  list(APPEND SRC_TESTS "${LIBSCRIPT_PROJECT_DIR}/vs/libscript.natvis")
endif()

add_executable(TEST_libscript_errors_tests ${SRC_TESTS})

add_dependencies(TEST_libscript_errors_tests libscript)
target_include_directories(TEST_libscript_errors_tests PUBLIC "../../include")
target_link_libraries(TEST_libscript_errors_tests libscript)
target_compile_definitions(TEST_libscript_errors_tests PRIVATE -D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING)

file(GLOB LIBSCRIPT_LANGUAGE_TEST_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.script")
file(COPY ${LIBSCRIPT_LANGUAGE_TEST_FILES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

file(WRITE "test-list.h" "")
foreach(f IN ITEMS ${LIBSCRIPT_LANGUAGE_TEST_FILES})
  message("${f}")
  file(APPEND "test-list.h" "\"${f}\",\n") 
endforeach()

add_test(TEST_libscript_errors_tests TEST_libscript_errors_tests)