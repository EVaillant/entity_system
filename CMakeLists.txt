cmake_minimum_required(VERSION 3.0)
project(entity_system VERSION 0.0.1)

set(PROJECT_DESCRIPTION "Entity System C++ (c++14)")

set(ENTITY_SYSTEM_TESTS_DIR "${CMAKE_SOURCE_DIR}/tests/")
set(ENTITY_SYSTEM_DEMOS_DIR "${CMAKE_SOURCE_DIR}/demos/")
set(CMAKE_CXX_FLAGS         "${CMAKE_CXX_FLAGS} -Wall -Wextra -ggdb -std=c++1y")
set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG}   -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
set(CMAKE_MODULE_PATH       "${CMAKE_SOURCE_DIR}/cmake/")

if( NOT CMAKE_BUILD_TYPE )
  set( CMAKE_BUILD_TYPE Debug CACHE STRING  "Choose the type of build, options are: Debug Release" FORCE )
endif()

if(NOT DISABLE_UNITTEST)
  set(DISABLE_UNITTEST OFF CACHE BOOL "Disable unittest" FORCE)
endif()

if(NOT DISABLE_DEMOS)
  set(DISABLE_DEMOS OFF CACHE BOOL "Disable demos" FORCE)
endif()

enable_testing()

#
# include dir
include_directories(
    include
    ${CMAKE_CURRENT_BINARY_DIR}
)

#
# UNITTEST 
if(NOT DISABLE_UNITTEST)
  include(FindBoost)

  find_package(
    Boost
    1.50.0
    COMPONENTS unit_test_framework
  )
  
  if (NOT Boost_FOUND)
    message(SEND_ERROR "Could not find boost")
  endif ()

  add_subdirectory(tests)
endif ()

#
# DEMOS
if(NOT DISABLE_DEMOS)
  find_package(PkgConfig)
  pkg_check_modules(ALLEGRO5 REQUIRED allegro-5 allegro_main-5 allegro_image-5 allegro_color-5 allegro_primitives-5)
  add_subdirectory(demos)
endif ()

#
# doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
    ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    @ONLY
  )
  add_custom_target(
    doc
    ${DOXYGEN_EXECUTABLE}
    ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen" VERBATIM
  )
endif(DOXYGEN_FOUND)


#
# configure file
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/entity_system.pc.in ${CMAKE_CURRENT_BINARY_DIR}/entity_system.pc @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h @ONLY)

#
# install
install(DIRECTORY include/ DESTINATION include)
install(FILES "${CMAKE_BINARY_DIR}/entity_system.pc" DESTINATION lib/pkgconfig)
