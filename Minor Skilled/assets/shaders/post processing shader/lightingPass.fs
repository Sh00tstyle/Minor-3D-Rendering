#version 460 core

//light types
const int DIRECTIONAL = 0;
const int POINT = 1;
const int SPOT = 2;

struct Light {
    vec4 position;
    vec4 direction;

    vec4 ambient;
    vec4 diffuse;
    vec4 specular;

    int type;

    float constant;
    float linear;
    float quadratic;
    float innerCutoff;
    float outerCutoff;

    vec2 padding;
};

in vec2 texCoord;

layout (std140) uniform matricesBlock {
    mat4 viewMatrix;
    mat4 projectionMatrix;
    mat4 lightSpaceMatrix;
};

layout (std140) uniform dataBlock {
    bool useShadows;

    vec3 cameraPos;
    vec3 directionalLightPos;
};

layout(std430) buffer lightsBlock {
    int usedLights;
    Light lights[];
};

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
uniform sampler2D gEmissionShiny;

uniform sampler2D shadowMap;

layout (location = 0) out vec4 fragColor;
layout (location = 1) out vec4 brightColor;

vec3 CalculateDirectionalLight(Light light, vec3 normal, vec3 viewDirection, vec2 texCoord);
vec3 CalculatePointLight(Light light, vec3 normal, vec3 fragPos, vec3 viewDirection, vec2 texCoord);
vec3 CalculateSpotLight(Light light, vec3 normal, vec3 fragPos, vec3 viewDirection, vec2 texCoord);

float CalculateShadow(vec3 normal, vec3 fragPos, vec4 lightSpaceFragPos);

vec4 CalculateBrightColor(vec3 color);

void main() {
    //sample data from the gBuffer textures
    vec3 fragPos = texture(gPosition, texCoord).rgb;
    vec3 normal = texture(gNormal, texCoord).rgb;

    //lighting
    vec3 viewDirection = normalize(cameraPos - fragPos);
    vec3 result = vec3(0.0f);

    for(int i = 0; i < usedLights; i++) {
        switch(lights[i].type) {
            case DIRECTIONAL:
                result += CalculateDirectionalLight(lights[i], normal, viewDirection, texCoord);
                break;

            case POINT:
                result += CalculatePointLight(lights[i], normal, fragPos, viewDirection, texCoord);
                break;

            case SPOT:
                result += CalculateSpotLight(lights[i], normal, fragPos, viewDirection, texCoord);
                break;
        }
    }

    if(usedLights == 0) { //in case we have no light, simply take the albedo
        result = texture(gAlbedoSpec, texCoord).rgb;
    }

    //shadows
    vec4 lightSpaceFragPos = lightSpaceMatrix * vec4(fragPos, 1.0f);

    float shadow = CalculateShadow(normal, fragPos, lightSpaceFragPos);
    shadow = 1.0f - shadow * 0.5f;
    result *= shadow;

    //emission
    vec3 emission = texture(gEmissionShiny, texCoord).rgb;
    result += emission;

    fragColor = vec4(result, 1.0f);
    brightColor = CalculateBrightColor(result);
}

vec3 CalculateDirectionalLight(Light light, vec3 normal, vec3 viewDirection, vec2 texCoord) {
    //ambient
    vec3 ambient = light.ambient.rgb * texture(gAlbedoSpec, texCoord).rgb;

    //diffuse
    float difference = max(dot(normal, -light.direction.xyz), 0.0f);
    vec3 diffuse = light.diffuse.rgb * difference * texture(gAlbedoSpec, texCoord).rgb;

    //specular
    vec3 halfwayDireciton = normalize(light.direction.xyz + viewDirection); //blinn-phong
    float specularity = pow(max(dot(normal, halfwayDireciton), 0.0f), texture(gEmissionShiny, texCoord).a * 255.0f);
    vec3 specular = light.specular.rgb * specularity * texture(gAlbedoSpec, texCoord).a;

    //combine results
    return (ambient + diffuse + specular);
}

