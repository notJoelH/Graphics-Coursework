#pragma once

#include "DXF.h"

using namespace std;
using namespace DirectX;

class DepthShader : public BaseShader
{

public:

	DepthShader(ID3D11Device* device, HWND hwnd);
	~DepthShader();

	//Pass height map as optional parameter
	void setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX& world, const XMMATRIX& view, const XMMATRIX& projection, 
		ID3D11ShaderResourceView* height = nullptr);

private:
	void initShader(const wchar_t* vs, const wchar_t* ps);

	struct MatrixBufferType
	{
		XMMATRIX world;
		XMMATRIX view;
		XMMATRIX projection;
		int      useHeightmap; 
		XMFLOAT3 padding;
	};

private:
	ID3D11Buffer* matrixBuffer;
	ID3D11SamplerState* sampleState;
};