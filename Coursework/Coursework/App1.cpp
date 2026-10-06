#include "App1.h"

App1::App1()
{
	for (int i = 0; i < MAX_LIGHTS; i++) { light[i] = nullptr; }
	for (int i = 0; i < MAX_LIGHTS; i++) { lightSphere[i] = nullptr; }
	for (int i = 0; i < MAX_LIGHTS; i++) { shadowMaps[i] = nullptr; }
	for (int i = 0; i < MAX_LIGHTS; i++) { shadowSRVs[i] = nullptr; }
}

void App1::init(HINSTANCE hinstance, HWND hwnd, int screenWidth, int screenHeight, Input *in, bool VSYNC, bool FULL_SCREEN)
{
	// Call super/parent init function (required!)
	BaseApplication::init(hinstance, hwnd, screenWidth, screenHeight, in, VSYNC, FULL_SCREEN);

	// ----- LOAD TEXTURES -----
	textureMgr->loadTexture(L"mountain", L"res/mountain.png");
	textureMgr->loadTexture(L"mTex", L"res/mountainTex.png");
	textureMgr->loadTexture(L"square", L"res/DefaultDiffuse.png");
	textureMgr->loadTexture(L"house", L"res/Farmhouse Texture.jpg");
	textureMgr->loadTexture(L"lamp", L"res/lamp_ao.png");

	textureMgr->loadTexture(L"brick", L"res/brick1.dds");
	textureMgr->loadTexture(L"wood", L"res/wood.png");

	// ----- INIT MESHES -----
	mountain = new PlaneMesh(renderer->getDevice(), renderer->getDeviceContext());
	wave = new PlaneMesh(renderer->getDevice(), renderer->getDeviceContext(), 200);
	floor = new PlaneMesh(renderer->getDevice(), renderer->getDeviceContext(), 20);
	sun = new SphereMesh(renderer->getDevice(), renderer->getDeviceContext(), 60);
	testCube = new CubeMesh(renderer->getDevice(), renderer->getDeviceContext());

	screenMesh = new OrthoMesh(renderer->getDevice(), renderer->getDeviceContext(), screenWidth, screenHeight);
	debugHUD = new OrthoMesh(renderer->getDevice(), renderer->getDeviceContext(), screenWidth / 4, screenHeight / 4, (int)(screenWidth / 2.7), (int)(screenHeight / 2.7));

	house = new AModel(renderer->getDevice(), "res/farmhouse.obj");
	lamp = new AModel(renderer->getDevice(), "res/Street_Lamp.obj");

	// ----- INIT SHADERS -----
	bShader = new BrightShader(renderer->getDevice(), hwnd);
	tShader = new TexShader(renderer->getDevice(), hwnd);
	hShader = new HeightShader(renderer->getDevice(), hwnd);
	lShader = new LightShader(renderer->getDevice(), hwnd);
	wShader = new WaveShader(renderer->getDevice(), hwnd);
	dShader = new DepthShader(renderer->getDevice(), hwnd);
	sShader = new ShadowShader(renderer->getDevice(), hwnd);
	hBlurShader = new HorizBlurShader(renderer->getDevice(), hwnd);
	vBlurShader = new VertBlurShader(renderer->getDevice(), hwnd);

	ssShader = new SimpleShadowShader(renderer->getDevice(), hwnd);

	// ----- INIT RENDER TEXTURES -----
	screenTex = new RenderTexture(renderer->getDevice(), screenWidth, screenHeight, SCREEN_NEAR, SCREEN_DEPTH);
	bTex = new RenderTexture(renderer->getDevice(), screenWidth, screenHeight, SCREEN_NEAR, SCREEN_DEPTH);
	dScaleTex = new RenderTexture(renderer->getDevice(), screenWidth / 2, screenHeight / 2, SCREEN_NEAR, SCREEN_DEPTH);
	hBlurTex = new RenderTexture(renderer->getDevice(), screenWidth / 2, screenHeight / 2, SCREEN_NEAR, SCREEN_DEPTH);
	vBlurTex = new RenderTexture(renderer->getDevice(), screenWidth / 2, screenHeight / 2, SCREEN_NEAR, SCREEN_DEPTH);
	uScaleTex = new RenderTexture(renderer->getDevice(), screenWidth, screenHeight, SCREEN_NEAR, SCREEN_DEPTH);

	// ----- INIT LIGHTS -----

	//To add more lights, increment numLights, then set up light[i] properties. Shadow maps will be generated dynamically based on numLights
	numLights = 2;

	// Light 0: directional sun
	light[0] = new Light();
	light[0]->setLightType(0);
	light[0]->setAmbientColour(0.12f, 0.15f, 0.22f, 1.0f);   // subtle blue ambient
	light[0]->setDiffuseColour(0.85f, 0.70f, 0.65f, 1.0f); 
	lightPos[0] = XMFLOAT3(75.0f, 50.0f, -25.0f);
	lightDir[0] = XMFLOAT3(-0.55f, -0.5f, 0.5f);
	light[0]->setPosition(lightPos[0].x, lightPos[0].y, lightPos[0].z);
	light[0]->setDirection(lightDir[0].x, lightDir[0].y, lightDir[0].z);
	light[0]->generateOrthoMatrix(200.f, 200.f, 0.1f, 200.f); // Larger ortho matrix to encompass whole scene

	// Light 1: point light at the lamp post
	light[1] = new Light();
	light[1]->setLightType(1);
	light[1]->setAmbientColour(0.0f, 0.0f, 0.0f, 1.0f);
	light[1]->setDiffuseColour(1.0f, 0.85f, 0.55f, 1.0f);
	lightPos[1] = XMFLOAT3(36.0f, 4.0f, 20.0f);
	lightDir[1] = XMFLOAT3(0.0f, -1.0f, 0.0f);
	light[1]->setPosition(lightPos[1].x, lightPos[1].y, lightPos[1].z);
	light[1]->setDirection(lightDir[1].x, lightDir[1].y, lightDir[1].z);
	light[1]->setAttenuation(1.0f, 0.15f, 0.03f);
	light[1]->generateOrthoMatrix(100.f, 100.f, 0.1f, 100.f); // Smaller ortho matrix to improve shadow map resolution for nearby objects

	// ----- INIT SHADOW MAPS -----
	int shadowMapSize = 2048;

	// Generate shadow maps and corresponding SRVs based on numLights, and generate light spheres for visualization in debug mode
	for (int i = 0; i < numLights; i++)
	{
		shadowMaps[i] = new ShadowMap(renderer->getDevice(), shadowMapSize, shadowMapSize);
		shadowSRVs[i] = nullptr;  // filled each frame in depthPass()
		lightSphere[i] = new SphereMesh(renderer->getDevice(), renderer->getDeviceContext());
	}


	// ----- Assign Var Properties -----
	camera->setPosition(0.0f, 10.0f, -10.0f);
	waveProps = XMFLOAT3(0.45f, 1.0f, 0.4f);
	rgbToggle = false;
	bloomToggle = false;
	debugLightToggle = true;
	totalTime = 0.0f;
	bloomLight = 0.7f;
	bloomStrength = 0.4f;
}

