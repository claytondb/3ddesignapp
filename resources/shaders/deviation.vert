/**
 * @file deviation.vert
 * @brief Vertex shader for deviation colormap visualization
 * 
 * Transforms vertices and passes deviation values to fragment shader.
 */

#version 410 core

// Vertex attributes
layout(location = 0) in vec3 a_Position;    // Vertex position
layout(location = 1) in vec3 a_Normal;      // Vertex normal
layout(location = 2) in float a_Deviation;  // Per-vertex deviation value

// Uniforms
uniform mat4 u_MVP;           // Model-View-Projection matrix
uniform mat4 u_Model;         // Model matrix
uniform mat4 u_NormalMatrix;  // Normal transformation matrix (transpose inverse of model)
uniform float u_MinVal;       // Minimum deviation value
uniform float u_MaxVal;       // Maximum deviation value

// Outputs to fragment shader
out vec3 v_Normal;            // Normal in world space
out vec3 v_WorldPos;          // Position in world space
out float v_Deviation;        // Raw deviation value
out float v_NormalizedDev;    // Deviation normalized to [0, 1]

void main() {
    // Transform position
    vec4 worldPos = u_Model * vec4(a_Position, 1.0);
    v_WorldPos = worldPos.xyz;
    gl_Position = u_MVP * vec4(a_Position, 1.0);
    
    // Transform normal to world space
    v_Normal = normalize(mat3(u_NormalMatrix) * a_Normal);
    
    // Pass through deviation
    v_Deviation = a_Deviation;
    
    // Normalize deviation to [0, 1] range for colormap lookup
    float range = u_MaxVal - u_MinVal;
    if (range > 0.0001) {
        v_NormalizedDev = clamp((a_Deviation - u_MinVal) / range, 0.0, 1.0);
    } else {
        // Handle zero range (all same value)
        v_NormalizedDev = 0.5;
    }
}
