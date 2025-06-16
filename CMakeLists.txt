cmake_minimum_required(VERSION 3.10.0)
project(os-socket-chat VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -g")
set(LINK_OPTIONS "${LINK_OPTIONS} -lc -lpthread")

# add_compile_definitions(USE_DEBUG_LOGGING)

set(INCLUDE_DIRS
    inc/
)
set(SERVER_SRCS
    src/server.c
    src/log.c
    src/client_list.c
)
set(CLIENT_SRCS
    src/client.c
    src/log.c
)
include_directories(${INCLUDE_DIRS})

add_executable(server ${SERVER_SRCS})
add_executable(client ${CLIENT_SRCS})