App1::~App1()
{
	// Run base application deconstructor
	BaseApplication::~BaseApplication();

	// Clean up memory
	if (mountain) { delete mountain;   mountain = nullptr; }
	if (wave) { delete wave;       wave = nullptr; }
	if (sun) { delete sun;        sun = nullptr; }
	if (floor) { delete floor;      floor = nullptr; }
	if (testCube) { delete testCube;   testCube = nullptr; }
	if (screenMesh) { delete screenMesh; screenMesh = nullptr; }
	if (debugHUD) { delete debugHUD;   debugHUD = nullptr; }
	if (house) { delete house;      house = nullptr; }
	if (lamp) { delete lamp;       lamp = nullptr; }

	if (hShader) { delete hShader;     hShader = nullptr; }
	if (bShader) { delete bShader;     bShader = nullptr; }
	if (lShader) { delete lShader;     lShader = nullptr; }
	if (wShader) { delete wShader;     wShader = nullptr; }
	if (tShader) { delete tShader;     tShader = nullptr; }
	if (dShader) { delete dShader;     dShader = nullptr; }
	if (sShader) { delete sShader;     sShader = nullptr; }
	if (ssShader) { delete ssShader;    ssShader = nullptr; }
	if (hBlurShader) { delete hBlurShader; hBlurShader = nullptr; }
	if (vBlurShader) { delete vBlurShader; vBlurShader = nullptr; }

	for (int i = 0; i < MAX_LIGHTS; i++)
	{
		if (light[i]) { delete light[i];       light[i] = nullptr; }
		if (lightSphere[i]) { delete lightSphere[i]; lightSphere[i] = nullptr; }
		if (shadowMaps[i]) { delete shadowMaps[i];  shadowMaps[i] = nullptr; }
	}

	if (screenTex) { delete screenTex; screenTex = nullptr; }
	if (bTex) { delete bTex;      bTex = nullptr; }
	if (dScaleTex) { delete dScaleTex; dScaleTex = nullptr; }
	if (hBlurTex) { delete hBlurTex;  hBlurTex = nullptr; }
	if (vBlurTex) { delete vBlurTex;  vBlurTex = nullptr; }
	if (uScaleTex) { delete uScaleTex; uScaleTex = nullptr; }
}

