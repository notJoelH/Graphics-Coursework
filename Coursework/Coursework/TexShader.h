#pragma once
#include "BaseShader.h"

using namespace std;
using namespace DirectX;

class TexShader : public BaseShader
{
private:
	struct BloomBufferType
	{
		float bloomStrength;
		int bloomOn;
		XMFLOAT2 padding;
	};

public:
	TexShader(ID3D11Device* device, HWND hwnd);
	~TexShader();

	void setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX& world, const XMMATRIX& view, const XMMATRIX& projection, ID3D11ShaderResourceView* texture);
	void setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX& world, const XMMATRIX& view, const XMMATRIX& projection, ID3D11ShaderResourceView* tex1, ID3D11ShaderResourceView* tex2, float bloomValue, bool bloomToggle);

private:
	void initShader(const wchar_t* vs, const wchar_t* ps);

private:
	ID3D11Buffer* matrixBuffer;
	ID3D11Buffer* bloomBuffer;
	ID3D11SamplerState* sampleState;
};