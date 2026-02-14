/**
 * Selection Vertex Shader
 * 
 * Renders selection highlights:
 * - Outlined selected objects (smooth screen-space outline)
 * - Highlighted selected faces
 * - Selected vertex/edge indicators
 * - Hover highlighting before click
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
uniform vec2 viewportSize;       // For screen-space outline

// Selection-specific uniforms
uniform float outlineScale;      // For outline pass (pixels in screen space)
uniform int selectionMode;       // 0=Object, 1=Face, 2=Vertex, 3=Edge
uniform int highlightPass;       // 0=fill, 1=outline
uniform bool isHover;            // Hover highlight (pre-selection feedback)

// Outputs
out vec3 vWorldPosition;
out vec3 vWorldNormal;
out vec2 vTexCoord;
out vec3 vViewPosition;
out float vOutlineAlpha;

void main() {
    vec3 pos = position;
    vec3 norm = normalize(normal);
    vOutlineAlpha = 1.0;
    
    // Transform to world and view space first
    vec4 worldPos = model * vec4(pos, 1.0);
    vec4 viewPos = view * worldPos;
    vec4 clipPos = projection * viewPos;
    
    // For outline pass, expand in screen space for consistent width
    if (highlightPass == 1 && outlineScale > 0.0) {
        // Transform normal to view space
        vec3 viewNormal = normalize(mat3(view) * normalMatrix * norm);
        
        // Project normal to screen space
        vec2 screenNormal = normalize(viewNormal.xy);
        
        // Compute screen-space offset (consistent pixel width)
        float pixelScale = outlineScale * 2.0 / viewportSize.y;
        float zScale = clipPos.w * pixelScale;
        
        // Offset in clip space
        clipPos.xy += screenNormal * zScale;
        
        // Fade outline at glancing angles for smoother appearance
        float facing = abs(dot(normalize(viewNormal), vec3(0.0, 0.0, 1.0)));
        vOutlineAlpha = smoothstep(0.0, 0.3, facing);
    }
    
    vWorldPosition = worldPos.xyz;
    vWorldNormal = normalize(normalMatrix * norm);
    vTexCoord = texCoord;
    vViewPosition = viewPos.xyz;
    
    gl_Position = clipPos;
}
