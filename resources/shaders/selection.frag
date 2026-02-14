/**
 * Selection Fragment Shader
 * 
 * Renders selection highlights with various modes:
 * - Solid color tint for selected faces
 * - Smooth outline for selected objects
 * - Point/line rendering for vertices/edges
 * - Hover highlighting (pre-selection feedback)
 * 
 * Color scheme:
 * - Object selection: Orange outline (#FF9500)
 * - Face selection: Blue tint with semi-transparency
 * - Vertex selection: Green points
 * - Edge selection: Yellow lines
 * - Hover: Subtle white overlay
 */
#version 410 core

// Inputs from vertex shader
in vec3 vWorldPosition;
in vec3 vWorldNormal;
in vec2 vTexCoord;
in vec3 vViewPosition;
in float vOutlineAlpha;

// Selection uniforms
uniform int selectionMode;       // 0=Object, 1=Face, 2=Vertex, 3=Edge
uniform int highlightPass;       // 0=fill, 1=outline
uniform vec4 highlightColor;     // Selection highlight color
uniform float opacity;           // Overall opacity
uniform vec3 cameraPosition;     // For fresnel effects
uniform bool isHover;            // Hover highlight (pre-selection)

// X-ray mode
uniform bool xrayMode;           // Draw through other geometry

// Output
out vec4 fragColor;

// Color constants - refined for professional look
const vec4 OBJECT_COLOR = vec4(1.0, 0.58, 0.0, 1.0);      // Orange #FF9500
const vec4 OBJECT_HOVER = vec4(1.0, 0.75, 0.4, 0.6);      // Lighter orange for hover
const vec4 FACE_COLOR = vec4(0.3, 0.6, 1.0, 0.5);         // Blue with alpha
const vec4 FACE_HOVER = vec4(0.5, 0.75, 1.0, 0.3);        // Light blue hover
const vec4 VERTEX_COLOR = vec4(0.2, 1.0, 0.3, 1.0);       // Green
const vec4 EDGE_COLOR = vec4(1.0, 0.9, 0.2, 1.0);         // Yellow

void main() {
    vec4 color;
    
    // Choose base color based on selection mode and hover state
    if (selectionMode == 0) {
        // Object mode
        if (isHover) {
            color = OBJECT_HOVER;
        } else {
            color = (highlightColor.a > 0.0) ? highlightColor : OBJECT_COLOR;
        }
    } else if (selectionMode == 1) {
        // Face mode
        if (isHover) {
            color = FACE_HOVER;
        } else {
            color = (highlightColor.a > 0.0) ? highlightColor : FACE_COLOR;
        }
    } else if (selectionMode == 2) {
        // Vertex mode
        color = (highlightColor.a > 0.0) ? highlightColor : VERTEX_COLOR;
    } else {
        // Edge mode
        color = (highlightColor.a > 0.0) ? highlightColor : EDGE_COLOR;
    }
    
    // Apply pass-specific modifications
    if (highlightPass == 1) {
        // Outline pass - apply smooth outline alpha from vertex shader
        color.a *= vOutlineAlpha;
        
        // Softer outline for hover
        if (isHover) {
            color.a *= 0.7;
        }
    } else {
        // Fill pass
        if (selectionMode == 0) {
            // Object fill: subtle overlay
            color.a *= isHover ? 0.1 : 0.15;
            
            // Add fresnel rim effect for better depth perception
            vec3 viewDir = normalize(cameraPosition - vWorldPosition);
            vec3 normal = normalize(vWorldNormal);
            float fresnel = pow(1.0 - max(dot(viewDir, normal), 0.0), 3.0);
            color.rgb = mix(color.rgb, vec3(1.0), fresnel * 0.4);
            color.a += fresnel * 0.2;
        } else if (selectionMode == 1) {
            // Face fill: moderate transparency with fresnel
            color.a *= isHover ? 0.4 : 0.6;
            
            vec3 viewDir = normalize(cameraPosition - vWorldPosition);
            vec3 normal = normalize(vWorldNormal);
            float fresnel = pow(1.0 - max(dot(viewDir, normal), 0.0), 2.0);
            color.rgb = mix(color.rgb, vec3(1.0), fresnel * 0.3);
        }
    }
    
    // Apply overall opacity
    color.a *= opacity;
    
    // X-ray mode makes selection more visible through geometry
    if (xrayMode) {
        color.a = min(color.a * 1.5, 0.9);
    }
    
    // Ensure we don't exceed max alpha
    color.a = clamp(color.a, 0.0, 1.0);
    
    fragColor = color;
}
