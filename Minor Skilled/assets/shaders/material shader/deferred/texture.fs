#version 460 core

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    sampler2D normal;
    sampler2D emission;
    sampler2D reflection;
    sampler2D height;

    float shininess;
    float refractionFactor;
    float heightScale;
    bool flipNormals;

    bool hasSpecular;
    bool hasNormal;
    bool hasReflection;
    bool hasHeight;
};

in VS_OUT {
    vec3 fragPosWorld;
    vec3 fragPosView;
    vec3 fragNormalView;
    vec3 fragNormalWorld;
    vec2 texCoord;

    mat3 TBN;
} fs_in;

layout (std140) uniform matricesBlock {
    mat4 viewMatrix;
    mat4 projectionMatrix;
    mat4 previousViewProjectionMatrix;
    mat4 lightSpaceMatrix;
};

layout (std140) uniform dataBlock {
    bool dirShadows;
    int usedCubeShadows;
    float farPlane;

    vec3 cameraPos;
    vec3 directionalLightPos;

    vec3 pointLightPositions[5];
};

uniform Material material;
uniform samplerCube environmentMap;

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec3 gAlbedo;
layout (location = 3) out vec2 gEmissionSpec;
layout (location = 4) out vec4 gEnvironmentShiny;

vec3 GetNormal(vec2 texCoord);
vec3 GetReflection(vec3 normal, vec2 texCoord);
vec2 ParallaxMapping(vec3 viewDirection);

void main() {
    vec3 viewDirection = normalize(fs_in.fragPosView); //camera pos is vec3(0.0f), since we are in view space

    //parallax mapping
    vec2 texCoord = ParallaxMapping(viewDirection);
    //if(material.hasHeight && (texCoord.x > 1.0f || texCoord.y > 1.0f || texCoord.x < 0.0f || texCoord.y < 0.0f)) discard; //cutoff edges to avoid artifacts when using parallax mapping

    //store the data in the gBuffer
    gPosition.rgb = fs_in.fragPosView;

    gNormal.rgb = GetNormal(texCoord);

    gAlbedo.rgb = texture(material.diffuse, texCoord).rgb;

    gEmissionSpec.r = texture(material.emission, texCoord).r;
    gEmissionSpec.g = texture(material.specular, texCoord).r;

    gEnvironmentShiny.rgb = GetReflection(fs_in.fragNormalWorld, texCoord);
    gEnvironmentShiny.a = material.shininess / 255.0f;
}

vec3 GetNormal(vec2 texCoord) {
    //take the normal from the normal map if there is one, otherwise use frag normal
    vec3 normal;

    if(material.hasNormal) {
        normal = texture(material.normal, texCoord).rgb; //range [0, 1]
        normal = normalize(normal * 2.0f - 1.0f); //bring to range [-1, 1]
        normal = normalize(fs_in.TBN * normal); //transform normal from tangent to view space 

        if(material.flipNormals) normal.y = -normal.y;
    } else {
        normal = normalize(fs_in.fragNormalView); //view space
    }

    return normal;
}

vec3 GetReflection(vec3 normal, vec2 texCoord) {
    if(!material.hasReflection) return vec3(0.0f);

    float reflectionAmount = texture(material.reflection, texCoord).r;

    if(reflectionAmount <= 0.01f) return vec3(0.0f);

    vec3 I = normalize(fs_in.fragPosWorld - cameraPos);
    vec3 R;

    if(material.refractionFactor > 0.0f) {
        //use refraction if the factor is not 0
        float ratio = 1.0f / material.refractionFactor;

        R = refract(I, normalize(normal), ratio);
    } else {
        //use reflection
        R = reflect(I, normalize(normal));
    }

    //sample from dynamic environment map
    return texture(environmentMap, R).rgb;
}

vec2 ParallaxMapping(vec3 viewDirection) {
    //use normal texCoords if there is no height map, otherwise apply parallax occulsion mapping
    if(!material.hasHeight) return fs_in.texCoord; 

    //number of depth layers
    const float minLayers = 8.0f;
    const float maxLayers = 32.0f;
    float layerAmount = mix(maxLayers, minLayers, abs(dot(vec3(0.0f, 0.0f, 1.0f), viewDirection)));

    //calculate the size of each layer
    float layerDepth = 1.0f / layerAmount;
    //depth of current layer
    float currentLayerDepth = 0.0f;
    //the amount to shift the texture coordinates per layer (from vector P)
    vec2 P = viewDirection.xy / viewDirection.z * material.heightScale; 
    vec2 deltaTexCoords = P / layerAmount;
  
    //get initial values
    vec2 currentTexCoords = fs_in.texCoord;
    float currentDepthMapValue = texture(material.height, currentTexCoords).r;
    
    while(currentLayerDepth < currentDepthMapValue) { //basically raycasting
        //shift texture coordinates along direction of P
        currentTexCoords -= deltaTexCoords;
        //get depthmap value at current texture coordinates
        currentDepthMapValue = texture(material.height, currentTexCoords).r;  
        //get depth of next layer
        currentLayerDepth += layerDepth;  
    }
    
    //get texture coordinates before collision (reverse operations)
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

    //get depth after and before collision for linear interpolation
    float afterDepth  = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = texture(material.height, prevTexCoords).r - currentLayerDepth + layerDepth;
 
    //interpolation of texture coordinates
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0f - weight);

    return finalTexCoords;
}