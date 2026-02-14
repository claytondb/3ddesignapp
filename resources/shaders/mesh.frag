/**
 * Mesh Fragment Shader
 * 
 * Blinn-Phong shading with support for:
 * - Single directional light
 * - Optional vertex colors
 * - Optional deviation colormap
 */
#version 410 core

// Inputs from vertex shader
in vec3 vWorldPosition;
in vec3 vWorldNormal;
in vec2 vTexCoord;
in vec4 vVertexColor;
in vec3 vViewPosition;

// Material properties
uniform vec3 baseColor;
uniform float metallic;
uniform float roughness;
uniform float ambientStrength;
uniform bool useVertexColor;
uniform bool useDeviation;

// Lighting - simple uniforms matching Viewport.cpp
uniform vec3 lightDir;      // Direction TO the light (or FROM light, we normalize)
uniform vec3 lightColor;    // Light color/intensity

// Camera
uniform vec3 cameraPosition;

// Deviation map
uniform float deviationMin;
uniform float deviationMax;
uniform sampler1D deviationColormap;

// Output
out vec4 fragColor;

// Fresnel-Schlick approximation
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    // Get base surface color
    vec3 albedo = baseColor;
    if (useVertexColor) {
        albedo *= vVertexColor.rgb;
    }
    
    // Normal and view direction
    vec3 normal = normalize(vWorldNormal);
    vec3 viewDir = normalize(cameraPosition - vWorldPosition);
    
    // Handle back faces by flipping normal
    if (!gl_FrontFacing) {
        normal = -normal;
    }
    
    // Ambient lighting
    vec3 ambient = albedo * ambientStrength;
    
    // Directional light calculation
    // lightDir typically points FROM light TO surface, so we negate it
    vec3 toLightDir = normalize(-lightDir);
    vec3 halfDir = normalize(toLightDir + viewDir);
    
    // Diffuse (Lambert)
    float NdotL = max(dot(normal, toLightDir), 0.0);
    vec3 diffuse = albedo * NdotL;
    
    // Specular (Blinn-Phong)
    float NdotH = max(dot(normal, halfDir), 0.0);
    float rough = max(roughness, 0.05); // Prevent division issues
    float shininess = max(2.0 / (rough * rough) - 2.0, 1.0);
    float spec = pow(NdotH, shininess);
    
    // Fresnel for specular
    float metallicVal = max(metallic, 0.0);
    vec3 F0 = mix(vec3(0.04), albedo, metallicVal);
    vec3 fresnel = fresnelSchlick(max(dot(halfDir, viewDir), 0.0), F0);
    vec3 specular = fresnel * spec * (1.0 - rough) * 0.5;
    
    // Combine direct lighting
    vec3 directLight = (diffuse + specular) * lightColor;
    
    // Simple rim light effect based on view angle (built-in, no extra uniform needed)
    float rimFactor = 1.0 - max(dot(normal, viewDir), 0.0);
    rimFactor = pow(rimFactor, 3.0);
    vec3 rimLight = lightColor * rimFactor * 0.15;
    
    // Final color
    vec3 color = ambient + directLight + rimLight;
    
    // Apply deviation colormap if enabled
    if (useDeviation) {
        float deviation = vVertexColor.a;
        float normalizedDev = (deviation - deviationMin) / (deviationMax - deviationMin);
        normalizedDev = clamp(normalizedDev, 0.0, 1.0);
        vec3 deviationColor = texture(deviationColormap, normalizedDev).rgb;
        color = deviationColor;
    }
    
    // Tone mapping (simple Reinhard)
    color = color / (color + vec3(1.0));
    
    // Gamma correction
    color = pow(color, vec3(1.0 / 2.2));
    
    fragColor = vec4(color, 1.0);
}
