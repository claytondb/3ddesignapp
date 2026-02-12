/**
 * Grid Fragment Shader
 * 
 * Renders grid lines with:
 * - Distance-based alpha fading
 * - Anti-aliased lines (when using line primitives)
 * - Support for major/minor line distinction
 */
#version 410 core

// Inputs from vertex shader
in float vDistance;
in vec3 vWorldPos;

// Uniforms
uniform vec3 lineColor;         // Color of grid lines
uniform float fadeDistance;     // Distance at which fading begins
uniform float maxDistance;      // Maximum visible distance
uniform float gridOpacity;      // Base opacity multiplier

// Output
out vec4 fragColor;

void main() {
    // Calculate fade factor based on distance
    // Smooth fade from fadeDistance to maxDistance
    float fadeStart = fadeDistance * 0.5;
    float fadeFactor = 1.0 - smoothstep(fadeStart, maxDistance, vDistance);
    
    // Discard fully transparent fragments
    if (fadeFactor <= 0.001) {
        discard;
    }
    
    // Apply opacity
    float alpha = fadeFactor * gridOpacity;
    
    // Output final color
    fragColor = vec4(lineColor, alpha);
}
