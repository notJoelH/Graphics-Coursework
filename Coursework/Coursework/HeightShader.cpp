#include "HeightShader.h"

HeightShader::HeightShader(ID3D11Device* device, HWND hwnd) : BaseShader(device, hwnd)
{
	initShader(L"height_vs.cso", L"height_ps.cso");
}

HeightShader::~HeightShader()
{
	if (sampleState) { sampleState->Release();       sampleState = 0; }
	if (sampleStateShadow) { sampleStateShadow->Release(); sampleStateShadow = 0; }
	if (matrixBuffer) { matrixBuffer->Release();      matrixBuffer = 0; }
	if (lightBuffer) { lightBuffer->Release();       lightBuffer = 0; }
	if (cameraBuffer) { cameraBuffer->Release();      cameraBuffer = 0; }
	if (layout) { layout->Release();            layout = 0; }
	BaseShader::~BaseShader();
}

void HeightShader::initShader(const wchar_t* vsFilename, const wchar_t* psFilename)
{
    loadVertexShader(vsFilename);
    loadPixelShader(psFilename);

    // Shared Buffer Description for Matrix, Light, and Camera Buffers
    // Saves on code duplication and ensures consistency in buffer creation
    D3D11_BUFFER_DESC bufDesc;
    bufDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bufDesc.MiscFlags = 0;
    bufDesc.StructureByteStride = 0;

    bufDesc.ByteWidth = sizeof(MatrixBufferType);
    renderer->CreateBuffer(&bufDesc, NULL, &matrixBuffer);

    bufDesc.ByteWidth = sizeof(LightBufferType);
    renderer->CreateBuffer(&bufDesc, NULL, &lightBuffer);

    bufDesc.ByteWidth = sizeof(CameraBufferType);
    renderer->CreateBuffer(&bufDesc, NULL, &cameraBuffer);

    // Diffuse sampler
    D3D11_SAMPLER_DESC samplerDesc;
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
    renderer->CreateSamplerState(&samplerDesc, &sampleState);

    // Shadow map sampler
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
    samplerDesc.BorderColor[0] = 1.0f;
    samplerDesc.BorderColor[1] = 1.0f;
    samplerDesc.BorderColor[2] = 1.0f;
    samplerDesc.BorderColor[3] = 1.0f;
    renderer->CreateSamplerState(&samplerDesc, &sampleStateShadow);
}

void HeightShader::setShaderParameters(
    ID3D11DeviceContext* deviceContext,
    const XMMATRIX& worldMatrix,
    const XMMATRIX& viewMatrix,
    const XMMATRIX& projectionMatrix,
    int numLights,
    Light* lights[MAX_LIGHTS],
    ID3D11ShaderResourceView* diffuseTexture,
    ID3D11ShaderResourceView* heightmapTexture,
    ID3D11ShaderResourceView* shadowMaps[MAX_LIGHTS],
    XMFLOAT3 camPos,
    bool rgbToggle)
{
    if (numLights > MAX_LIGHTS) numLights = MAX_LIGHTS;
    if (numLights < 0)          numLights = 0;

    D3D11_MAPPED_SUBRESOURCE mappedResource;

    // ---- MATRIX BUFFER (VS b0) ----
    MatrixBufferType* matPtr;
    deviceContext->Map(matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    matPtr = (MatrixBufferType*)mappedResource.pData;

    matPtr->world = XMMatrixTranspose(worldMatrix);
    matPtr->view = XMMatrixTranspose(viewMatrix);
    matPtr->projection = XMMatrixTranspose(projectionMatrix);
    matPtr->numLights = numLights;
    matPtr->padding = XMFLOAT3(0, 0, 0);

    for (int i = 0; i < MAX_LIGHTS; ++i)
    {
        if (i < numLights && lights[i])
        {
            matPtr->lightView[i] = XMMatrixTranspose(lights[i]->getViewMatrix());
            matPtr->lightProjection[i] = XMMatrixTranspose(lights[i]->getOrthoMatrix());
        }
        else
        {
            matPtr->lightView[i] = XMMatrixIdentity();
            matPtr->lightProjection[i] = XMMatrixIdentity();
        }
    }

    deviceContext->Unmap(matrixBuffer, 0);
    deviceContext->VSSetConstantBuffers(0, 1, &matrixBuffer);

    // ---- CAMERA BUFFER (VS b1) ----
    CameraBufferType* camPtr;
    deviceContext->Map(cameraBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    camPtr = (CameraBufferType*)mappedResource.pData;
    camPtr->camPos = camPos;
    camPtr->padding = 0.0f;
    deviceContext->Unmap(cameraBuffer, 0);
    deviceContext->VSSetConstantBuffers(1, 1, &cameraBuffer);

    // ---- LIGHT BUFFER (PS b0) ----
    LightBufferType* lightPtr;
    deviceContext->Map(lightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    lightPtr = (LightBufferType*)mappedResource.pData;

    lightPtr->numLights = numLights;
    lightPtr->padding = XMFLOAT3(0, 0, 0);

    for (int i = 0; i < MAX_LIGHTS; ++i)
    {
        if (i < numLights && lights[i])
        {
            lightPtr->lights[i].ambient = lights[i]->getAmbientColour();
            lightPtr->lights[i].diffuse = lights[i]->getDiffuseColour();
            lightPtr->lights[i].position = lights[i]->getPosition();
            lightPtr->lights[i].angle = lights[i]->getAngle();
            lightPtr->lights[i].direction = lights[i]->getDirection();
            lightPtr->lights[i].lightType = lights[i]->getLightType();
            lightPtr->lights[i].rgbnorms = rgbToggle ? 1 : 0;
            lightPtr->lights[i].attenFacs = lights[i]->getAttenuation();
            lightPtr->lights[i].specPower = lights[i]->getSpecularPower();
            lightPtr->lights[i].specular = lights[i]->getSpecularColour();
            lightPtr->lights[i].pad0 = 0.f;
            lightPtr->lights[i].pad1 = XMFLOAT2(0.f, 0.f);
        }
        else
        {
            lightPtr->lights[i] = {};
        }
    }

    deviceContext->Unmap(lightBuffer, 0);
    deviceContext->PSSetConstantBuffers(0, 1, &lightBuffer);

    // ---- SRVs (PS) ----
    // t0 = diffuse, t1 = heightmap, t2..t7 = per-light shadow maps
    deviceContext->PSSetShaderResources(0, 1, &diffuseTexture);
    deviceContext->PSSetShaderResources(1, 1, &heightmapTexture);

    // The heightmap also needs to be bound to the VS for displacement
    deviceContext->VSSetShaderResources(0, 1, &heightmapTexture);
    deviceContext->VSSetSamplers(0, 1, &sampleState);

    for (int i = 0; i < MAX_LIGHTS; ++i)
    {
        ID3D11ShaderResourceView* srv = (i < numLights && shadowMaps[i]) ? shadowMaps[i] : nullptr;
        deviceContext->PSSetShaderResources(2 + i, 1, &srv);
    }

    deviceContext->PSSetSamplers(0, 1, &sampleState);
    deviceContext->PSSetSamplers(1, 1, &sampleStateShadow);
}
