#pragma once
#define MAX_LIGHTS 6

#include "DXF.h"

using namespace std;
using namespace DirectX;

class HeightShader : public BaseShader
{
private:
    struct MatrixBufferType
    {
        XMMATRIX world;
        XMMATRIX view;
        XMMATRIX projection;
        XMMATRIX lightView[MAX_LIGHTS];
        XMMATRIX lightProjection[MAX_LIGHTS];
        int      numLights;
        XMFLOAT3 padding;
    };

    struct LightType
    {
        XMFLOAT4 ambient;
        XMFLOAT4 diffuse;
        XMFLOAT3 position;
        float    angle;
        XMFLOAT3 direction;
        float    pad0;
        int      lightType;
        int      rgbnorms;
        XMFLOAT2 pad1;
        XMFLOAT3 attenFacs;
        float    specPower;
        XMFLOAT4 specular;
    };

    struct LightBufferType
    {
        LightType lights[MAX_LIGHTS];
        int       numLights;
        XMFLOAT3  padding;
    };

    struct CameraBufferType
    {
        XMFLOAT3 camPos;
        float    padding;
    };

public:
    HeightShader(ID3D11Device* device, HWND hwnd);
    ~HeightShader();

    void setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX& world, const XMMATRIX& view, const XMMATRIX& projection,
        int numLights, Light* lights[MAX_LIGHTS], ID3D11ShaderResourceView* diffuseTexture, ID3D11ShaderResourceView* heightmapTexture,
        ID3D11ShaderResourceView* shadowMaps[MAX_LIGHTS], XMFLOAT3 camPos, bool rgbToggle);

private:
    void initShader(const wchar_t* vs, const wchar_t* ps);

    ID3D11Buffer* matrixBuffer;
    ID3D11Buffer* lightBuffer;
    ID3D11Buffer* cameraBuffer;
    ID3D11SamplerState* sampleState;
    ID3D11SamplerState* sampleStateShadow;
};
