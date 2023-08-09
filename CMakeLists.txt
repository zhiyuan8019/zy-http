cmake_minimum_required(VERSION 3.10)
set(CMAKE_C_COMPILER "/home/zhenyang/gcc-13.1.0/bin/gcc")
set(CMAKE_CXX_COMPILER "/home/zhenyang/gcc-13.1.0/bin/g++")

project(zy-http)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_BUILD_TYPE Release)


include_directories(include)

file(GLOB SOURCE_FILE src/*.cpp)

add_executable(zy-http ${SOURCE_FILE})


if(CMAKE_BUILD_TYPE STREQUAL Debug)
  target_compile_options(zy-http PRIVATE -Wall -Wextra)
  target_link_libraries(zy-http PRIVATE asan ubsan uring)
  target_compile_options(zy-http PRIVATE -fno-omit-frame-pointer -fsanitize=address -fsanitize=undefined)
else()
  target_compile_options(zy-http PRIVATE -Wall -Wextra -O3)
  target_link_libraries(zy-http PRIVATE uring)

endif()



