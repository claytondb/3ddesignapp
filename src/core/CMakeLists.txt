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
    
    # Commands subdirectory
    Commands/DeleteCommand.cpp
    Commands/DeleteCommand.h
    Commands/ImportMeshCommand.cpp
    Commands/ImportMeshCommand.h
    Commands/MeshEditCommand.cpp
    Commands/MeshEditCommand.h
    Commands/TransformCommand.cpp
    Commands/TransformCommand.h
)

add_library(dc3d_core STATIC
    ${CORE_SOURCES}
)

target_include_directories(dc3d_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Commands
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# Add GLM include directory if found manually (not via package)
if(GLM_INCLUDE_DIR AND NOT GLM_INCLUDE_DIR STREQUAL "")
    target_include_directories(dc3d_core PUBLIC ${GLM_INCLUDE_DIR})
endif()

target_link_libraries(dc3d_core PUBLIC
    Qt6::Core
    Qt6::Widgets  # For QUndoCommand
    Qt6::OpenGLWidgets  # For QOpenGLWidget in Viewport.h
)

# Link glm::glm target if found via package
if(TARGET glm::glm)
    target_link_libraries(dc3d_core PUBLIC glm::glm)
endif()