bool App1::frame()
{
	bool result;

	result = BaseApplication::frame();
	if (!result) { return false; }
	
	// Render the graphics.
	result = render();
	if (!result) { return false; }

	return true;
}

bool App1::render()
{
	totalTime += timer->getTime();

	depthPass();
	scenePass();
	brightPass();

	//Toggle Bloom effect
	if (bloomToggle)
	{
		downScalePass();
		horizontalBlurPass();
		verticalBlurPass();
		upScalePass();
	}

	//Combine passes
	finalPass();
	gui();

	// Present the rendered scene to the screen.
	renderer->endScene();
	return true;
}

void App1::depthPass()
{
	// Variable to make life easy later on
	ID3D11DeviceContext* dc = renderer->getDeviceContext();

	// One depth pass per active light
	for (int i = 0; i < numLights; i++)
	{
		if (!light[i] || !shadowMaps[i]) continue;

		shadowMaps[i]->BindDsvAndSetNullRenderTarget(dc);

		light[i]->generateViewMatrix();
		XMMATRIX lightView = light[i]->getViewMatrix();
		XMMATRIX lightProj = light[i]->getOrthoMatrix();

		// Cache SRV for scenePass to use later
		shadowSRVs[i] = shadowMaps[i]->getDepthMapSRV();

		XMMATRIX worldMatrix;

		// Waves — no heightmap
		worldMatrix = XMMatrixTranslation(-50.f, 0.0f, -50.0f);
		RenderObject(dShader, wave, [=]() { dShader->setShaderParameters(dc, worldMatrix, lightView, lightProj); });

		// Mountain — pass heightmap so depth VS displaces
		worldMatrix = renderer->getWorldMatrix() * XMMatrixTranslation(0.f, -4.0f, 0.0f);
		RenderObject(dShader, mountain, [=]() { dShader->setShaderParameters(dc, worldMatrix, lightView, lightProj, 
			textureMgr->getTexture(L"mountain")); });

		// Sun
		worldMatrix = XMMatrixScaling(2.f, 2.f, 2.f) * XMMatrixTranslation(lightPos[0].x, lightPos[0].y, lightPos[0].z);
		RenderObject(dShader, sun, [=]() { dShader->setShaderParameters(dc, worldMatrix, lightView, lightProj); });

		// House
		worldMatrix = XMMatrixScaling(0.1f, 0.1f, 0.1f) * XMMatrixRotationRollPitchYaw(0.0f, 2.6f, 0.0f) * XMMatrixTranslation(22.f, 1.65f, 20.f);
		RenderObject(dShader, house, [=]() { dShader->setShaderParameters(dc, worldMatrix, lightView, lightProj); });

		// Lamp
		worldMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixTranslation(36.0f, 1.2f, 20.0f);
		RenderObject(dShader, lamp, [=]() { dShader->setShaderParameters(dc, worldMatrix, lightView, lightProj); });

		renderer->setBackBufferRenderTarget();
		renderer->resetViewport();
	}
}

