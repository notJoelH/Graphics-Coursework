#pragma once
#include "DXF.h"

using namespace std;
using namespace DirectX;

#define MAX_LIGHTS 6

class LightShader : public BaseShader
{
private:
	struct LightType
	{
		XMFLOAT4 ambient;
		XMFLOAT4 diffuse;

		XMFLOAT3 position;
		float angle;

		XMFLOAT3 direction;
		float pad0;

		int lightType;
		int rgbnorms;
		XMFLOAT2 pad1;

		XMFLOAT3 attenFacs;
		float specPower;

		XMFLOAT4 specular;
	};

	struct LightBufferType
	{
		LightType lights[MAX_LIGHTS];
		int       numLights;
		int       emissiveMode;   // NEW
		XMFLOAT2  pad2;           
	};

	struct CameraBufferType
	{
		XMFLOAT3 cameraPos;
		float padding;
	};

public:
	LightShader(ID3D11Device* device, HWND hwnd);
	~LightShader();

	void LightShader::setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX& worldMatrix,
		const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix, ID3D11ShaderResourceView* texture,
		Light* lights[MAX_LIGHTS], XMFLOAT3 camPos, bool rgbToggle, bool emissive = false);

private:
	void initShader(const wchar_t* cs, const wchar_t* ps);

private:
	ID3D11Buffer* matrixBuffer;
	ID3D11SamplerState* sampleState;
	ID3D11Buffer* lightBuffer;
	ID3D11Buffer* cameraBuffer;

	int activeLights;
};

