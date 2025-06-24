cmake_minimum_required(VERSION 3.10) # CMakeのバージョンを指定
project(OpenGLApp LANGUAGES CXX C)   # プロジェクト名と言語を指定

# ソースファイルをワイルドカードで取得
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.c")

# 実行可能ファイルを定義
add_executable(${PROJECT_NAME} ${SOURCES})

# インクルードディレクトリの指定
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/dependencies/include
)

# リンクライブラリの指定
target_link_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_SOURCE_DIR}/dependencies/lib
)
target_link_libraries(${PROJECT_NAME} PUBLIC
    glfw3 gdi32 opengl32 user32 kernel32 mingw32 msvcrt m
)

# ビルド後の実行ファイルの出力先を設定
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build")