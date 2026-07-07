cmake_minimum_required(VERSION 3.16)
project(fps_mvp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# OpenCV is used ONLY as: window display + framebuffer blit + 2D overlay draw.
find_package(OpenCV REQUIRED)
# OpenMP parallelizes the per-column raycaster loop (columns are independent).
find_package(OpenMP)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.cpp)

add_executable(fps_mvp ${SOURCES})
target_include_directories(fps_mvp PRIVATE ${CMAKE_SOURCE_DIR}/src ${OpenCV_INCLUDE_DIRS})
target_link_libraries(fps_mvp PRIVATE ${OpenCV_LIBS})
if(OpenMP_CXX_FOUND)
  target_link_libraries(fps_mvp PRIVATE OpenMP::OpenMP_CXX)
endif()

# Win32 native input (GetAsyncKeyState / mouse-lock) + winmm (timeBeginPeriod).
if(WIN32)
  target_link_libraries(fps_mvp PRIVATE user32 winmm)
endif()

if(MSVC)
  target_compile_options(fps_mvp PRIVATE /O2)
else()
  target_compile_options(fps_mvp PRIVATE -O3 -ffast-math)
endif()
