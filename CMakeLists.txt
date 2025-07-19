cmake_minimum_required(VERSION 3.17)
project(Chess)

set(CMAKE_CXX_STANDARD 17)

include_directories("/opt/homebrew/include")
link_directories("/opt/homebrew/opt/sfml/lib")

add_executable(Chess main.cpp
        Board.h
        BoardState.h)

target_link_libraries(Chess sfml-graphics sfml-window sfml-system)