# Sketch UI module sources

set(SKETCH_SOURCES
    SketchMode.cpp
    SketchToolbox.cpp
    SketchViewport.cpp
    SketchTool.cpp
    LineTool.cpp
    ArcTool.cpp
    SplineTool.cpp
    DimensionTool.cpp
)

set(SKETCH_HEADERS
    SketchMode.h
    SketchToolbox.h
    SketchViewport.h
    SketchTool.h
    LineTool.h
    ArcTool.h
    SplineTool.h
    DimensionTool.h
)

# Add to parent scope
set(UI_SKETCH_SOURCES ${SKETCH_SOURCES} PARENT_SCOPE)
set(UI_SKETCH_HEADERS ${SKETCH_HEADERS} PARENT_SCOPE)
