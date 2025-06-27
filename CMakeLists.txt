cmake_minimum_required(VERSION 3.10)
project(OpenGLApp LANGUAGES CXX C)

# ソースファイルをワイルドカードで再帰的に取得
# src/ 以下の全てのサブディレクトリにある .cpp と .c ファイルを対象にする
file(GLOB_RECURSE SOURCES "${CMAKE_SOURCE_DIR}/src/*.cpp" "${CMAKE_SOURCE_DIR}/src/*.c")
# もしくは、より明示的に src ディレクトリ以下すべてを対象とする
# file(GLOB_RECURSE SOURCES "${CMAKE_SOURCE_DIR}/src/**/*.cpp" "${CMAKE_SOURCE_DIR}/src/**/*.c")
# 上記の形式は、CMake 3.10以降で推奨されます。
# どちらの形式も動作するはずですが、後者の方が意図が明確です。
# 混乱を避けるため、今回は前者を採用します。

add_executable(${PROJECT_NAME} ${SOURCES})
add_compile_definitions(GLFW_INCLUDE_NONE)



target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/dependencies/include
    # ここも重要：サブディレクトリにヘッダーファイルがある場合、それらのパスも追加する必要があります
    # もしsrc内のサブフォルダにもヘッダーファイルがあるなら、src/subfolder のように追加
    # 例: ${CMAKE_SOURCE_DIR}/src/another_folder
)

target_link_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_SOURCE_DIR}/dependencies/lib
)
target_link_libraries(${PROJECT_NAME} PUBLIC
    glfw3 gdi32 opengl32 user32 kernel32 mingw32 msvcrt m
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build")