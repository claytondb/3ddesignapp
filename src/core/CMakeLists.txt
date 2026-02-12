# Core module - Central services shared across all modules

add_library(dc3d_core STATIC
    SceneManager.cpp
)

target_include_directories(dc3d_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(dc3d_core PUBLIC
    Qt6::Core
)
