#define MAX_LIGHTS 6
#define NUM_WAVES  4    // Number of waves to combine for final displacement

// Updated to include multi-light support
cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
    matrix lightView[MAX_LIGHTS];
    matrix lightProjection[MAX_LIGHTS];
    int numLights;
    float3 padding;
};

cbuffer CameraBuffer : register(b1)
{
    float3 cameraPos;
    float camPad;
};

cbuffer TimeBuffer : register(b2)
{
    float time;
    float3 waveProps; // x = amplitude scale, y = frequency scale, z = speed scale
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
    float3 viewVector : TEXCOORD2;
    float4 lightViewPos[MAX_LIGHTS] : TEXCOORD3;
};

// Gerstner wave parameters — direction, amplitude, wavelength, steepness, phase speed
static const float2 waveDir[NUM_WAVES] = { float2(1.0f, 0.0f), float2(0.7f, 0.7f),  float2(-0.4f, 0.9f), float2(0.3f, -0.8f) };
static const float waveAmp[NUM_WAVES] = { 0.35f, 0.20f, 0.15f, 0.08f };
static const float waveLen[NUM_WAVES] = { 12.0f, 8.0f, 5.0f, 3.0f };
static const float waveSteep[NUM_WAVES] = { 0.6f, 0.5f, 0.4f, 0.3f };
static const float waveSpeed[NUM_WAVES] = { 1.0f, 1.3f, 1.7f, 2.1f };

OutputType main(InputType input)
{
    OutputType output;

    // Read original XZ before displacement — used as input to wave function
    float3 originalPos = input.position.xyz;
    float2 posXZ = originalPos.xz;

    // Initialize sums for normal calculation
    float3 displaced = originalPos;
    float3 tangentSum = float3(1.0f, 0.0f, 0.0f);
    float3 bitangentSum = float3(0.0f, 0.0f, 1.0f);

    // Per-wave calculations for Gerstner waves (unrolled for performance)
    [unroll]
    for (int w = 0; w < NUM_WAVES; w++)
    {
        // Apply GUI controls
        float A = waveAmp[w] * waveProps.x;
        float L = waveLen[w] / max(waveProps.y, 0.0001f);
        float S = waveSpeed[w] * waveProps.z;

        // Gerstner wave calculations
        float k = 6.28318f / L;
        float freq = sqrt(9.81f * k);
        float phase = k * dot(waveDir[w], posXZ) + freq * time * S;

        float Q = waveSteep[w] / (k * A * NUM_WAVES); // normalise steepness

        float cosP = cos(phase);
        float sinP = sin(phase);

        // Position offsets
        displaced.x += Q * A * waveDir[w].x * cosP;
        displaced.z += Q * A * waveDir[w].y * cosP;
        displaced.y += A * sinP;

        float dSin = A * k * cosP;
        float dCos = Q * A * k * sinP;

        tangentSum.x += -waveDir[w].x * waveDir[w].x * dCos;
        tangentSum.y += waveDir[w].x * dSin;
        tangentSum.z += -waveDir[w].x * waveDir[w].y * dCos;

        bitangentSum.x += -waveDir[w].y * waveDir[w].x * dCos;
        bitangentSum.y += waveDir[w].y * dSin;
        bitangentSum.z += -waveDir[w].y * waveDir[w].y * dCos;
    }

    // Calculate normal from sums & update position
    float3 waveNormal = normalize(cross(bitangentSum, tangentSum));
    input.position.xyz = displaced;

    // Transform to world space
    float4 worldPos = mul(input.position, worldMatrix);
    output.position = mul(worldPos, viewMatrix);
    output.position = mul(output.position, projectionMatrix);
    
    output.tex = input.tex;
    output.normal = normalize(mul(waveNormal, (float3x3) worldMatrix));
    output.worldPos = worldPos.xyz;
    output.viewVector = normalize(cameraPos - worldPos.xyz);

    // Per-light shadow coords
    for (int i = 0; i < numLights; i++)
    {
        float4 lsPos = mul(worldPos, lightView[i]);
        lsPos = mul(lsPos, lightProjection[i]);
        output.lightViewPos[i] = lsPos;
    }

    return output;
}