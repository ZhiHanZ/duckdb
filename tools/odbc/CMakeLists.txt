cmake_minimum_required(VERSION 3.12.0)
find_package(ODBC)

if(NOT ODBC_FOUND)
  message(FATAL_ERROR No ODBC found)
endif()

set(LINK_LIB_LIST "")

if(ODBC_FOUND)
  find_library(ODBCINST_LIB NAMES odbcinst libodbcinst)
  if(ODBCINST_LIB)
    list(APPEND LINK_LIB_LIST ${ODBCINST_LIB})
    add_definitions(-DODBC_LINK_ODBCINST)
    message(STATUS "Found ODBCINST_LIB: " ${ODBCINST_LIB})
  endif()
endif()

add_definitions(-DNO_FRAMEWORKS)
include_directories(${ODBC_INCLUDE_DIRS} include)

add_library(
  duckdb_odbc SHARED
  duckdb_odbc.cpp
  driver.cpp
  connection.cpp
  statement.cpp
  prepared.cpp
  result.cpp
  api_info.cpp
  statement_functions.cpp
  odbc_interval.cpp
  odbc_fetch.cpp
  parameter_wrapper.cpp)

set_target_properties(duckdb_odbc PROPERTIES DEFINE_SYMBOL "DUCKDB_ODBC_API")

target_link_libraries(duckdb_odbc ${LINK_LIB_LIST} duckdb_static)
