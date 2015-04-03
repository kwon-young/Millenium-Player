cmake_minimum_required(VERSION 2.8)
project(Millenium-Player)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
include_directories(./include/)
add_definitions(-DSFML_STATIC)
file(GLOB_RECURSE SRC_LIST "src/*")
file(GLOB_RECURSE INCLUDE_LIST "include/*")
source_group(${PROJECT_NAME}  FILES ${SRC_LIST} ${INCLUDE_LIST})
add_executable(${PROJECT_NAME} ${SRC_LIST} ${INCLUDE_LIST})

target_link_libraries(
  ${PROJECT_NAME}
  optimized tools-sfml
  debug tools-sfml-d
  optimized sfml-graphics-s
  optimized sfml-window-s
  optimized sfml-system-s
  debug sfml-graphics-s-d
  debug sfml-window-s-d
  debug sfml-system-s-d
  general vorbisfile
  general vorbis
  general ogg
  general ws2_32
  general freetype
  general glew32
  general opengl32
  general winmm
  general gdi32
  general jpeg
  debug rtaudio_static
)
