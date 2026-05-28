cmake_minimum_required(VERSION 3.20)
project(Chess)

set(CMAKE_CXX_STANDARD 17)

set(SFML_DIR "/opt/homebrew/Cellar/sfml/3.0.2/lib/cmake/SFML")

find_package(SFML 3 REQUIRED COMPONENTS Graphics Window System)

add_executable(Chess main.cpp
        Game/GameRunners.cpp
        AI/Searcher.cpp
        AI/Evaluation.cpp
        Stockfish/StockfishPlayer.cpp
        Game/GameManager.cpp
        Game/Board.cpp
)

target_link_libraries(Chess
        SFML::Graphics
        SFML::Window
        SFML::System
)
