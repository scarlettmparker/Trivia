project(TriviaBackend)
cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED COMPONENTS system filesystem)
include_directories(${Boost_INCLUDE_DIRS})

add_executable(TriviaBackend main.cpp server.cpp ./parser/parser.cpp)
target_link_libraries(TriviaBackend ${Boost_LIBRARIES})