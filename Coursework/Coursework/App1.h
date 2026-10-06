// Application.h
#ifndef _APP1_H
#define _APP1_H

#define MAX_LIGHTS 6

// Includes
#include "DXF.h"
#include "BrightShader.h"
#include "TexShader.h"
#include "HeightShader.h"
#include "LightShader.h"
#include "WaveShader.h"
#include "DepthShader.h"
#include "ShadowShader.h"
#include "SimpleShadowShader.h"
#include "VertBlurShader.h"
#include "HorizBlurShader.h"
#include <functional>

class App1 : public BaseApplication
{
public:

	App1();
	~App1();
	void init(HINSTANCE hinstance, HWND hwnd, int screenWidth, int screenHeight, Input* in, bool VSYNC, bool FULL_SCREEN);

	bool frame();

protected:
	bool render();
	void gui();

	// --- Passes ---
	void depthPass();
	void scenePass();
	void brightPass();
	void downScalePass();
	void horizontalBlurPass();
	void verticalBlurPass();
	void upScalePass();
	void finalPass();

	// Void lambda for code clarity
	void RenderObject(BaseShader* shader, BaseMesh* mesh, std::function<void()> setParams);

private:
	
	// --- Geometry ---
	PlaneMesh* mountain		= nullptr;
	PlaneMesh* wave			= nullptr;
	SphereMesh* sun			= nullptr;
	PlaneMesh* floor		= nullptr;
	CubeMesh* testCube		= nullptr; // test item, remove when complete
	OrthoMesh* screenMesh	= nullptr;
	OrthoMesh* debugHUD		= nullptr;
	AModel* house			= nullptr;
	AModel* lamp			= nullptr;

	// --- Shaders ---
	BrightShader* bShader			= nullptr;
	TexShader* tShader				= nullptr;
	HeightShader* hShader			= nullptr;
	LightShader* lShader			= nullptr;
	WaveShader* wShader				= nullptr;
	DepthShader* dShader			= nullptr;
	ShadowShader* sShader			= nullptr;
	HorizBlurShader* hBlurShader	= nullptr;
	VertBlurShader* vBlurShader		= nullptr;

	// --- Lighting ---
	// To add more lights first increment numLights and set up new light[i] properties
	int numLights = 0;
	Light* light[MAX_LIGHTS];
	XMFLOAT3 lightPos[MAX_LIGHTS];
	XMFLOAT3 lightDir[MAX_LIGHTS];

	SphereMesh* lightSphere[MAX_LIGHTS]; // For visualization of lights

	// --- Shadows --- 
	// Edited to dynamically create shadows based on numLights
	ShadowMap* shadowMaps[MAX_LIGHTS];
	ID3D11ShaderResourceView* shadowSRVs[MAX_LIGHTS];

	// --- Render Textures ---
	RenderTexture* screenTex	= nullptr;
	RenderTexture* bTex			= nullptr;
	RenderTexture* dScaleTex	= nullptr;
	RenderTexture* hBlurTex		= nullptr;
	RenderTexture* vBlurTex		= nullptr;
	RenderTexture* uScaleTex	= nullptr;

	// --- Waves ---
	XMFLOAT3 waveProps;
	float totalTime;

	// --- Bloom ---
	float bloomLight = 1.0f;
	float bloomStrength = 0.4f;

	// --- Toggles ---
	bool rgbToggle			= false;
	bool bloomToggle		= false;
	bool debugToggle		= false;
	bool debugLightToggle	= true;

	// --- Vars & Debug ---
	float rotation;
	XMFLOAT3 position;
	SimpleShadowShader* ssShader = nullptr;
};

#endif