/**
 * @file deviation.frag
 * @brief Fragment shader for deviation colormap visualization
 * 
 * Renders per-vertex deviation values using configurable colormaps.
 * Supports multiple colormap types with smooth color interpolation.
 */

#version 410 core

// Inputs from vertex shader
in vec3 v_Normal;           // Surface normal (world space)
in vec3 v_WorldPos;         // World position
in float v_Deviation;       // Raw deviation value
in float v_NormalizedDev;   // Deviation normalized to [0, 1]

// Uniforms
uniform int u_ColormapType;     // 0=BlueGreenRed, 1=Rainbow, 2=CoolWarm, 3=Viridis, 4=Magma, 5=Grayscale
uniform float u_MinVal;          // Minimum value for normalization
uniform float u_MaxVal;          // Maximum value for normalization
uniform float u_Transparency;    // Overall transparency (1.0 = opaque)
uniform bool u_ShowIsolines;     // Whether to show isolines
uniform float u_IsolineSpacing;  // Spacing between isolines
uniform vec3 u_LightDir;         // Light direction (world space)
uniform float u_AmbientStrength; // Ambient light strength
uniform float u_DiffuseStrength; // Diffuse light strength

// Output
out vec4 fragColor;

// ============================================================================
// Colormap Functions
// ============================================================================

/**
 * Blue-Green-Red diverging colormap
 * Blue at 0, Green at 0.5, Red at 1
 */
vec3 blueGreenRed(float t) {
    if (t < 0.5) {
        // Blue to Green
        float s = t * 2.0;
        return vec3(0.0, s, 1.0 - s);
    } else {
        // Green to Red
        float s = (t - 0.5) * 2.0;
        return vec3(s, 1.0 - s, 0.0);
    }
}

/**
 * Rainbow colormap (HSV-based)
 * Red at 0 through violet at 1
 */
vec3 rainbow(float t) {
    float h = (1.0 - t) * 0.8;  // Hue from red to purple
    float s = 1.0;               // Full saturation
    float v = 1.0;               // Full value
    
    // HSV to RGB conversion
    float c = v * s;
    float x = c * (1.0 - abs(mod(h * 6.0, 2.0) - 1.0));
    float m = v - c;
    
    vec3 rgb;
    float h6 = h * 6.0;
    if (h6 < 1.0) rgb = vec3(c, x, 0.0);
    else if (h6 < 2.0) rgb = vec3(x, c, 0.0);
    else if (h6 < 3.0) rgb = vec3(0.0, c, x);
    else if (h6 < 4.0) rgb = vec3(0.0, x, c);
    else if (h6 < 5.0) rgb = vec3(x, 0.0, c);
    else rgb = vec3(c, 0.0, x);
    
    return rgb + m;
}

/**
 * Cool-Warm diverging colormap
 * Blue at 0, White at 0.5, Red at 1
 */
vec3 coolWarm(float t) {
    vec3 cool = vec3(0.231, 0.298, 0.753);   // Blue
    vec3 warm = vec3(0.706, 0.016, 0.150);   // Red
    vec3 white = vec3(0.867, 0.867, 0.867);  // Off-white
    
    if (t < 0.5) {
        return mix(cool, white, t * 2.0);
    } else {
        return mix(white, warm, (t - 0.5) * 2.0);
    }
}

/**
 * Viridis colormap (perceptually uniform)
 * Dark purple to yellow
 */
vec3 viridis(float t) {
    // Key colors from the Viridis colormap
    const vec3 c0 = vec3(0.267, 0.004, 0.329);  // Dark purple
    const vec3 c1 = vec3(0.282, 0.140, 0.458);
    const vec3 c2 = vec3(0.254, 0.265, 0.530);
    const vec3 c3 = vec3(0.163, 0.471, 0.558);  // Teal
    const vec3 c4 = vec3(0.134, 0.659, 0.518);
    const vec3 c5 = vec3(0.477, 0.821, 0.318);
    const vec3 c6 = vec3(0.993, 0.906, 0.144);  // Yellow
    
    if (t < 0.167) return mix(c0, c1, t / 0.167);
    else if (t < 0.333) return mix(c1, c2, (t - 0.167) / 0.167);
    else if (t < 0.5) return mix(c2, c3, (t - 0.333) / 0.167);
    else if (t < 0.667) return mix(c3, c4, (t - 0.5) / 0.167);
    else if (t < 0.833) return mix(c4, c5, (t - 0.667) / 0.167);
    else return mix(c5, c6, (t - 0.833) / 0.167);
}

