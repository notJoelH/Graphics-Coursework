Texture2D heightMap : register(t0);
SamplerState sampler0 : register(s0);

cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
    int useHeightMap;
    float3 padding;
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
    float4 depthPosition : TEXCOORD0;
};

// Declare constants
static const float HEIGHT_SCALE = 100.0f;

// Get height from heightmap
float GetHeight(float2 uv)
{
    float rawHeight = heightMap.SampleLevel(sampler0, uv, 0).r;
    rawHeight = pow(rawHeight, 2.0f); // Exaggerate height differences bc map is bright)
    return rawHeight * HEIGHT_SCALE;
}

OutputType main(InputType input)
{
    OutputType output;
    
    //Only displaace if there's a heightmap to displace with
    if (useHeightMap == 0)
    {
        input.position.y += GetHeight(input.tex);
    }

    // Calculate the position of the vertex against the world, view, and projection matrices.
    output.position = mul(input.position, worldMatrix);
    output.position = mul(output.position, viewMatrix);
    output.position = mul(output.position, projectionMatrix);

    // Store the position value in a second input value for depth value calculations.
    output.depthPosition = output.position;
	
    return output;
}