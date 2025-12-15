cmake_minimum_required(VERSION 3.16)
project(pong_net)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")

set(PONG_SERVER "pong_server")

file(GLOB COMMON_SOURCES src/game/*.cpp src/net/*.cpp)

file(GLOB SERVER_SOURCES ${COMMON_SOURCES}
    src/pong_server.cpp)

file(GLOB CLIENT_SOURCES ${COMMON_SOURCES}
    src/pong_client.cpp)

add_executable(${PONG_SERVER} ${SERVER_SOURCES})
add_executable(${PROJECT_NAME} ${CLIENT_SOURCES})

target_include_directories(${PONG_SERVER} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_subdirectory(vendored/SDL_net EXCLUDE_FROM_ALL)

find_package(SDL3   REQUIRED)

# for linux
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

target_link_libraries(${PONG_SERVER} PRIVATE
    SDL3::SDL3
    SDL3_net::SDL3_net
    Threads::Threads
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL3::SDL3
    SDL3_net::SDL3_net
    Threads::Threads
)

# copy dll lib to output dir on windows
if(WIN32 AND TARGET SDL3::SDL3)
    set(SDL_DLL_TARGETS
        SDL3::SDL3
    )

    foreach(DLL_TARGET IN LISTS SDL_DLL_TARGETS)
        if(TARGET ${DLL_TARGET})
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                # 动态获取DLL完整路径
                $<TARGET_FILE:${DLL_TARGET}>
                ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
                COMMENT "Copying ${DLL_TARGET} to output dir"
            )
        endif()
    endforeach()
endif()
