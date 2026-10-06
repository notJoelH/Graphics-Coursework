// Light pixel shader
// Calculate diffuse lighting for a single directional light (also texturing)
Texture2D texture0 : register(t0);
SamplerState sampler0 : register(s0);

#define MAX_LIGHTS 6

struct LightType
{
    float4 ambientColour;
    float4 diffuseColour;
    
    float3 lightPosition; // light position in space
    float lightAngle; // angle of light cone
    
    float3 lightDirection; // direction light is travelling
    float pad0;

    int lightType; // 0 = directional, 1 = point, 2 = spot  //consider changing this to an Enum Struct
    int rgbnorms;
    float2 pad1;
    
    float3 attenFacs; // attenuation
    float specPower; // specular power
    
    float4 specular; // specular colour
};

cbuffer LightBuffer : register(b0)
{
    LightType lights[MAX_LIGHTS];
    int numLights;
    int emissiveMode; // NEW: 1 = skip lighting, output bright colour
    float2 pad2; 
};

struct InputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float3 worldPosition : TEXCOORD1;
    float3 viewVector : TEXCOORD2;
};

// Calculate lighting intensity based on direction and normal. Combine with light colour.
float4 calculateLighting(float3 lightDirection, float3 normal, float4 diffuse)
{
    float intensity = saturate(dot(normal, lightDirection));
    float4 colour = saturate(diffuse * intensity);
    return colour;
};

float4 calculateSpecular(float3 lightDir, float3 normal, float3 viewVec, float4 specColour, float specPower)
{
    //Blinn-Phong specular calculation
    float3 halfway = normalize(lightDir + viewVec);
    float specIntensity = pow(max(dot(normal, halfway), 0.0f), specPower);
    
    return saturate(specColour * specIntensity);
}

float4 main(InputType input) : SV_TARGET
{
    //Sample textures
    float4 textureColour = texture0.Sample(sampler0, input.tex);
    
    //Emissive mode: output texture colour without lighting calculations
    if (emissiveMode == 1)
    {
        return textureColour * lights[0].diffuseColour * 2.5f;
    }
    
    //If on, output RGB normals as colours
    if (lights[0].rgbnorms == 1)
    {
        // normalize vectors, set colours of xyz to rgb, return 16 bytes
        float3 vecNorm = normalize(input.normal);
        float3 colour = (vecNorm * 0.5f) + 0.5f;
        return float4(colour, 1.0f);
    }
    
    float3 normal = normalize(input.normal);
    float3 viewVec = normalize(input.viewVector);
    float4 newLightColour = (0.0f, 0.0f, 0.0f, 1.0f);

    for (int i = 0; i < numLights; i++)
    {
        //declaring variables
        float distance, attenuation, cosOuter, spotAngle, spotAtten, NdotL;
        float3 lightVector = lights[i].lightPosition - input.worldPosition; //set surface normals to unit normals
        float4 lightColour = lights[i].ambientColour, diffuse, specAmount;
    
        //Switch statement to select lightType
        switch (lights[i].lightType)
        {
            case 0: //Directional Light
        {   
                    lightVector = normalize(-lights[i].lightDirection);
                    NdotL = max(dot(normal, lightVector), 0.0f);
                    diffuse = lights[i].diffuseColour * NdotL;
                    specAmount = calculateSpecular(lightVector, normal, viewVec, lights[i].specular, lights[i].specPower);
                    lightColour += diffuse + specAmount;
                    break;
                }
        
            case 1: //Point Light
        {
                    distance = length(lightVector);
                    lightVector = normalize(lightVector);
                    attenuation = 1.0f / (lights[i].attenFacs.x + (lights[i].attenFacs.y * distance) + (lights[i].attenFacs.z * (distance * distance)));
                    NdotL = max(dot(normal, lightVector), 0.0f);
                    diffuse = lights[i].diffuseColour * NdotL * attenuation;
                    specAmount = calculateSpecular(lightVector, normal, viewVec, lights[i].specular, lights[i].specPower);
                    lightColour += diffuse + specAmount;
                    break;
                }
        
            case 2: //Spot Light
        {
                    distance = length(lightVector);
                    lightVector = normalize(lightVector);
                    spotAngle = dot(normalize(lights[i].lightDirection), -lightVector);
                    cosOuter = cos(radians(lights[i].lightAngle));
                    attenuation = 1.0f / (lights[i].attenFacs.x + (lights[i].attenFacs.y * distance) + (lights[i].attenFacs.z * (distance * distance)));
                    spotAtten = saturate((spotAngle - cosOuter) / (1.0f - cosOuter));
                    NdotL = max(dot(normal, lightVector), 0.0f);
                    diffuse = lights[i].diffuseColour * NdotL * attenuation * spotAtten;
                    specAmount = calculateSpecular(lightVector, normal, viewVec, lights[i].specular, lights[i].specPower);
                    lightColour += diffuse + specAmount;
                    break;
                }
        }
        
        newLightColour += lightColour;
    }
    
    return saturate(textureColour * newLightColour);
}