void App1::scenePass()
{
	ID3D11DeviceContext* dc = renderer->getDeviceContext();

	// Set the render target to be the render texture and clear it
	screenTex->setRenderTarget(dc);
	screenTex->clearRenderTarget(dc, 0.53f, 0.81f, 0.98f, 1.0f);  // sky blue

	// Get matrices
	camera->update();
	XMMATRIX worldMatrix = renderer->getWorldMatrix();
	XMMATRIX viewMatrix = camera->getViewMatrix();
	XMMATRIX projectionMatrix = renderer->getProjectionMatrix();
	XMMATRIX lightViewMatrix = light[0]->getViewMatrix();
	XMMATRIX lightProjectionMatrix = light[0]->getOrthoMatrix();
	XMFLOAT3 camPos = camera->getPosition();

	// ----- Render Objects with Shadows -----

	// Waves — multi-light capable with Gerstner waves
	worldMatrix = XMMatrixTranslation(-50.f, 0.0f, -50.0f);
	RenderObject(wShader, wave, [=]() {
		wShader->setShaderParameters(dc, worldMatrix, viewMatrix, projectionMatrix,
			numLights, light, textureMgr->getTexture(L"square"), shadowSRVs, camPos, waveProps, totalTime, rgbToggle);
	});

	// Mountain - multi-light capable
	worldMatrix = renderer->getWorldMatrix() * XMMatrixTranslation(0.f, -4.0f, 0.0f);
	RenderObject(hShader, mountain, [=]() {
		hShader->setShaderParameters(dc, worldMatrix, viewMatrix, projectionMatrix,
			numLights, light, textureMgr->getTexture(L"mTex"), textureMgr->getTexture(L"mountain"), shadowSRVs, camPos, rgbToggle);
	});

	// Sun sphere — unlit for guaranteed brightness
	worldMatrix = XMMatrixScaling(2.f, 2.f, 2.f) * XMMatrixTranslation(lightPos[0].x, lightPos[0].y, lightPos[0].z);
	RenderObject(lShader, sun, [=]() {
		lShader->setShaderParameters(dc, worldMatrix, viewMatrix, projectionMatrix,
			textureMgr->getTexture(L"square"), light, camPos, rgbToggle);
	});

	// House - multi-light capable
	worldMatrix = XMMatrixScaling(0.1f, 0.1f, 0.1f) * XMMatrixRotationRollPitchYaw(0.0f, 2.6f, 0.0f) * XMMatrixTranslation(22.f, 1.65f, 20.f);
	RenderObject(sShader, house, [=]() {
		sShader->setShaderParameters(dc, worldMatrix, viewMatrix, projectionMatrix,
			numLights, light, textureMgr->getTexture(L"house"), shadowSRVs, camPos, rgbToggle);
	});

	// Lamp - multi-light capable
	worldMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixTranslation(36.0f, 1.2f, 20.0f);
	RenderObject(sShader, lamp, [=]() {
		sShader->setShaderParameters(dc, worldMatrix, viewMatrix, projectionMatrix,
			numLights, light, textureMgr->getTexture(L"lamp"), shadowSRVs, camPos, rgbToggle);
	});

	// Debug light spheres
	if (debugLightToggle)
	{
		// Reusable single-light array (avoids re-declaring each iteration)
		Light* singleLight[MAX_LIGHTS] = { nullptr };

		for (int i = 0; i < numLights; i++)
		{
			if (!light[i] || !lightSphere[i]) continue;

			XMFLOAT3 pos = light[i]->getPosition();
			worldMatrix = XMMatrixScaling(0.5f, 0.5f, 0.5f) * XMMatrixTranslation(pos.x, pos.y, pos.z);

			// Point slot 0 at the current light
			singleLight[0] = light[i];

			RenderObject(lShader, lightSphere[i], [=, &singleLight]() {
				lShader->setShaderParameters(dc, worldMatrix, viewMatrix, projectionMatrix,
					textureMgr->getTexture(L"square"), singleLight, camPos, rgbToggle, true);
				});
		}
	}

	// Debug: Display shadowSRVs 
	if (debugToggle)
	{
		renderer->setZBuffer(false);
		worldMatrix = renderer->getWorldMatrix();
		XMMATRIX orthoView = camera->getOrthoViewMatrix();
		XMMATRIX orthoProj = renderer->getOrthoMatrix();

		debugHUD->sendData(dc);
		tShader->setShaderParameters(dc, worldMatrix, orthoView, orthoProj, bTex->getShaderResourceView());
		tShader->render(dc, debugHUD->getIndexCount());
		renderer->setZBuffer(true);
	}

	renderer->setBackBufferRenderTarget();
}

