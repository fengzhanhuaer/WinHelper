cmake_minimum_required(VERSION 3.20)
project(WinHelper LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
    add_compile_options(/W4 /utf-8)
endif()

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/third_party/imgui)

add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
    ${IMGUI_DIR}/backends/imgui_impl_dx11.cpp
)

target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

target_link_libraries(imgui PUBLIC
    d3d11
    dxgi
    d3dcompiler
)

add_executable(WinHelper WIN32
    src/main.cpp
    src/matrix_rain.cpp
    src/tab_sysinfo.cpp
    src/tab_quickaction.cpp
    src/tab_virtualgps.cpp
    src/tab_about.cpp
)

target_include_directories(WinHelper PRIVATE
    src
)

target_link_libraries(WinHelper PRIVATE
    imgui
    shlwapi
)

# Copy scripts to build directory
file(COPY ${CMAKE_SOURCE_DIR}/scripts/
     DESTINATION ${CMAKE_BINARY_DIR}/scripts/
     FILES_MATCHING PATTERN "*.bat")

# Note: Driver build requires WDK and must be built separately using Visual Studio
# Driver files are located in driver/ directory
# Use Visual Studio with WDK to build the UMDF2 driver
