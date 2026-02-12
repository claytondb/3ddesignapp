/**
 * Mesh Vertex Shader
 * 
 * Basic mesh rendering with per-vertex lighting preparation.
 * Outputs world-space position and normal for fragment shader lighting.
 */
#version 410 core

// Vertex attributes
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in vec4 color;

// Uniforms
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;  // transpose(inverse(model)) for non-uniform scaling

// Outputs to fragment shader
out vec3 vWorldPosition;
out vec3 vWorldNormal;
out vec2 vTexCoord;
out vec4 vVertexColor;
out vec3 vViewPosition;  // Position in view space for specular

void main() {
    // Transform to world space
    vec4 worldPos = model * vec4(position, 1.0);
    vWorldPosition = worldPos.xyz;
    
    // Transform normal to world space
    vWorldNormal = normalize(normalMatrix * normal);
    
    // Pass through texture coordinates and vertex color
    vTexCoord = texCoord;
    vVertexColor = color;
    
    // View space position for specular calculations
    vec4 viewPos = view * worldPos;
    vViewPosition = viewPos.xyz;
    
    // Final clip-space position
    gl_Position = projection * viewPos;
}