void App1::brightPass()
{
	ID3D11DeviceContext* dc = renderer->getDeviceContext();

	//Set render target & clear
	bTex->setRenderTarget(dc);
	bTex->clearRenderTarget(dc, 0.98f, 0.60f, 0.70f, 1.0f);

	renderer->setZBuffer(false);
	XMMATRIX worldMatrix = renderer->getWorldMatrix();
	XMMATRIX orthoViewMatrix = camera->getOrthoViewMatrix();
	XMMATRIX orthoMatrix = dScaleTex->getOrthoMatrix();

	//Render bright areas of the scene only // Change this to new shader when made
	RenderObject(bShader, screenMesh, [=]() { bShader->setShaderParameters(dc, worldMatrix, orthoViewMatrix, orthoMatrix, screenTex->getShaderResourceView(), bloomLight); });
	renderer->setZBuffer(true);

	renderer->setBackBufferRenderTarget();
}

void App1::downScalePass()
{
	ID3D11DeviceContext* dc = renderer->getDeviceContext();

	dScaleTex->setRenderTarget(dc);
	dScaleTex->clearRenderTarget(dc, 0.98f, 0.60f, 0.70f, 1.0f);

	renderer->setZBuffer(false);
	XMMATRIX worldMatrix = renderer->getWorldMatrix();
	XMMATRIX orthoViewMatrix = camera->getOrthoViewMatrix();
	XMMATRIX orthoMatrix = dScaleTex->getOrthoMatrix();

	RenderObject(tShader, screenMesh, [=]() { tShader->setShaderParameters(dc,worldMatrix, orthoViewMatrix, orthoMatrix, bTex->getShaderResourceView()); });
	renderer->setZBuffer(true);

	renderer->setBackBufferRenderTarget();
}

void App1::horizontalBlurPass()
{
	ID3D11DeviceContext* dc = renderer->getDeviceContext();

	float screenSizeX = (float)dScaleTex->getTextureWidth();
	hBlurTex->setRenderTarget(dc);
	hBlurTex->clearRenderTarget(dc, 0.98f, 0.60f, 0.70f, 1.0f);

	renderer->setZBuffer(false);
	XMMATRIX worldMatrix = renderer->getWorldMatrix();
	XMMATRIX orthoViewMatrix = camera->getOrthoViewMatrix();
	XMMATRIX orthoMatrix = hBlurTex->getOrthoMatrix();

	RenderObject(hBlurShader, screenMesh, [=]() { hBlurShader->setShaderParameters(dc, worldMatrix, orthoViewMatrix, orthoMatrix, dScaleTex->getShaderResourceView(), screenSizeX); });
	renderer->setZBuffer(true);

	renderer->setBackBufferRenderTarget();
}

