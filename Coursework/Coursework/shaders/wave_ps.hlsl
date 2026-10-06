#define MAX_LIGHTS 6    // Copy of ShadowShader

Texture2D diffuseTexture : register(t0);
Texture2D shadowMap[MAX_LIGHTS] : register(t1);

SamplerState diffuseSampler : register(s0);
SamplerState shadowSampler : register(s1);

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

float4 calculateSpecular(float3 lightDir, float3 normal, float3 viewVec, float4 specColour, float specPower)
{
    float3 halfway = normalize(lightDir + viewVec);
    float specIntensity = pow(max(dot(normal, halfway), 0.0f), specPower);
    return saturate(specColour * specIntensity);
}

float2 getShadowUV(float4 lightViewPos)
{
    float2 uv = lightViewPos.xy / lightViewPos.w;
    uv *= float2(0.5f, -0.5f);
    uv += float2(0.5f, 0.5f);
    return uv;
}

float shadowTest(int lightIndex, float4 lightViewPos, float3 normal, float3 lightDir)
{
    float2 uv = getShadowUV(lightViewPos);
    if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f)
        return 1.0f;

    float bias = max(0.005f * (1.0f - dot(normal, lightDir)), 0.001f);
    float storedDepth = shadowMap[lightIndex].Sample(shadowSampler, uv).r;
    float currentDepth = lightViewPos.z / lightViewPos.w - bias;

    return (currentDepth <= storedDepth) ? 1.0f : 0.0f;
}

float4 main(InputType input) : SV_TARGET
{
    float4 texColour = diffuseTexture.Sample(diffuseSampler, input.tex);
    float3 normal = normalize(input.normal);
    float3 viewVec = normalize(input.viewVector);

    if (lights[0].rgbnorms == 1)
    {
        float3 col = normal * 0.5f + 0.5f;
        return float4(col, 1.0f);
    }

    float4 totalLight = float4(0, 0, 0, 0);

    for (int i = 0; i < numLights && i < MAX_LIGHTS; i++)
    {
        float4 ambient = lights[i].ambient;
        float4 diffuse = float4(0, 0, 0, 0);
        float4 spec = float4(0, 0, 0, 0);
        float shadow = 1.0f;

        switch (lights[i].lightType)
        {
            case 0:
            {
                    float3 lightDir = normalize(-lights[i].direction);
                    float NdotL = max(dot(normal, lightDir), 0.0f);
                    diffuse = lights[i].diffuse * NdotL;
                    spec = calculateSpecular(lightDir, normal, viewVec, lights[i].specular, lights[i].specPower);
                    shadow = shadowTest(i, input.lightViewPos[i], normal, lightDir);
                    break;
                }
            case 1:
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
            case 2:
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