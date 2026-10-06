#define MAX_LIGHTS 6

Texture2D diffuseTexture : register(t0);
Texture2D heightMap : register(t1);
Texture2D shadowMap[MAX_LIGHTS] : register(t2);

SamplerState diffuseSampler : register(s0);
SamplerState shadowSampler : register(s1);

// Same Light struct from ShadowShader
struct LightType
{
    float4 ambient;
    float4 diffuse;
    float3 position;
    float angle;
    float3 direction;
    float pad0;
    int lightType;
    int rgbnorms;
    float2 pad1;
    float3 attenFacs;
    float specPower;
    float4 specular;
};

cbuffer LightBuffer : register(b0)
{
    LightType lights[MAX_LIGHTS];
    int numLights;
    float3 padding;
};

struct InputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float3 worldPos : TEXCOORD1;
    float3 viewVector : TEXCOORD2;
    float4 lightViewPos[MAX_LIGHTS] : TEXCOORD3;
};

// Heightmap normal constants
static const float heightScale = 100.f;
static const float heightMapRes = 2048.f;
static const float planeSize = 100.f;
static const float deltaUV = 3.0f / heightMapRes;
static const float worldStep = planeSize * 3.0f/ heightMapRes;

// Get height from heightmap
float GetHeight(float2 uv)
{
    float rawHeight = heightMap.SampleLevel(diffuseSampler, uv, 0).r;
    rawHeight = pow(rawHeight, 2.0f); // Exaggerate height differences bc map is bright)
    return rawHeight * heightScale;
}

// Changed regular lighting calculation to Blinn-Phong specular calculation
float4 calculateSpecular(float3 lightDir, float3 normal, float3 viewVec, float4 specColour, float specPower)
{
    float3 halfway = normalize(lightDir + viewVec);
    float specIntensity = pow(max(dot(normal, halfway), 0.0f), specPower);
    return saturate(specColour * specIntensity);
}

float2 getProjectiveCoords(float4 lightViewPos)
{
    // Calculate the projected texture coordinates.
    float2 projTex = lightViewPos.xy / lightViewPos.w;
    projTex *= float2(0.5, -0.5);
    projTex += float2(0.5f, 0.5f);
    return projTex;
}

//Shadow test using depth map
float shadowTest(int lightNum, float4 lightViewPos, float3 normal, float3 lightDir)
{
    float2 uv = getProjectiveCoords(lightViewPos);
    
    //If the geometry is outside shadow, treat as lit
    if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f)
        return 1.0f;
    
    //Slope-scaled shadow bias
    float bias = max(.005f * (1.0f - dot(normal, lightDir)), 0.001f);
    
    float depthValue = shadowMap[lightNum].Sample(shadowSampler, uv).r; // Sample the shadow map (get depth of geometry)
    float lightDepth = lightViewPos.z / lightViewPos.w - bias; // Calculate the depth from the light.
    
    //Return 0 if in shadow, or 1 if lit
    return (lightDepth <= depthValue) ? 1.0f : 0.0f;
}


float4 main(InputType input) : SV_TARGET
{
    //Heightmap Normal Generation by sampling 5 points
    float centerHeight = GetHeight(input.tex);                        //center point
    float northHeight = GetHeight(input.tex + float2(0.f, deltaUV));  //positive y direction
    float southHeight = GetHeight(input.tex + float2(0.f, -deltaUV)); //negative y direction
    float eastHeight = GetHeight(input.tex + float2(deltaUV, 0.f));   //positive x direction
    float westHeight = GetHeight(input.tex + float2(-deltaUV, 0.f));  //negative x direction
    
    //Calculate slope deltas
    float deltaHeightX = (eastHeight - westHeight);
    float deltaHeightZ = (northHeight - southHeight);
    
    //Construct tangent vectors
    float3 tanX = float3(worldStep * 2.f, deltaHeightX, 0.f); 
    float3 tanZ = float3(0.f, deltaHeightZ, worldStep * 2.f);
 
    //Calculate normals and lighting
    float3 normal = normalize(cross(tanZ, tanX));
    float3 viewVec = normalize(input.viewVector);
    
    // RGB normal debug
    if (lights[0].rgbnorms == 1)
    {
        float3 col = normal * 0.5f + 0.5f;
        return float4(col, 1.0f);
    }

    float4 texColour = diffuseTexture.Sample(diffuseSampler, input.tex);
    float4 totalLight = float4(0, 0, 0, 0);
    
    // Per-light loop (from ShadowShader)
    for (int i = 0; i < numLights && i < MAX_LIGHTS; i++)
    {
        float4 ambient = lights[i].ambient;
        float4 diffuse = float4(0, 0, 0, 0);
        float4 spec = float4(0, 0, 0, 0);
        float shadow = 1.0f;

        switch (lights[i].lightType)
        {
            case 0: // Directional
            {
                    float3 lightDir = normalize(-lights[i].direction);
                    float NdotL = max(dot(normal, lightDir), 0.0f);
                    diffuse = lights[i].diffuse * NdotL;
                    spec = calculateSpecular(lightDir, normal, viewVec, lights[i].specular, lights[i].specPower);
                    shadow = shadowTest(i, input.lightViewPos[i], normal, lightDir);
                    break;
                }
            case 1: // Point
            {
                    float3 lightVec = lights[i].position - input.worldPos;
                    float dist = length(lightVec);
                    float3 lightDir = lightVec / dist;
                    float atten = 1.0f / (lights[i].attenFacs.x
                                + lights[i].attenFacs.y * dist
                                + lights[i].attenFacs.z * dist * dist);
                    float NdotL = max(dot(normal, lightDir), 0.0f);
                    diffuse = lights[i].diffuse * NdotL * atten;
                    spec = calculateSpecular(lightDir, normal, viewVec, lights[i].specular, lights[i].specPower) * atten;
                    shadow = 1.0f;
                    break;
                }
            case 2: // Spot
            {
                    float3 lightVec = lights[i].position - input.worldPos;
                    float dist = length(lightVec);
                    float3 lightDir = lightVec / dist;
                    float atten = 1.0f / (lights[i].attenFacs.x
                                 + lights[i].attenFacs.y * dist
                                 + lights[i].attenFacs.z * dist * dist);
                    float spotCos = dot(normalize(lights[i].direction), -lightDir);
                    float cosOuter = cos(radians(lights[i].angle));
                    float spotAtten = saturate((spotCos - cosOuter) / (1.0f - cosOuter));
                    float NdotL = max(dot(normal, lightDir), 0.0f);
                    diffuse = lights[i].diffuse * NdotL * atten * spotAtten;
                    spec = calculateSpecular(lightDir, normal, viewVec, lights[i].specular, lights[i].specPower) * atten * spotAtten;
                    shadow = shadowTest(i, input.lightViewPos[i], normal, lightDir);
                    break;
                }
        }

        totalLight += ambient + (diffuse + spec) * shadow;
    }

    float4 finalColour = saturate(totalLight) * texColour;
    finalColour.a = texColour.a;
    return finalColour;
}