void App1::verticalBlurPass()
{
	ID3D11DeviceContext* dc = renderer->getDeviceContext();

	float screenSizeY = (float)hBlurTex->getTextureHeight();
	vBlurTex->setRenderTarget(dc);
	vBlurTex->clearRenderTarget(dc, 0.98f, 0.60f, 0.70f, 1.0f);

	renderer->setZBuffer(false);
	XMMATRIX worldMatrix = renderer->getWorldMatrix();
	XMMATRIX orthoViewMatrix = camera->getOrthoViewMatrix();
	XMMATRIX orthoMatrix = vBlurTex->getOrthoMatrix();

	RenderObject(vBlurShader, screenMesh, [=]() { vBlurShader->setShaderParameters(dc, worldMatrix, orthoViewMatrix, orthoMatrix, hBlurTex->getShaderResourceView(), screenSizeY); });
	renderer->setZBuffer(true);

	renderer->setBackBufferRenderTarget();
}

void App1::upScalePass()
{
	ID3D11DeviceContext* dc = renderer->getDeviceContext();

	//Set render target & clear
	uScaleTex->setRenderTarget(dc);
	uScaleTex->clearRenderTarget(dc, 0.98f, 0.60f, 0.70f, 1.0f);

	renderer->setZBuffer(false);
	XMMATRIX worldMatrix = renderer->getWorldMatrix();
	XMMATRIX orthoViewMatrix = camera->getOrthoViewMatrix();
	XMMATRIX orthoMatrix = uScaleTex->getOrthoMatrix();

	RenderObject(tShader, screenMesh, [=]() { tShader->setShaderParameters(dc, worldMatrix, orthoViewMatrix, orthoMatrix, vBlurTex->getShaderResourceView()); });
	renderer->setZBuffer(true);

	renderer->setBackBufferRenderTarget();
}

void App1::finalPass()
{
	ID3D11DeviceContext* dc = renderer->getDeviceContext();

	// Clear the back buffer (sky blue matches scenePass so any letterboxing looks natural)
	renderer->beginScene(0.53f, 0.81f, 0.98f, 1.0f);
	camera->update();

	// 2D composite: draw the fullscreen ortho quad with the final scene texture
	renderer->setZBuffer(false);

	XMMATRIX worldMatrix = renderer->getWorldMatrix();
	XMMATRIX orthoViewMatrix = camera->getOrthoViewMatrix();
	XMMATRIX orthoMatrix = renderer->getOrthoMatrix();

	if (bloomToggle)
	{
		// Composite scene + upscaled blurred bloom layer
		RenderObject(tShader, screenMesh, [=]() {
			tShader->setShaderParameters(dc, worldMatrix, orthoViewMatrix, orthoMatrix,
				screenTex->getShaderResourceView(),
				uScaleTex->getShaderResourceView(),
				bloomStrength, bloomToggle);
			});
	}
	else
	{
		// Scene only, no bloom
		RenderObject(tShader, screenMesh, [=]() {
			tShader->setShaderParameters(dc, worldMatrix, orthoViewMatrix, orthoMatrix,
				screenTex->getShaderResourceView());
			});
	}

	renderer->setZBuffer(true);
}

