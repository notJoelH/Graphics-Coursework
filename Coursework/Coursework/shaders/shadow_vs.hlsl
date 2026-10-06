#define MAX_LIGHTS 6

cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
    matrix lightViewMatrix[MAX_LIGHTS];
    matrix lightProjectionMatrix[MAX_LIGHTS];
    int numlights;
    float3 padding;
};

cbuffer Camerabuffer : register(b1)
{
    float3 camPos;
    float camPadding;
};

struct InputType
{
    float4 position : POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
};

struct OutputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float3 worldPos : TEXCOORD1;
    float3 viewVec : TEXCOORD2;
    float4 lightViewPos[MAX_LIGHTS] : TEXCOORD3;
};

OutputType main(InputType input)
{
    OutputType output;

	// Calculate the position of the vertex
    float4 worldPos = mul(input.position, worldMatrix);
    output.position = mul(worldPos, viewMatrix);
    output.position = mul(output.position, projectionMatrix);
    
    // Transform & pass attributes to pixel shader
    output.tex = input.tex;
    output.normal = normalize(mul(input.normal, (float3x3) worldMatrix));
    output.worldPos = worldPos.xyz;
    output.viewVec = normalize(camPos - worldPos.xyz);
  
	// Calculate the position of the vertice as viewed by the light source. Once for each light
    for (int i = 0; i < numlights; i++)
    {
        float4 lightSpacePos = mul(worldPos, lightViewMatrix[i]);
        lightSpacePos = mul(lightSpacePos, lightProjectionMatrix[i]);
        output.lightViewPos[i] = lightSpacePos;
    }
    
    return output;
}