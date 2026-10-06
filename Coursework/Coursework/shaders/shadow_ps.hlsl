#define MAX_LIGHTS 6

// t0 = diffuse tex, t1-6 shadow maps (one per light)
Texture2D shaderTexture : register(t0);
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
    int lightType;
    int rgbnorms;
    float3 attenFacs;
    float3 padding;
    float specPower;
    float4 specular;
};

cbuffer LightBuffer : register(b0)
{
    LightType lights[MAX_LIGHTS];
    int numLights;
    float3 pad2;
}

struct InputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float3 worldPos : TEXCOORD1;
    float3 viewVector : TEXCOORD2;
    float4 lightViewPos[MAX_LIGHTS] : TEXCOORD3;
};

//Blinn-Phong specular calculation
float4 calculateSpecular(float3 lightDir, float3 normal, float3 viewVec, float4 specColour, float specPower)
{
    float3 halfway = normalize(lightDir + viewVec);
    float specIntensity = pow(max(dot(normal, halfway), 0.0f), specPower);
    return saturate(specColour * specIntensity);
}

//Convert light-space position to normalized UV
float2 getProjectiveCoords(float4 lightViewPos)
{
    // Calculate shadow UV
    float2 projTex = lightViewPos.xy / lightViewPos.w;
    projTex *= float2(0.5f, -0.5f);
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
    // Initialize variables
    float4 textureColour = shaderTexture.Sample(diffuseSampler, input.tex);
    float3 normal = normalize(input.normal);
    float3 viewVec = normalize(input.viewVector);

    //Visualise normals toggle
    if (lights[0].rgbnorms == 1)
    {
        float3 colour = normal * 0.5f + 0.5f;
        return float4(colour, 1.0f);
    }
    
    //Accumulate lighting across lights
    float4 totalLight = float4(0, 0, 0, 0); 
    
    //Per-light loop
    for (int i = 0; i < numLights && i < MAX_LIGHTS; i++)
    {
        //Declaring variables for directional light
        float4 ambient = lights[i].ambient; //Start with base ambient light value
        float4 diffuse = float4(0, 0, 0, 0);
        float4 spec = float4(0, 0, 0, 0);
        float shadow = 1.0f;
    
        //Switch statement for lighting calculations
        switch (lights[i].lightType)
        {
            case 0: //Directional Light
        {   
                    float3 lightDir = normalize(-lights[i].direction);
                    float NdotL = max(dot(normal, lightDir), 0.0f);
                    diffuse = lights[i].diffuse * NdotL;
                    spec = calculateSpecular(lightDir, normal, viewVec, lights[i].specular, lights[i].specPower);
                    shadow = shadowTest(i, input.lightViewPos[i], normal, lightDir);
                    break;
                }
        
            case 1: //Point Light
        {
                    float3 lightVec = lights[i].position - input.worldPos;
                    float dist = length(lightVec);
                    float3 lightDir = lightVec / dist;
                    float attenuation = 1.0f / (lights[i].attenFacs.x + (lights[i].attenFacs.y * dist) + (lights[i].attenFacs.z * (dist * dist)));
                    float NdotL = max(dot(normal, lightDir), 0.0f);
                    diffuse = lights[i].diffuse * NdotL * attenuation;
                    spec = calculateSpecular(lightDir, normal, viewVec, lights[i].specular, lights[i].specPower);
                    shadow = 1.0f;
                    break;
                }
        
            case 2: //Spot Light
        {
                    float3 lightVec = lights[i].position - input.worldPos;
                    float dist = length(lightVec);
                    float3 lightDir = lightVec / dist;
                    float attenuation = 1.0f / (lights[i].attenFacs.x + (lights[i].attenFacs.y * dist) + (lights[i].attenFacs.z * (dist * dist)));
                    float spotAngle = dot(normalize(lights[i].direction), -lightDir);
                    float cosOuter = cos(radians(lights[i].angle));
                    float spotAtten = saturate((spotAngle - cosOuter) / (1.0f - cosOuter));
                    float NdotL = max(dot(normal, lightDir), 0.0f);
                    diffuse = lights[i].diffuse * NdotL * attenuation * spotAtten;
                    spec = calculateSpecular(lightDir, normal, viewVec, lights[i].specular, lights[i].specPower) * attenuation * spotAtten;
                    shadow = shadowTest(i, input.lightViewPos[i], normal, lightDir);
                    break;
                }
        }

        //Accumulate light contribution from this light, factoring in shadowing
        totalLight += ambient + (diffuse + spec) * shadow; 
    }
    
    //Final Colour value
    float4 finalColour = saturate(totalLight) * textureColour;
    finalColour.a = textureColour.a;
    return finalColour;
}