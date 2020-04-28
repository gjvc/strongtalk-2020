
#
# src/cmake/spdlog-download.cmake -----------------------------------------
#
#

cmake_minimum_required( VERSION 3.0 )

project( spdlog-download NONE )
set( CMAKE_SYSTEM_NAME Windows )
include( ExternalProject )

ExternalProject_Add( spdlog
  GIT_REPOSITORY    https://github.com/gabime/spdlog.git
  GIT_TAG           master
  SOURCE_DIR        "${CMAKE_CURRENT_BINARY_DIR}/spdlog-src"
  BINARY_DIR        "${CMAKE_CURRENT_BINARY_DIR}/spdlog-build"
  CONFIGURE_COMMAND ""
  BUILD_COMMAND     ""
  INSTALL_COMMAND   ""
  TEST_COMMAND      ""
  CMAKE_TOOLCHAIN_FILE=
)

