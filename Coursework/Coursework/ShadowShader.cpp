// texture shader.cpp
#include "ShadowShader.h"


ShadowShader::ShadowShader(ID3D11Device* device, HWND hwnd) : BaseShader(device, hwnd)
{
	initShader(L"shadow_vs.cso", L"shadow_ps.cso");
}

ShadowShader::~ShadowShader()
{

	// Same code, just inline 
	if (sampleState)		{ sampleState->Release();		sampleState = 0; }
	if (sampleStateShadow)	{ sampleStateShadow->Release();	sampleStateShadow = 0; }
	if (matrixBuffer)		{ matrixBuffer->Release();		matrixBuffer = 0; }
	if (lightBuffer)		{ lightBuffer->Release();		lightBuffer = 0; }
	if (cameraBuffer)		{ cameraBuffer->Release();		cameraBuffer = 0; }
	if (layout)				{ layout->Release();			layout = 0; }

	BaseShader::~BaseShader();
}

void ShadowShader::initShader(const wchar_t* vsFilename, const wchar_t* psFilename)
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

	// Diffuse texture sampler
	D3D11_SAMPLER_DESC samplerDesc;
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	renderer->CreateSamplerState(&samplerDesc, &sampleState);

	// Shadow map sampler (border colour = 1 means outside the map reads as fully lit)
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

void ShadowShader::setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix,
	int numLights, Light* lights[MAX_LIGHTS], ID3D11ShaderResourceView* texture, ID3D11ShaderResourceView* shadowMaps[MAX_LIGHTS], XMFLOAT3 camPos, bool rgbToggle)
{
	// Safety clamp
	if (numLights > MAX_LIGHTS) numLights = MAX_LIGHTS;
	if (numLights < 0) numLights = 0;

	D3D11_MAPPED_SUBRESOURCE mappedResource;

	/// ------- MATRIX BUFFER (VS) -------
	XMMATRIX tworld = XMMatrixTranspose(worldMatrix);
	XMMATRIX tview = XMMatrixTranspose(viewMatrix);
	XMMATRIX tproj = XMMatrixTranspose(projectionMatrix);

	// Lock the constant buffer so it can be written to.
	MatrixBufferType* dataPtr;
	deviceContext->Map(matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

	dataPtr = (MatrixBufferType*)mappedResource.pData;
	dataPtr->world = tworld;// worldMatrix;
	dataPtr->view = tview;
	dataPtr->projection = tproj;
	dataPtr->numLights = numLights;
	dataPtr->padding = XMFLOAT3(0, 0, 0);

	// Iterate through view & projection matrices for each light
	for (int i = 0; i < MAX_LIGHTS; ++i)
	{
		if (i < numLights && lights[i])
		{
			dataPtr->lightView[i] = XMMatrixTranspose(lights[i]->getViewMatrix());
			dataPtr->lightProjection[i] = XMMatrixTranspose(lights[i]->getOrthoMatrix());
		}
		else
		{
			// Zero unused slots
			dataPtr->lightView[i] = XMMatrixIdentity();
			dataPtr->lightProjection[i] = XMMatrixIdentity();
		}
	}

	deviceContext->Unmap(matrixBuffer, 0);
	deviceContext->VSSetConstantBuffers(0, 1, &matrixBuffer);

	/// ------- CAMERA BUFFER (VS) -------
	CameraBufferType* camPtr;
	deviceContext->Map(cameraBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	
	camPtr = (CameraBufferType*)mappedResource.pData;
	camPtr->camPos = camPos;
	camPtr->padding = 0;
	
	deviceContext->Unmap(cameraBuffer, 0);
	deviceContext->VSSetConstantBuffers(1, 1, &cameraBuffer);

	/// ------- LIGHT BUFFER (PS) -------
	LightBufferType* lightPtr;
	deviceContext->Map(lightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	lightPtr = (LightBufferType*)mappedResource.pData;

	lightPtr->numLights = numLights;
	lightPtr->padding = XMFLOAT3(0, 0, 0);

	//Iterate through per-light data
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
		}
		else
		{
			// Zero unused slots
			lightPtr->lights[i] = LightType();
		}
	}

	deviceContext->Unmap(lightBuffer, 0);
	deviceContext->PSSetConstantBuffers(0, 1, &lightBuffer);

	/// ------- SRV + Samplers (PS) -------
	deviceContext->PSSetShaderResources(0, 1, &texture); //diffuse texture (t0)
	
	//Per-light shadow maps (t1 - t6)
	for (int i = 0; i < MAX_LIGHTS; ++i)
	{
		// Set SRV to shadow map if light exists, otherwise set to null (shader should ignore null SRVs)
		ID3D11ShaderResourceView* srv = (i < numLights && shadowMaps[i]) ? shadowMaps[i] : nullptr;
		deviceContext->PSSetShaderResources(1 + i, 1, &srv);
	}

	deviceContext->PSSetSamplers(0, 1, &sampleState);
	deviceContext->PSSetSamplers(1, 1, &sampleStateShadow);
}