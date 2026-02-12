/**
 * Line Fragment Shader
 * 
 * Simple line rendering with color pass-through.
 * Supports alpha blending for smooth lines.
 */
#version 410 core

// Inputs from vertex shader
in vec4 vColor;
in vec3 vWorldPos;

// Uniforms
uniform float opacity;          // Global opacity multiplier
uniform bool depthFade;         // Enable depth-based fading
uniform float depthFadeStart;   // Depth at which fading begins
uniform float depthFadeEnd;     // Depth at which fully faded

// Output
out vec4 fragColor;

void main() {
    vec4 color = vColor;
    
    // Apply global opacity
    color.a *= opacity;
    
    // Optional depth-based fading (useful for dense wireframes)
    if (depthFade) {
        float depth = gl_FragCoord.z;
        float depthFactor = 1.0 - smoothstep(depthFadeStart, depthFadeEnd, depth);
        color.a *= depthFactor;
    }
    
    // Discard fully transparent fragments
    if (color.a < 0.001) {
        discard;
    }
    
    fragColor = color;
}
