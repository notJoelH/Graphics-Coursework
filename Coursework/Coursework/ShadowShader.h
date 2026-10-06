#ifndef _SHADOWSHADER_H_
#define _SHADOWSHADER_H_
#define MAX_LIGHTS 6

#include "DXF.h"

using namespace std;
using namespace DirectX;

class ShadowShader : public BaseShader
{
private:
	struct MatrixBufferType
	{
		XMMATRIX world;
		XMMATRIX view;
		XMMATRIX projection;
		XMMATRIX lightView[MAX_LIGHTS];
		XMMATRIX lightProjection[MAX_LIGHTS];
		int numLights;
		XMFLOAT3 padding;
	};

	struct LightType
	{
		XMFLOAT4 ambient;
		XMFLOAT4 diffuse;
		XMFLOAT3 position;
		float angle;
		XMFLOAT3 direction;
		int lightType;
		int rgbnorms;
		XMFLOAT3 attenFacs;
		XMFLOAT3 padding;
		float specPower;
		XMFLOAT4 specular;
	};

	struct LightBufferType
	{
		LightType lights[MAX_LIGHTS];
		int numLights;
		XMFLOAT3 padding;
	};

	struct CameraBufferType
	{
		XMFLOAT3 camPos;
		float padding;
	};

public:

	ShadowShader(ID3D11Device* device, HWND hwnd);
	~ShadowShader();

	void setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix,
		 int numLights, Light* lights[MAX_LIGHTS], ID3D11ShaderResourceView* texture, ID3D11ShaderResourceView* shadowMaps[MAX_LIGHTS], XMFLOAT3 camPos, bool rgbToggle);

private:
	void initShader(const wchar_t* vs, const wchar_t* ps);

	// --- Buffers --- 
	ID3D11Buffer* matrixBuffer;
	ID3D11Buffer* lightBuffer;
	ID3D11Buffer* cameraBuffer;
	ID3D11SamplerState* sampleState;
	ID3D11SamplerState* sampleStateShadow;
};

#endif