vec3 CalculatePointLight(Light light, vec3 normal, vec3 fragPos, vec3 viewDirection, vec2 texCoord) {
    vec3 lightDirection = normalize(light.position.xyz - fragPos);

    //ambient
    vec3 ambient = light.ambient.rgb * texture(gAlbedoSpec, texCoord).rgb;

    //diffuse
    float difference = max(dot(normal, lightDirection), 0.0f);
    vec3 diffuse = light.diffuse.rgb * difference * texture(gAlbedoSpec, texCoord).rgb;

    //specular
    vec3 halfwayDireciton = normalize(lightDirection + viewDirection); //blinn-phong
    float specularity = pow(max(dot(normal, halfwayDireciton), 0.0f), texture(gEmissionShiny, texCoord).a * 255.0f);
    vec3 specular = light.specular.rgb * specularity * texture(gAlbedoSpec, texCoord).a;

    //attenuation
    float distance = length(light.position.xyz - fragPos);
    float attenuation = 1.0f / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    //combine results
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;

    return (ambient + diffuse + specular);
}

vec3 CalculateSpotLight(Light light, vec3 normal, vec3 fragPos, vec3 viewDirection, vec2 texCoord) {
    vec3 lightDirection = normalize(light.position.xyz - fragPos);

    //ambient
    vec3 ambient = light.ambient.rgb * texture(gAlbedoSpec, texCoord).rgb;

    //diffuse
    float difference = max(dot(normal, lightDirection), 0.0f);
    vec3 diffuse = light.diffuse.rgb * difference * texture(gAlbedoSpec, texCoord).rgb;

    //specular
    vec3 halfwayDireciton = normalize(lightDirection + viewDirection); //blinn-phong
    float specularity = pow(max(dot(normal, halfwayDireciton), 0.0f), texture(gEmissionShiny, texCoord).a * 255.0f);
    vec3 specular = light.specular.rgb * specularity *  texture(gAlbedoSpec, texCoord).a;

    //attenuation
    float distance = length(light.position.xyz - fragPos);
    float attenuation = 1.0f / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    //spotlight
    float theta = dot(lightDirection, normalize(light.direction.xyz));
    float epsilon = light.innerCutoff - light.outerCutoff;
    float intensity = clamp((theta - light.outerCutoff) / epsilon, 0.0f, 1.0f);

    //combine results
    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;

    return (ambient + diffuse + specular);
}

float CalculateShadow(vec3 normal, vec3 fragPos, vec4 lightSpaceFragPos) {
    if(!useShadows) return 0.0f; //no shadows

    //perform perspective divide
    vec3 projectedCoords = lightSpaceFragPos.xyz / lightSpaceFragPos.w;

    //transform to [0,1] range
    projectedCoords = projectedCoords * 0.5f + 0.5f;

    //get closest depth value from lights perspective (using [0,1] range lightSpaceFragPos as coords)
    float closestDepth = texture(shadowMap, projectedCoords.xy).r; 

    //get depth of current fragment from lights perspective
    float currentDepth = projectedCoords.z;

    //calculate bias based on depth map resolution and slope
    vec3 lightDirection = normalize(directionalLightPos - fragPos);
    float bias = max(0.05f * (1.0f - dot(normal, lightDirection)), 0.005f);

    //PCF
    float shadow = 0.0f;
    vec2 texelSize = 1.0f / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, projectedCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth ? 1.0f : 0.0f;        
        }    
    }

    shadow /= 9.0f;
    
    //keep the shadow at 0.0f when outside the far plane region of the lights frustum.
    if(projectedCoords.z > 1.0f) shadow = 0.0f;

    return shadow;
}

vec4 CalculateBrightColor(vec3 color) {
    const vec3 threshold = vec3(0.2126f, 0.7152f, 0.0722f);

    float brightness = dot(color, threshold);

    //return the color if it was bright enough, otherwise return black
    if(brightness > 1.0f) return vec4(color, 1.0f);
    else return vec4(vec3(0.0f), 1.0f);
}