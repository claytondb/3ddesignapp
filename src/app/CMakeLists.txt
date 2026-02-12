# Application module - Main entry point

set(APP_SOURCES
    main.cpp
    Application.cpp
)

# Platform-specific executable type
if(APPLE)
    set(MACOSX_BUNDLE_BUNDLE_NAME "DC 3D Design App")
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.dc3ddesignapp")
    set(MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION})
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION})
    
    add_executable(${PROJECT_NAME} MACOSX_BUNDLE
        ${APP_SOURCES}
    )
elseif(WIN32)
    add_executable(${PROJECT_NAME} WIN32
        ${APP_SOURCES}
    )
else()
    add_executable(${PROJECT_NAME}
        ${APP_SOURCES}
    )
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
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
