/**
 * Line Vertex Shader
 * 
 * Basic line/wireframe rendering with per-vertex color support.
 * Used for:
 * - Coordinate axes
 * - Selection outlines
 * - Wireframe overlay
 * - Debug visualization
 */
#version 410 core

// Vertex attributes
layout(location = 0) in vec3 position;
layout(location = 3) in vec4 color;

// Uniforms
uniform mat4 mvp;           // Model-View-Projection matrix
uniform mat4 model;         // Model matrix (for world-space calcs)
uniform bool useUniformColor;
uniform vec4 uniformColor;

// Outputs
out vec4 vColor;
out vec3 vWorldPos;

void main() {
    // World position
    vWorldPos = (model * vec4(position, 1.0)).xyz;
    
    // Pass color
    if (useUniformColor) {
        vColor = uniformColor;
    } else {
        vColor = color;
    }
    
    // Transform to clip space
    gl_Position = mvp * vec4(position, 1.0);
}
