
#
# src/cmake/g3log-download.cmake -----------------------------------------
#
#

cmake_minimum_required( VERSION 3.0 )

project( g3log-download NONE )
set( CMAKE_SYSTEM_NAME Windows )
include( ExternalProject )

ExternalProject_Add( g3log
  GIT_REPOSITORY    https://github.com/KjellKod/g3log.git
  GIT_TAG           master
  SOURCE_DIR        "${CMAKE_CURRENT_BINARY_DIR}/g3log-src"
  BINARY_DIR        "${CMAKE_CURRENT_BINARY_DIR}/g3log-build"
  CONFIGURE_COMMAND ""
  BUILD_COMMAND     ""
  INSTALL_COMMAND   ""
  TEST_COMMAND      ""
  CMAKE_TOOLCHAIN_FILE=
)

