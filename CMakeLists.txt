cmake_minimum_required(VERSION 3.9)
project(libscript)

set (CMAKE_CXX_STANDARD 14)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  # disables warning about DLL export 
  # this is annoying when using C++ STL
  add_compile_options(/wd4251)  
endif()

##################################################################
###### coverage build
##################################################################

if(ENABLE_COVERAGE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g ")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftest-coverage")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()


##################################################################
###### feature test
##################################################################

try_compile(HAVE_FILESYSTEM ${CMAKE_BINARY_DIR} "${CMAKE_SOURCE_DIR}/cmake/filesystem.cpp")

if(NOT ${HAVE_FILESYSTEM})
  try_compile(HAVE_FILESYSTEM_TS ${CMAKE_BINARY_DIR} "${CMAKE_SOURCE_DIR}/cmake/filesystem-ts.cpp")
endif()

set(LIBSCRIPT_USE_FILESYSTEM_SUPPORT_LIB TRUE CACHE BOOL "Set to true unless you have the filesystem TS or C++17")

if(${LIBSCRIPT_USE_FILESYSTEM_SUPPORT_LIB})
  add_definitions(-DLIBSCRIPT_USE_FILESYSTEM_SUPPORT_LIB)
elseif(${HAVE_FILESYSTEM} OR ${HAVE_FILESYSTEM_TS})
  message("Either the filesystem library or the filesystem TS will be used")
else()
  message("Neither filesystem or the filesystem TS could be found")
  message("The filesystem support library will be used instead")
  add_definitions(-DLIBSCRIPT_USE_FILESYSTEM_SUPPORT_LIB)
endif()

##################################################################
###### libscript
##################################################################

file(GLOB_RECURSE SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
file(GLOB_RECURSE HDR_FILES 
  ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
)
add_library(libscript ${HDR_FILES} ${SRC_FILES})
target_include_directories(libscript PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
# the following line causes trouble with standard headers like string.h... :(
#target_include_directories(libscript PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include/script")

foreach(_source IN ITEMS ${HDR_FILES} ${SRC_FILES})
    get_filename_component(_source_path "${_source}" PATH)
    file(RELATIVE_PATH _source_path_rel "${CMAKE_CURRENT_SOURCE_DIR}" "${_source_path}")
    string(REPLACE "/" "\\" _group_path "${_source_path_rel}")
    source_group("${_group_path}" FILES "${_source}")
endforeach()

#set(WITH_CONFIG_FILE TRUE CACHE BOOL "Set to true if you want to build libscript with a config file")
#if(${WITH_CONFIG_FILE})
#  add_definitions(-DLIBSCRIPT_HAS_CONFIG)
#endif()

target_compile_definitions(libscript PUBLIC -DLIBSCRIPT_COMPILE_LIBRARY)
#add_definitions(-DLIBSCRIPT_COMPILE_LIBRARY)

##################################################################
###### tests, examples & benchmarks
##################################################################

add_subdirectory(tests)
add_subdirectory(examples)
add_subdirectory(benchmarks)
