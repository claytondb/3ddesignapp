/**
 * Mesh Fragment Shader
 * 
 * Blinn-Phong shading with support for:
 * - Directional light (key light)
 * - Fill light (softer secondary)
 * - Rim/back light
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

// Light structure
struct DirectionalLight {
    vec3 direction;
    vec3 color;
    float intensity;
};

// Material properties
uniform vec3 baseColor;
uniform float metallic;
uniform float roughness;
uniform float ambientStrength;
uniform bool useVertexColor;
uniform bool useDeviation;

// Lights
uniform DirectionalLight keyLight;
uniform DirectionalLight fillLight;
uniform DirectionalLight rimLight;
uniform vec3 ambientColor;

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

// Calculate lighting contribution from a directional light
vec3 calculateLight(DirectionalLight light, vec3 normal, vec3 viewDir, vec3 albedo, float rough) {
    vec3 lightDir = normalize(-light.direction);
    vec3 halfDir = normalize(lightDir + viewDir);
    
    // Diffuse (Lambert)
    float NdotL = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = albedo * NdotL;
    
    // Specular (Blinn-Phong)
    float NdotH = max(dot(normal, halfDir), 0.0);
    float shininess = max(2.0 / (rough * rough) - 2.0, 1.0);
    float spec = pow(NdotH, shininess);
    
    // Fresnel
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 fresnel = fresnelSchlick(max(dot(halfDir, viewDir), 0.0), F0);
    vec3 specular = fresnel * spec * (1.0 - rough);
    
    return (diffuse + specular) * light.color * light.intensity;
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
    
    // Ambient
    vec3 ambient = ambientColor * albedo * ambientStrength;
    
    // Calculate lighting from each light
    vec3 lighting = vec3(0.0);
    lighting += calculateLight(keyLight, normal, viewDir, albedo, roughness);
    lighting += calculateLight(fillLight, normal, viewDir, albedo, roughness);
    
    // Rim light (uses fresnel for edge highlighting)
    float rimFactor = 1.0 - max(dot(normal, viewDir), 0.0);
    rimFactor = pow(rimFactor, 3.0);
    lighting += rimLight.color * rimLight.intensity * rimFactor * 0.5;
    
    // Combine
    vec3 color = ambient + lighting;
    
    // Apply deviation colormap if enabled
    if (useDeviation) {
        // Assuming deviation is stored in vVertexColor.a or a separate attribute
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
