#pragma once
#include "BaseShader.h"

using namespace std;
using namespace DirectX;

class BrightShader : public BaseShader
{
private:
	struct BrightBufferType
	{
		float lightValue;
		XMFLOAT3 padding;
	};

public:
	BrightShader(ID3D11Device* device, HWND hwnd);
	~BrightShader();

	void setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX& world, const XMMATRIX& view, const XMMATRIX& projection, 
		ID3D11ShaderResourceView* texture, float lValue);

private:
	void initShader(const wchar_t* vs, const wchar_t* ps);

private:
	ID3D11Buffer* matrixBuffer;
	ID3D11Buffer* brightBuffer;
	ID3D11SamplerState* sampleState;
};