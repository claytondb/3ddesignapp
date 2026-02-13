# Application module - Main entry point

set(APP_SOURCES
    main.cpp
    Application.cpp
    Application.h
    ImportCommand.cpp
    ImportCommand.h
)

# Resources
set(RESOURCES
    ${CMAKE_SOURCE_DIR}/resources/resources.qrc
)

# Platform-specific executable type
if(APPLE)
    set(MACOSX_BUNDLE_BUNDLE_NAME "DC 3D Design App")
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.dc3ddesignapp")
    set(MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION})
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION})
    
    add_executable(${PROJECT_NAME} MACOSX_BUNDLE
        ${APP_SOURCES}
        ${RESOURCES}
    )
elseif(WIN32)
    add_executable(${PROJECT_NAME} WIN32
        ${APP_SOURCES}
        ${RESOURCES}
    )
else()
    add_executable(${PROJECT_NAME}
        ${APP_SOURCES}
        ${RESOURCES}
    )
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    dc3d_core
    dc3d_geometry
    dc3d_io
    dc3d_renderer
    dc3d_ui
)

# Installation
include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Windows: Deploy Qt DLLs
if(WIN32)
    find_program(WINDEPLOYQT windeployqt HINTS "${Qt6_DIR}/../../../bin")
    if(WINDEPLOYQT)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${WINDEPLOYQT}
                --no-translations
                --no-system-d3d-compiler
                $<TARGET_FILE:${PROJECT_NAME}>
        )
    endif()
endif()

# macOS: Deploy Qt frameworks
if(APPLE)
    find_program(MACDEPLOYQT macdeployqt HINTS "${Qt6_DIR}/../../../bin")
    if(MACDEPLOYQT)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${MACDEPLOYQT}
                $<TARGET_BUNDLE_DIR:${PROJECT_NAME}>
                -always-overwrite
        )
    endif()
endif()
