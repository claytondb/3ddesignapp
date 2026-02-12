/**
 * Selection Vertex Shader
 * 
 * Renders selection highlights:
 * - Outlined selected objects
 * - Highlighted selected faces
 * - Selected vertex/edge indicators
 */
#version 410 core

// Vertex attributes
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;

// Uniforms
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;

// Selection-specific uniforms
uniform float outlineScale;      // For outline pass (>1.0)
uniform int selectionMode;       // 0=Object, 1=Face, 2=Vertex, 3=Edge
uniform int highlightPass;       // 0=fill, 1=outline

// Outputs
out vec3 vWorldPosition;
out vec3 vWorldNormal;
out vec2 vTexCoord;
out vec3 vViewPosition;

void main() {
    vec3 pos = position;
    
    // For outline pass, expand along normal
    if (highlightPass == 1 && outlineScale > 0.0) {
        pos = position + normalize(normal) * outlineScale;
    }
    
    // Transform to world space
    vec4 worldPos = model * vec4(pos, 1.0);
    vWorldPosition = worldPos.xyz;
    
    // Transform normal
    vWorldNormal = normalize(normalMatrix * normal);
    
    // Pass through texcoords
    vTexCoord = texCoord;
    
    // View space position
    vec4 viewPos = view * worldPos;
    vViewPosition = viewPos.xyz;
    
    // Clip space
    gl_Position = projection * viewPos;
}
