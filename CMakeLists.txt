cmake_minimum_required(VERSION 3.20)
project(Chess)

set(CMAKE_CXX_STANDARD 17)

set(SFML_DIR "/opt/homebrew/Cellar/sfml/3.0.2/lib/cmake/SFML")

find_package(SFML 3 REQUIRED COMPONENTS Graphics Window System)

add_executable(Chess main.cpp)

target_link_libraries(Chess
        SFML::Graphics
        SFML::Window
        SFML::System
)

include(CTest)

if(BUILD_TESTING)
        add_executable(IconSheetTest tests/icon_sheet_test.cpp)
        target_compile_definitions(IconSheetTest PRIVATE CHESS_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
        target_link_libraries(IconSheetTest
                SFML::Graphics
                SFML::System
        )
        add_test(NAME IconSheetTest COMMAND IconSheetTest)

        add_executable(IconBoardPreview tests/icon_board_preview.cpp)
        target_compile_definitions(IconBoardPreview PRIVATE CHESS_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
        target_link_libraries(IconBoardPreview
                SFML::Graphics
                SFML::Window
                SFML::System
        )
endif()
