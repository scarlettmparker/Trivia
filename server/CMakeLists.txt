cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0048 NEW)

project(TriviaBackend VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -Wall -Wextra")

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
  set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
endif()

# set the env environment in config
include(${CMAKE_CURRENT_SOURCE_DIR}/env/load_env.cmake)
function(check_env_file_changed ENV_FILE CHANGED_VAR)
  if(NOT EXISTS ${ENV_FILE})
    set(${CHANGED_VAR} TRUE PARENT_SCOPE)
    return()
  endif()

  file(TIMESTAMP ${ENV_FILE} CURRENT_TIMESTAMP)
  set(TIMESTAMP_FILE "${CMAKE_CURRENT_BINARY_DIR}/env_timestamp.txt")
  
  if(EXISTS ${TIMESTAMP_FILE})
    file(READ ${TIMESTAMP_FILE} PREVIOUS_TIMESTAMP)
    if(NOT "${PREVIOUS_TIMESTAMP}" STREQUAL "${CURRENT_TIMESTAMP}")
      set(${CHANGED_VAR} TRUE PARENT_SCOPE)
    else()
      set(${CHANGED_VAR} FALSE PARENT_SCOPE)
    endif()
  else()
    set(${CHANGED_VAR} TRUE PARENT_SCOPE)
  endif()
  file(WRITE ${TIMESTAMP_FILE} ${CURRENT_TIMESTAMP})
endfunction()

if("$ENV{TRIVIA_PRIVATE}" STREQUAL "TRUE")
  set(ENV_FILE ${CMAKE_CURRENT_SOURCE_DIR}/env/.env.private)
else()
  set(ENV_FILE ${CMAKE_CURRENT_SOURCE_DIR}/env/.env.public)
endif()

# check for env file changes
check_env_file_changed(${ENV_FILE} ENV_CHANGED)
if(ENV_CHANGED)
  message("Environment file changed, forcing config.h regeneration")
  file(REMOVE ${CMAKE_CURRENT_BINARY_DIR}/env/config.h)
endif()

configure_file(${ENV_FILE} ${CMAKE_CURRENT_BINARY_DIR}/env_file_dependency COPYONLY)
load_env_file(${ENV_FILE})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/env/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/env/config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/env ${Boost_INCLUDE_DIRS} ${LIBPQ_INCLUDE_DIRS})

set(PCH_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/pch.hpp")
set(PCH_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/pch.cpp")

file(WRITE ${PCH_SOURCE} "#include \"${PCH_HEADER}\"\n")

add_library(pch STATIC ${PCH_SOURCE})
target_precompile_headers(pch PRIVATE ${PCH_HEADER})

#boost
find_package(Boost REQUIRED COMPONENTS system filesystem)
include_directories(${Boost_INCLUDE_DIRS})

#libpqxx
find_library(LIBPQXX_LIB NAMES pqxx PATHS /usr/local/lib)
if (NOT LIBPQXX_LIB)
    message(FATAL_ERROR "libpqxx not found")
endif()

#libpq
include_directories(/usr/local/include)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBPQ REQUIRED libpq)
include_directories(${LIBPQ_INCLUDE_DIRS})

file(GLOB API_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/api/*.cpp")

foreach(SOURCE_FILE ${API_SOURCES})
  get_filename_component(LIB_NAME ${SOURCE_FILE} NAME_WE)
  add_library(${LIB_NAME} SHARED ${SOURCE_FILE}
    server.cpp request/postgres.cpp request/request.cpp parser/parser.cpp
  )
  set_target_properties(${LIB_NAME} PROPERTIES OUTPUT_NAME ${LIB_NAME} LIBRARY_OUTPUT_DIRECTORY "../api")
  target_link_libraries(${LIB_NAME} ${Boost_LIBRARIES} ${LIBPQXX_LIB} ${LIBPQ_LIBRARIES} pch)
endforeach()

add_executable(TriviaBackend main.cpp server.cpp request/postgres.cpp)
target_link_libraries(TriviaBackend ${Boost_LIBRARIES} ${LIBPQXX_LIB} ${LIBPQ_LIBRARIES} pch)

set_target_properties(TriviaBackend PROPERTIES LINK_FLAGS "-rdynamic")
set_target_properties(TriviaBackend PROPERTIES ENABLE_EXPORTS ON)
foreach(SOURCE_FILE ${API_SOURCES})
  get_filename_component(LIB_NAME ${SOURCE_FILE} NAME_WE)
  target_link_libraries(${LIB_NAME} TriviaBackend)
endforeach()