/**
 * Magma colormap (perceptually uniform)
 * Black through magenta to pale yellow
 */
vec3 magma(float t) {
    const vec3 c0 = vec3(0.001, 0.000, 0.014);  // Near black
    const vec3 c1 = vec3(0.282, 0.141, 0.459);  // Purple
    const vec3 c2 = vec3(0.716, 0.215, 0.475);  // Magenta
    const vec3 c3 = vec3(0.994, 0.624, 0.427);  // Orange
    const vec3 c4 = vec3(0.987, 0.991, 0.749);  // Pale yellow
    
    if (t < 0.25) return mix(c0, c1, t / 0.25);
    else if (t < 0.5) return mix(c1, c2, (t - 0.25) / 0.25);
    else if (t < 0.75) return mix(c2, c3, (t - 0.5) / 0.25);
    else return mix(c3, c4, (t - 0.75) / 0.25);
}

/**
 * Grayscale colormap
 * Black at 0, White at 1
 */
vec3 grayscale(float t) {
    return vec3(t);
}

/**
 * Plasma colormap (perceptually uniform)
 * Blue through magenta to yellow
 */
vec3 plasma(float t) {
    const vec3 c0 = vec3(0.050, 0.030, 0.527);  // Dark blue
    const vec3 c1 = vec3(0.417, 0.001, 0.658);  // Purple
    const vec3 c2 = vec3(0.693, 0.165, 0.564);  // Magenta
    const vec3 c3 = vec3(0.881, 0.392, 0.384);  // Salmon
    const vec3 c4 = vec3(0.988, 0.652, 0.211);  // Orange
    const vec3 c5 = vec3(0.940, 0.975, 0.131);  // Yellow
    
    if (t < 0.2) return mix(c0, c1, t / 0.2);
    else if (t < 0.4) return mix(c1, c2, (t - 0.2) / 0.2);
    else if (t < 0.6) return mix(c2, c3, (t - 0.4) / 0.2);
    else if (t < 0.8) return mix(c3, c4, (t - 0.6) / 0.2);
    else return mix(c4, c5, (t - 0.8) / 0.2);
}

/**
 * Get color from selected colormap
 */
vec3 getColormapColor(float t) {
    switch (u_ColormapType) {
        case 0: return blueGreenRed(t);
        case 1: return rainbow(t);
        case 2: return coolWarm(t);
        case 3: return viridis(t);
        case 4: return magma(t);
        case 5: return grayscale(t);
        case 6: return plasma(t);
        default: return blueGreenRed(t);
    }
}

// ============================================================================
// Lighting Functions
// ============================================================================

/**
 * Simple diffuse lighting
 */
float diffuseLighting(vec3 normal, vec3 lightDir) {
    return max(dot(normal, lightDir), 0.0);
}

// ============================================================================
// Main
// ============================================================================

void main() {
    // Normalize interpolated normal
    vec3 normal = normalize(v_Normal);
    
    // Get base color from colormap
    vec3 baseColor = getColormapColor(v_NormalizedDev);
    
    // Apply isolines if enabled
    if (u_ShowIsolines && u_IsolineSpacing > 0.0) {
        float isolineValue = abs(v_Deviation) / u_IsolineSpacing;
        float isolineAlpha = fract(isolineValue);
        
        // Create isoline bands
        float lineWidth = 0.05;
        if (isolineAlpha < lineWidth || isolineAlpha > (1.0 - lineWidth)) {
            // Darken at isolines
            baseColor *= 0.7;
        }
    }
    
    // Simple diffuse lighting
    vec3 lightDir = normalize(u_LightDir);
    float ambient = u_AmbientStrength;
    float diffuse = diffuseLighting(normal, lightDir) * u_DiffuseStrength;
    
    // Ensure minimum visibility
    float lighting = max(ambient + diffuse, 0.3);
    
    // Final color with lighting
    vec3 finalColor = baseColor * lighting;
    
    // Output with transparency
    fragColor = vec4(finalColor, u_Transparency);
}
