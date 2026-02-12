/**
 * Grid Vertex Shader
 * 
 * Renders ground plane grid lines with distance-based fading.
 * Supports both finite grid geometry and infinite grid projection.
 */
#version 410 core

layout(location = 0) in vec3 position;

// Uniforms
uniform mat4 mvp;           // Model-View-Projection matrix
uniform vec3 cameraPos;     // Camera world position

// Outputs
out float vDistance;        // Distance from camera for fading
out vec3 vWorldPos;         // World position for shader effects

void main() {
    vWorldPos = position;
    
    // Calculate horizontal distance from camera (XZ plane)
    vec2 horizontalPos = position.xz - cameraPos.xz;
    vDistance = length(horizontalPos);
    
    // Transform to clip space
    gl_Position = mvp * vec4(position, 1.0);
}
