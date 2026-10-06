Texture2D texture0 : register(t0);
Texture2D bloomTex : register(t1);
SamplerState Sampler0 : register(s0);

cbuffer BloomBuffer : register(b0)
{
    float bloomStrength;
    int bloomOn;
    float2 padding;
};

struct InputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
};

float4 main(InputType input) : SV_TARGET
{
	// Sample the pixel color from the texture using the sampler at this texture coordinate location.
    float4 textureColor = texture0.Sample(Sampler0, input.tex);

    if (bloomOn == 1)
    {
        float4 bloom = bloomTex.Sample(Sampler0, input.tex);
        textureColor += bloom * bloomStrength;
    }
    
    return textureColor;
}