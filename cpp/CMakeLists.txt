cmake_minimum_required(VERSION 3.16)
project(radar_hazard_pipeline LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(radar_hazard_lib
    src/bufr_decoder.cpp
    src/geo_utils.cpp
    src/cell_grid.cpp
    src/cluster_analyzer.cpp
    src/echo_tops.cpp
    src/contour_merger.cpp
    src/image_renderer.cpp
    src/config.cpp
    src/json.cpp
)

target_include_directories(radar_hazard_lib PUBLIC include)

add_executable(radar_hazard_app src/main.cpp)

target_link_libraries(radar_hazard_app PRIVATE radar_hazard_lib)

install(TARGETS radar_hazard_app RUNTIME DESTINATION bin)