void App1::gui()
{
	// Force turn off unnecessary shader stages
	renderer->getDeviceContext()->GSSetShader(NULL, NULL, 0);
	renderer->getDeviceContext()->HSSetShader(NULL, NULL, 0);
	renderer->getDeviceContext()->DSSetShader(NULL, NULL, 0);

	// Performance 
	ImGui::Text("FPS: %.2f", timer->getFPS());
	ImGui::Separator();

	// Debug toggles
	ImGui::Text("Debug");
	ImGui::Checkbox("Wireframe mode", &wireframeToggle);
	ImGui::Checkbox("RGB Normals", &rgbToggle);
	ImGui::Checkbox("Shadow HUD", &debugToggle);
	ImGui::Checkbox("Light Spheres", &debugLightToggle);
	ImGui::Separator();

	// Bloom 
	ImGui::Text("Post-processing");
	ImGui::Checkbox("Bloom Enabled", &bloomToggle);
	ImGui::SliderFloat("Bloom Threshold", &bloomLight, 0.0f, 2.0f);
	ImGui::SliderFloat("Bloom Strength", &bloomStrength, 0.0f, 2.0f);
	ImGui::Separator();

	// Waves 
	ImGui::Text("Wave Properties");
	ImGui::SliderFloat("Wave Amplitude", &waveProps.x, 0.0f, 5.0f);
	ImGui::SliderFloat("Wave Frequency", &waveProps.y, 0.0f, 5.0f);
	ImGui::SliderFloat("Wave Speed", &waveProps.z, 0.0f, 20.0f);
	ImGui::Separator();

	// Dynamic Light Controls 
	// One collapsing section per active light, generated automatically
	ImGui::Text("Lights");
	for (int i = 0; i < numLights; i++)
	{
		if (!light[i]) continue;

		const char* typeName = (light[i]->getLightType() == 0) ? "Directional" :
			(light[i]->getLightType() == 1) ? "Point" : "Spot";

		char header[64];
		sprintf_s(header, "Light %d (%s)", i, typeName);

		if (ImGui::CollapsingHeader(header))
		{
			// Unique widget IDs so ImGui doesn't confuse controls between lights
			char posLabel[32], dirLabel[32], diffLabel[32], attenLabel[32];
			sprintf_s(posLabel, "Position##%d", i);
			sprintf_s(dirLabel, "Direction##%d", i);
			sprintf_s(diffLabel, "Diffuse##%d", i);
			sprintf_s(attenLabel, "Attenuation##%d", i);

			// Position (all light types)
			ImGui::SliderFloat3(posLabel, &lightPos[i].x, -100.0f, 100.0f);
			light[i]->setPosition(lightPos[i].x, lightPos[i].y, lightPos[i].z);

			// Direction only for directional and spot lights
			if (light[i]->getLightType() != 1)
			{
				ImGui::SliderFloat3(dirLabel, &lightDir[i].x, -1.0f, 1.0f);
				light[i]->setDirection(lightDir[i].x, lightDir[i].y, lightDir[i].z);
			}

			// Diffuse colour picker
			XMFLOAT4 col = light[i]->getDiffuseColour();
			float rgb[3] = { col.x, col.y, col.z };
			if (ImGui::ColorEdit3(diffLabel, rgb))
				light[i]->setDiffuseColour(rgb[0], rgb[1], rgb[2], 1.0f);

			//Ambient colour picker
			XMFLOAT4 amb = light[i]->getAmbientColour();
			float ambRGB[3] = { amb.x, amb.y, amb.z };
			char ambLabel[32];
			sprintf_s(ambLabel, "Ambient##%d", i);
			if (ImGui::ColorEdit3(ambLabel, ambRGB))
				light[i]->setAmbientColour(ambRGB[0], ambRGB[1], ambRGB[2], 1.0f);
		}
	}

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

//RenderObject function with lambdas for code clarity
void App1::RenderObject(BaseShader* shader, BaseMesh* mesh, std::function<void()> setParams)
{
	if (!shader || !mesh) return; //safety

	ID3D11DeviceContext* dc = renderer->getDeviceContext();
	mesh->sendData(dc);    // send vertex/index buffers
	setParams();           // call the shader-specific parameter setup
	shader->render(dc, mesh->getIndexCount());
}