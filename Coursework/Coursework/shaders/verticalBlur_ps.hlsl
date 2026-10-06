Texture2D shaderTexture : register(t0);
SamplerState SampleType : register(s0);

cbuffer ScreenSizeBuffer : register(b0)
{
    float screenHeight;
    float3 padding;
};

struct InputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
};

float4 main(InputType input) : SV_TARGET
{
    // Symmetric Gaussian weights that sum to 1.0
    float weight0 = 0.227027f; // center
    float weight1 = 0.194594f;
    float weight2 = 0.121621f;
    float weight3 = 0.054054f;
    float weight4 = 0.016216f;
    
    float texelSize = 1.0f / screenHeight;
    float4 colour = float4(0, 0, 0, 0);
    
	/* Legacy box blur
    float nWeight = 1.0f / 9.0f;
    weight0 = nWeight;
    weight1 = nWeight;
    weight2 = nWeight;
    weight3 = nWeight;
    weight4 = nWeight;
    */
    
    colour += shaderTexture.Sample(SampleType, input.tex + float2(texelSize * -4.0f, 0.0f)) * weight4;
    colour += shaderTexture.Sample(SampleType, input.tex + float2(texelSize * -3.0f, 0.0f)) * weight3;
    colour += shaderTexture.Sample(SampleType, input.tex + float2(texelSize * -2.0f, 0.0f)) * weight2;
    colour += shaderTexture.Sample(SampleType, input.tex + float2(texelSize * -1.0f, 0.0f)) * weight1;
    colour += shaderTexture.Sample(SampleType, input.tex) * weight0;
    colour += shaderTexture.Sample(SampleType, input.tex + float2(texelSize * 1.0f, 0.0f)) * weight1;
    colour += shaderTexture.Sample(SampleType, input.tex + float2(texelSize * 2.0f, 0.0f)) * weight2;
    colour += shaderTexture.Sample(SampleType, input.tex + float2(texelSize * 3.0f, 0.0f)) * weight3;
    colour += shaderTexture.Sample(SampleType, input.tex + float2(texelSize * 4.0f, 0.0f)) * weight4;

    colour.a = 1.0f;
    return colour;
}
