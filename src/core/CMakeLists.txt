# Core module - Central services shared across all modules

set(CORE_SOURCES
    SceneManager.cpp
    SceneManager.h
    Selection.cpp
    Selection.h
    IntegrationController.cpp
    IntegrationController.h
    Command.h
    CommandStack.cpp
    CommandStack.h
)

add_library(dc3d_core STATIC
    ${CORE_SOURCES}
)

target_include_directories(dc3d_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(dc3d_core PUBLIC
    Qt6::Core
)
