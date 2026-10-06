Texture2D sceneTex : register(t0);
SamplerState sampler0 : register(s0);

cbuffer BrightBuffer : register(b0)
{
    float lightValue;
    float3 padding;
};

struct InputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
};

float luminance(float3 colour)
{
    return dot(colour, float3(0.2126, 0.7152, 0.0722));
}

// Changed due to bloom not working correctly
// Pixel Shader will now output the excess brightness of the scene, which will be used in the bloom effect
float4 main(InputType input) : SV_TARGET
{
    float4 colour = sceneTex.Sample(sampler0, input.tex);
    float brightness = luminance(colour.rgb);
    float excess = max(brightness - lightValue, 0.0f);
    return float4(colour.rgb * excess, 1.0f);
}