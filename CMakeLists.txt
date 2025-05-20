cmake_minimum_required(VERSION 3.10)
project(igmp_client C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# flags
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror")

# output to /bin
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

include_directories(include)
file(GLOB SRC_FILES "src/*.c")
list(REMOVE_ITEM SRC_FILES "${CMAKE_SOURCE_DIR}/src/gen_query.c")

add_executable(igmp_client ${SRC_FILES})
add_executable(gen_query src/gen_query.c)

