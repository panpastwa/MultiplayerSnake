cmake_minimum_required(VERSION 3.10)
project(MultiplayerSnake)

set(CMAKE_CXX_STANDARD 14)

set(SFML_INCLUDE_DIR "lib/SFML-2.5.1/include")
set(SFML_LIBRARY_DIR "lib/SFML-2.5.1/lib")
set(SFML_DIR "lib/SFML-2.5.1/lib/cmake/SFML")

link_directories(SFML_LIBRARY_DIR)
include_directories(SFML_INCLUDE_DIR)

find_package(SFML 2.5.1 COMPONENTS system window graphics network audio)

if(SFML_FOUND)
    message(STATUS "SFML_INCLUDE_DIR: ${SFML_INCLUDE_DIR}")
    message(STATUS "SFML_LIBRARIES: ${SFML_LIBRARIES}")
    message(STATUS "SFML_VERSION: ${SFML_VERSION}")
endif()

add_executable(ServerApp src/server/main.cpp src/server/server.cpp src/server/server.h src/server/classes/Player.h src/server/classes/Point.h src/server/classes/Score.h)
target_link_libraries(ServerApp sfml-graphics sfml-window sfml-system pthread)

add_executable(ClientApp src/client/main.cpp src/client/client.cpp src/client/client.h)
target_link_libraries(ClientApp sfml-graphics sfml-window sfml-system pthread)
