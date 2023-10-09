#include "VolumeRenderer.h"

#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/intersect.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <fstream>
#include <iostream>
#include "dawn/utils/WGPUHelpers.h"
#include "dawn/utils/ComboRenderPipelineDescriptor.h"
#include "Input.h"
#include "ShaderUtil.h"
#include "Camera.h"
#include "Hotkeys.h"
#include "Helper.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
// define is used so that json throws exceptions
#define __EXCEPTIONS
#include <json/json.hpp>
using json = nlohmann::json;

#define M_PI 3.14159265358979323846

struct CameraData
{
	glm::mat4 view;
	glm::mat4 viewInv;
	glm::mat4 proj;
	float aspectRatio;
	float fov;
	char gap[8];
} cameraData;

struct ParamData
{
	int enableEarlyRayTermination;
	int enableJittering;
	int enableAmbientOcclusion;
	int enableSoftShadows;

	float interaction;
	float sampleRate;
	float aoRadius;
	float aoStrength;

	int aoNumSamples;
	float shadowQuality;
	float shadowStrength;
	float voxelSize;

	int enableVolumeA;
	int enableVolumeB;
	int enableVolumeC;
	int enableVolumeD;

	glm::vec4 clippingMask;
	glm::vec4 viewVector;
	glm::vec4 clippingPlaneOrigin;
	glm::vec4 clippingPlaneNormal;
	glm::vec4 clearColor;

	int enableAnnotations;
	int annotationVolume;
	int annotationPingPong;
	float shadowRadius;
} paramData;

struct EffectData
{
	glm::vec4 param0;
	glm::vec4 param1;
	glm::vec4 param2;
	glm::vec4 param3;
	glm::vec4 param4;
} effectData;

VolumeRenderer::VolumeRenderer()
{

}

VolumeRenderer::~VolumeRenderer()
{
#ifndef __EMSCRIPTEN__
	ImGui_ImplGlfw_Shutdown();
	ImGui_ImplWGPU_Shutdown();
	ImGui::DestroyContext();
#endif


#ifndef __EMSCRIPTEN__
	//bindGroup.Release();
	indexBuffer.Release();
	vertexBuffer.Release();
	volumePipeline.Release();
	//swapchain.Release();
	//device.Release();
	//wgpuBindGroupRelease(bindGroup);
	//wgpuBufferRelease(indxBuf);
	//wgpuBufferRelease(vertBuf);
	//wgpuRenderPipelineRelease(pipeline);
	//wgpuSwapChainRelease(swapchain);
	//wgpuQueueRelease(queue);
	//wgpuDeviceRelease(device);
#endif
}

void VolumeRenderer::reload()
{
	effectsBindGroup.clear();
	effectPipelines.clear();

	setupShaders();

	if (initialized)
	{
		annotationHandler->init();
		segmentationUpdateHandler->init();
		volumePipeline = setupVolumePipelineLayout();
		for (size_t i = 0; i < effectShaderModules.size(); i++)
		{
			wgpu::RenderPipeline pipeline = setupEffectPipelineLayout(effectShaderModules[i][0], effectShaderModules[i][1], effectTextureViews[i], effectTextureViews[0], effectParamBuffers[i]);
			effectPipelines.push_back(pipeline);
		}
		clippingPlaneRenderer->depthTextureView = depthStencilTextureView;
	}
}

void VolumeRenderer::init()
{
	std::cout << "initialize VolumeRenderer... [width: " << width << ", height: " << height << "]" << std::endl;
	std::cout << "load and compile shaders..." << std::endl;
	setupShaders();
	
	std::cout << "create resources..." << std::endl;
	createVolumeResources();
	createEffectResources();
	createAnnotationResources();

	std::cout << "set up volume pipeline layout..." << std::endl;
	volumePipeline = setupVolumePipelineLayout();

	std::cout << "set up effect pipeline layouts..." << std::endl;
	for (size_t i = 0; i < effectShaderModules.size(); i++)
	{
		wgpu::RenderPipeline pipeline = setupEffectPipelineLayout(effectShaderModules[i][0], effectShaderModules[i][1], effectTextureViews[i], effectTextureViews[0], effectParamBuffers[i]);
		effectPipelines.push_back(pipeline);
	}

	segmentationUpdateHandler = new SegmentationUpdateHandler(device);
	segmentationUpdateHandler->init();

	clippingPlaneRenderer->setVolumeTexture(volumeTextureViews[0]);
	clippingPlaneRenderer->transferFunctionBufferColor = transferFunctionBufferColor;
	clippingPlaneRenderer->transferFunctionBufferRamp1 = transferFunctionBufferRamp1;
	clippingPlaneRenderer->transferFunctionBufferRamp2 = transferFunctionBufferRamp2;
	clippingPlaneRenderer->volumeRatiosBuffer = volumeRatiosBuffer;
	clippingPlaneRenderer->depthTextureView = depthStencilTextureView;
	clippingPlaneRenderer->volumeRatio = volumeRatios[0];

	clippingPlaneRenderer->transferFunctionBufferColorSize = sizeof(glm::vec4)* transferFunctionColor.size();
	clippingPlaneRenderer->transferFunctionBufferRamp1Size = sizeof(float) * transferFunctionRampLow.size();
	clippingPlaneRenderer->transferFunctionBufferRamp2Size = sizeof(float) * transferFunctionRampHigh.size();
	clippingPlaneRenderer->volumeRatiosBufferSize = sizeof(glm::vec3) * volumeTextures.size();

	clippingPlaneRenderer->annotationTextureView = annotationTexturePingView;

	initialized = true;

	std::cout << " initialize VolumeRenderer done." << std::endl;
}

#ifndef __EMSCRIPTEN__
void VolumeRenderer::setupImGui(GLFWwindow* window)
{
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

#ifdef __EMSCRIPTEN__
	// For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
	// You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
	io.IniFilename = NULL;
#endif

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer backends				
	ImGui_ImplGlfw_InitForOther(window, true);
	//ImGui_ImplWGPU_Init(device.Get(), 3, WGPUTextureFormat_RGBA8Unorm);
	ImGui_ImplWGPU_Init(device.Get(), 3, WGPUTextureFormat(preferredSwapChainTextureFormat));

	// Load Fonts
		// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
		// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
		// - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
		// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
		// - Read 'docs/FONTS.md' for more instructions and details.
		// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
		// - Emscripten allows preloading a file or folder to be accessible at runtime. See Makefile for details.
		//io.Fonts->AddFontDefault();
#ifndef IMGUI_DISABLE_FILE_FUNCTIONS
				//io.Fonts->AddFontFromFileTTF("fonts/Roboto-Medium.ttf", 16.0f);
				//io.Fonts->AddFontFromFileTTF("fonts/Cousine-Regular.ttf", 15.0f);
				//io.Fonts->AddFontFromFileTTF("fonts/DroidSans.ttf", 16.0f);
				//io.Fonts->AddFontFromFileTTF("fonts/ProggyTiny.ttf", 10.0f);
				//ImFont* font = io.Fonts->AddFontFromFileTTF("fonts/ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
				//IM_ASSERT(font != NULL);
#endif
}
#endif

void VolumeRenderer::resize(int width, int height)
{
	std::cout << "VolumeRenderer::resize: " << width << " " << height << std::endl;
	this->width = width;
	this->height = height;

	init();
}

void VolumeRenderer::openVolume(std::string &file, bool openBySlice)
{
	VolumeLoader volumeLoader;
	bool success = volumeLoader.openVolumes(file);
	if (!success) {
		return;
	}
	
	if (openBySlice) {
		success = volumeLoader.loadAndUploadVolumesBySlice(device);
	}
	else {
		success = volumeLoader.loadAndUploadVolumes(device);
	}
	
	if (!success) {
		return;
	}

	loadSession("session.json");
	
	volumeTextures = volumeLoader.volumeTextures;
	volumeDescriptors = volumeLoader.volumeDescriptors;
	annotationHandler->volumeDescriptors = volumeDescriptors;
	uploadVolumeDescriptors(volumeLoader.volumeDescriptors);

	int sizeX = volumeLoader.volumeDescriptors[0].sizeX;
	int sizeY = volumeLoader.volumeDescriptors[0].sizeY;
	int sizeZ = volumeLoader.volumeDescriptors[0].sizeZ;
	
	numVolumes = volumeLoader.volumeDescriptors.size();

	std::cout << "create annotation textures." << std::endl;
	// create annotation textures according to volume size
	annotationTexturePing = volumeLoader.createTexture(device, sizeX, sizeY, sizeZ, wgpu::TextureFormat::RGBA8Unorm, true);
	annotationTexturePong = volumeLoader.createTexture(device, sizeX, sizeY, sizeZ, wgpu::TextureFormat::RGBA8Unorm, true);

	annotationHandler->setTexturePing(annotationTexturePing);
	annotationHandler->setTexturePong(annotationTexturePong);	
	annotationHandler->volumeRatios = volumeRatios;

	init();

	glm::uvec4 currentVolumeSize(sizeX, sizeY, sizeZ, 0);
	glm::vec3 currentVolumeRatio(volumeLoader.volumeDescriptors[0].voxelSizeX, volumeLoader.volumeDescriptors[0].voxelSizeY, volumeLoader.volumeDescriptors[0].voxelSizeZ);

	clippingPlaneRenderer->volumeSize = currentVolumeSize;
	clippingPlaneRenderer->volumeRatio = volumeRatios[0];
	clippingPlaneRenderer->paddingRatioX = paddingRatioX;
	clippingPlaneRenderer->init();

	annotationHandler->setVolumeRatio(currentVolumeRatio);
	annotationHandler->setVolumeSize(currentVolumeSize);
	annotationHandler->init();
}

void VolumeRenderer::updateSegmentationMask(int which, std::string maskFileName)
{
	if (volumeDescriptors.size() <= which)
	{
		std::cout << "Error: VolumeRenderer::updateMask: No Volume Descriptor for volume number: " << which << std::endl;
		return;
	}

	auto volumeDescriptor = volumeDescriptors[0];
	auto volumeTexture = volumeTextures[0];
	auto volumeTextureView = volumeTextureViews[0];

	auto maskDescriptor = volumeDescriptors[which];
	maskDescriptor.path = "";
	maskDescriptor.fileName = maskFileName;

	VolumeLoader loader;
	loader.volumeDescriptors.push_back(maskDescriptor);
	loader.loadAndUploadVolumesBySlice(device);

	auto segmentationTex = loader.volumeTextures[0];

	segmentationUpdateHandler->process(which, segmentationTex, volumeTexture, volumeTextureView, volumeDescriptor);
}

void VolumeRenderer::uploadVolumeDescriptors(const std::vector<VolumeDescriptor> &volumeDescriptors)
{
	volumeRatios.clear();
	float maxSize = 0.0f;

	for (int i = 0; i < volumeDescriptors.size(); i++)
	{
		VolumeDescriptor descriptor = volumeDescriptors[i];
		maxSize = std::max(std::max(descriptor.sizeX, descriptor.sizeY), descriptor.sizeZ);
		if (descriptor.sizeX != volumeDescriptors[0].sizeX ||
			descriptor.sizeY != volumeDescriptors[0].sizeY ||
			descriptor.sizeZ != volumeDescriptors[0].sizeZ)
		{
			std::cout << "Warning: Volumes have different sizes in json descriptor: "
				<< descriptor.sizeX << ", " << descriptor.sizeY << ", " << descriptor.sizeZ << " != "
				<< volumeDescriptors[0].sizeX << ", " << volumeDescriptors[0].sizeY << ", " << volumeDescriptors[0].sizeZ
				<< std::endl;
		}

	}
	for (int i = 0; i < volumeDescriptors.size(); i++)
	{
		VolumeDescriptor descriptor = volumeDescriptors[i];

		if (descriptor.voxelSizeX != volumeDescriptors[0].voxelSizeX ||
			descriptor.voxelSizeY != volumeDescriptors[0].voxelSizeY ||
			descriptor.voxelSizeZ != volumeDescriptors[0].voxelSizeZ)
		{
			std::cout << "Warning: Volumes have different ratios in json descriptor: "
				<< descriptor.voxelSizeX << ", " << descriptor.voxelSizeY << ", " << descriptor.voxelSizeZ << " != "
				<< volumeDescriptors[0].voxelSizeX << ", " << volumeDescriptors[0].voxelSizeY << ", " << volumeDescriptors[0].voxelSizeZ
				<< std::endl;
		}

		glm::vec3 ratio = glm::vec3(descriptor.voxelSizeX * descriptor.sizeX / maxSize, descriptor.voxelSizeY * descriptor.sizeY / maxSize, descriptor.voxelSizeZ * descriptor.sizeZ / maxSize);
		volumeRatios.push_back(ratio);
	}


	// prepare cpu buffers for upload to gpu
	transferFunctionColor.clear();
	transferFunctionRampLow.clear();
	transferFunctionRampHigh.clear();

	for (int i = 0; i < volumeDescriptors.size(); i++)
	{
		auto descriptor = volumeDescriptors[i];
		transferFunctionColor.push_back(descriptor.transferFunction.color);
		transferFunctionRampLow.push_back(descriptor.transferFunction.rampLow);
		transferFunctionRampHigh.push_back(descriptor.transferFunction.rampHigh);
	}
	
	std::cout << "create volume metadata GPU buffer." << std::endl;

	// upload data to gpu
	volumeRatiosBuffer = utils::CreateBufferFromData(device, &volumeRatios[0],
		sizeof(glm::vec3) * volumeTextures.size(), wgpu::BufferUsage::Storage);

	transferFunctionBufferColor = utils::CreateBufferFromData(device, &transferFunctionColor[0],
		sizeof(glm::vec4) * transferFunctionColor.size(), wgpu::BufferUsage::Storage);

	transferFunctionBufferRamp1 = utils::CreateBufferFromData(device, &transferFunctionRampLow[0],
		sizeof(float) * transferFunctionRampLow.size(), wgpu::BufferUsage::Storage);

	transferFunctionBufferRamp2 = utils::CreateBufferFromData(device, &transferFunctionRampHigh[0],
		sizeof(float) * transferFunctionRampHigh.size(), wgpu::BufferUsage::Storage);
}

#ifndef __EMSCRIPTEN__
void VolumeRenderer::composeImGui()
{
	if (!showImGui) {
		return;
	}

	// Start the Dear ImGui frame
	ImGui_ImplWGPU_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Transfer Functions");
	if (ImGui::BeginTabBar("MyTabBar"))
	{
		for (size_t i = 0; i < numVolumes; i++)
		{
			if (ImGui::BeginTabItem(volumeDescriptors[i].fileName.c_str()))
			{
					ImGui::SliderFloat("Ramp 1", &transferFunctionRampLow[i], 0.0f, 1.0f);
				ImGui::SliderFloat("Ramp 2", &transferFunctionRampHigh[i], 0.0f, 1.0f);
				ImGui::ColorEdit4("Color", (float*)&transferFunctionColor[i]);
				ImGui::EndTabItem();
			}
		}
		ImGui::EndTabBar();
	}
	ImGui::End();

	ImGui::Begin("Annotations Settings");
	ImGui::BeginGroup();
	ImGui::Checkbox("Enable", &enableAnnotations);
	const char* annotation_volumes[] = { "Volume A", "Volume B", "Volume C" };
	static const char* current_item_annotation = annotation_volumes[0];
	if (ImGui::BeginCombo("##combo1", current_item_annotation)) // The second parameter is the label previewed before opening the combo.
	{
		for (int n = 0; n < IM_ARRAYSIZE(annotation_volumes); n++)
		{
			bool is_selected = (current_item_annotation == annotation_volumes[n]); // You can store your selection however you want, outside or inside your objects
			if (ImGui::Selectable(annotation_volumes[n], is_selected))
			{
				current_item_annotation = annotation_volumes[n];
				annotationVolume = n;
				if (is_selected) {
					ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
				}
			}
		}
		ImGui::EndCombo();
	}
	
	if (ImGui::Button("Serialize Annotations..."))
	{
		annotationHandler->serialize("volume_annoations.json");
	}
	ImGui::SliderInt("Kernel Size", &annotationKernelSize, 1, 50);
	ImGui::Text("Segmentation Mask Update");
	ImGui::SliderInt("Which Mask", &whichMask, 0, 2);
	static char maskPath[128] = "data/320x320x448/ts_16_ilastik_predictions-Spikes-crop.raw";
	ImGui::InputText("Mask path", maskPath, IM_ARRAYSIZE(maskPath));
	if (ImGui::Button("Update Segmentation..."))
	{
		std::string maskPathStr(maskPath);
		updateSegmentationMask(whichMask, maskPathStr);
	}
	ImGui::EndGroup();
	ImGui::End();

	ImGui::Begin("Clipping Plane");
	ImGui::BeginGroup();
	ImGui::Text("Clipping Plane");
	ImGui::Checkbox("Enable", &enableClipping);
	ImGui::Checkbox("Toggle Fullscreen", &enableFullscreenAnnotations);
	ImGui::Checkbox("Clip Volume A", &clipMaskA);
	ImGui::Checkbox("Clip Volume B", &clipMaskB);
	ImGui::Checkbox("Clip Volume C", &clipMaskC);
	ImGui::SliderFloat("Clipping Plane Offset", &clippingPlaneOffset, -1.0f, 1.0f);
	const char* items[] = { "x Axis", "y Axis", "z Axis", "view aligned" };
	static const char* current_item = items[0];
	if (ImGui::BeginCombo("##combo2", current_item)) // The second parameter is the label previewed before opening the combo.
	{
		for (int n = 0; n < IM_ARRAYSIZE(items); n++)
		{
			bool is_selected = (current_item == items[n]); // You can store your selection however you want, outside or inside your objects
			if (ImGui::Selectable(items[n], is_selected))
			{
				current_item = items[n];
				clippingAxis = n;
				clippingViewAligned = (n == 3);
				if (is_selected) {
					ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
				}
			}
		}
		ImGui::EndCombo();
	}
	ImGui::EndGroup();
	ImGui::End();

	ImGui::Begin("Render Settings");
	ImGui::BeginGroup();
	ImGui::Text("Camera");
	ImGui::SliderFloat("Near Plane", &nearPlane, 0.01f, 1.0f);
	ImGui::SliderFloat("Far Plane", &farPlane, 1.0f, 20.0f);
	ImGui::ColorEdit3("Clear color", (float*)&clear_color);		// Edit 3 floats representing a color
	ImGui::Text("Raycasting Settings");
	ImGui::SliderFloat("Sample Rate", &sampleRate, 0.1f, 10.0f);		// Edit 3 floats representing a color
	ImGui::Checkbox("Early Ray Termination", &enableEarlyRayTermination);
	ImGui::Checkbox("Jittering", &enableJittering);
	ImGui::Text("Volumes Enabled");
	ImGui::Checkbox("enableVolumeA", &enableVolumeA);
	ImGui::Checkbox("enableVolumeB", &enableVolumeB);
	ImGui::Checkbox("enableVolumeC", &enableVolumeC);
	ImGui::Text("Effects");
	ImGui::Checkbox("Ambient Occlusion", &enableAmbientOcclusion);
	ImGui::Checkbox("Soft Shadows", &enableSoftShadows);
	ImGui::Text("Postprocessing");
	ImGui::Checkbox("Bloom", &enablePostProcessing);

	//ImGui::Checkbox("enableVolumeD", &enableVolumeD);
	ImGui::EndGroup();
	ImGui::BeginGroup();

	ImGui::Text("Ambient Occlusion");
	ImGui::SliderFloat("AO Radius", &aoRadius, 0.001f, 2.0f);
	ImGui::SliderInt("AO Num Samples", &aoNumSamples, 1, 40);
	ImGui::SliderFloat("AO Strength", &aoStrength, 0.0f, 1.0f);
	ImGui::Text("Soft Shadows");
	ImGui::SliderFloat("Shadow Quality", &shadowQuality, 0.1f, 5.0f);
	ImGui::SliderFloat("Shadow Strength", &shadowStrength, 0.0f, 1.0f);
	ImGui::SliderFloat("Shadow Radius", &shadowRadius, 0.01f, 1.0f);

	ImGui::EndGroup();
	ImGui::BeginGroup();
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::EndGroup();
	ImGui::End();

	ImGui::Begin("Volume Renderer");
	ImGui::BeginGroup();
	ImGui::Text("Volume");
	//static char volumePath[128] = "data/volumes/config.json";
	static char volumePath[128] = "data/320x320x448/config.json";
	//static char volumePath[128] = "data/nv/ts_16.json";
	//static char volumePath[128] = "data/cropped-volumes/1024x1024/ts_16/raw/config.json";

	ImGui::InputText("Volume path", volumePath, IM_ARRAYSIZE(volumePath));
	if (ImGui::Button("Open Volume by slice..."))
	{
		std::string volumePathStr(volumePath);
		openVolume(volumePathStr);
	}
	
	if (ImGui::Button("Open Volume..."))
	{
		std::string volumePathStr(volumePath);
		openVolume(volumePathStr, false);
	}
	ImGui::Text("Session");
	static char sessionPath[128] = "work/session.json";
	if (ImGui::Button("Load Session..."))
	{
		std::string sessionPathStr(sessionPath);
		loadSession(sessionPathStr);
	}
	if (ImGui::Button("Save Session..."))
	{
		std::string sessionPathStr(sessionPath);
		saveSession(sessionPathStr);
	}
	
	ImGui::Text("Hotkeys");
	static char hotkeysPath[128] = "work/hotkeys.json";
	ImGui::InputText("Hotkeys path", hotkeysPath, IM_ARRAYSIZE(hotkeysPath));
	if (ImGui::Button("Load Hotkeys..."))
	{
		Hotkeys::loadConfig(hotkeysPath);
	}
	ImGui::EndGroup();
	ImGui::End();
}
#endif

void VolumeRenderer::setupShaders()
{
	effectShaderModules.clear();

	std::string shaderPath("work/shader/");

	std::string vsFile(shaderPath + "volume.vs");
	std::string fsFile(shaderPath + "volume.fs");

	volumeShaderModules = ShaderUtil::loadAndCompileShader(device, vsFile, fsFile);

	std::vector<std::string> vsEffectShaderFiles;
	std::vector<std::string> fsEffectShaderFiles;

	std::string effectPath("work/config/effects.json");
	std::cout << "Load effects: " << effectPath << std::endl;
	std::ifstream configFile(effectPath);

	if (configFile.is_open())
	{
		json config;
		try
		{
			config = json::parse(configFile);
		}
		catch (json::parse_error& e)
		{
			// output exception information
			std::cout << "message: " << e.what() << '\n'
				<< "exception id: " << e.id << '\n'
				<< "byte position of error: " << e.byte << std::endl;
		}

		try
		{
			for (size_t i = 0; i < config["effects"].size(); i++)
			{
				std::string effectName = config["effects"][i]["name"];
				std::string vsFile = config["effects"][i]["vs"];
				std::string fsFile = config["effects"][i]["fs"];

				std::cout << "Load effect " << effectName << ": " << vsFile << ", " << fsFile << std::endl;

				vsEffectShaderFiles.push_back(shaderPath + vsFile);
				fsEffectShaderFiles.push_back(shaderPath + fsFile);
			}
		}
		catch (json::exception &e)
		{
			std::cout << e.what() << '\n';
		}		
	}

	for (size_t i = 0; i < vsEffectShaderFiles.size(); i++)
	{
		effectShaderModules.push_back(ShaderUtil::loadAndCompileShader(device, vsEffectShaderFiles[i], fsEffectShaderFiles[i]));
	}
}

void VolumeRenderer::createVolumeResources()
{
	std::cout << "VolumeRenderer::createResources()" << std::endl;
	// create the buffers (x, y, z, txc0, txc1)
	float const vertData[] =
	{
		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f, // 
		 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, // 
		-1.0f,  1.0f, 0.0f, 1.0f, 0.0f, //
		1.0f,  1.0f, 0.0f, 1.0f, 1.0f, // 
	};
	uint32_t const indexData[] = {0, 1, 2, 1, 3, 2};
	vertexBuffer = utils::CreateBufferFromData(device, vertData, sizeof(vertData), wgpu::BufferUsage::Vertex);
	indexBuffer = utils::CreateBufferFromData(device, indexData, sizeof(indexData), wgpu::BufferUsage::Index);

	// create uniform buffer
	cameraBuffer = utils::CreateBuffer(device, sizeof(CameraData), wgpu::BufferUsage::Uniform);
	paramBuffer = utils::CreateBuffer(device, sizeof(ParamData), wgpu::BufferUsage::Uniform);

	paramData.interaction = 0.0f;

	glm::mat4 transform(1.0);
	transformBuffer[0] = utils::CreateBufferFromData(device, &transform, sizeof(glm::mat4),
		wgpu::BufferUsage::Uniform);


	// ====================== VOLUME SAMPLER ======================
	{
		wgpu::SamplerDescriptor sampler_desc = {};
		sampler_desc.minFilter = wgpu::FilterMode::Linear;
		sampler_desc.magFilter = wgpu::FilterMode::Linear;
		sampler_desc.mipmapFilter = wgpu::FilterMode::Linear;
		sampler_desc.addressModeU = wgpu::AddressMode::ClampToEdge;
		sampler_desc.addressModeV = wgpu::AddressMode::ClampToEdge;
		sampler_desc.addressModeW = wgpu::AddressMode::ClampToEdge;
#if !defined(__EMSCRIPTEN__) 
		sampler_desc.maxAnisotropy = 1;
#endif
		volumeTextureSampler = device.CreateSampler(&sampler_desc);
	}

	// ====================== TEXTURE SAMPLER ======================
	{
		wgpu::SamplerDescriptor sampler_desc = {};
		sampler_desc.minFilter = wgpu::FilterMode::Linear;
		sampler_desc.magFilter = wgpu::FilterMode::Linear;
		sampler_desc.mipmapFilter = wgpu::FilterMode::Linear;
		sampler_desc.addressModeU = wgpu::AddressMode::ClampToEdge;
		sampler_desc.addressModeV = wgpu::AddressMode::ClampToEdge;
		sampler_desc.addressModeW = wgpu::AddressMode::ClampToEdge;
#if !defined(__EMSCRIPTEN__) 
		sampler_desc.maxAnisotropy = 1;
#endif
		effectTextureSampler = device.CreateSampler(&sampler_desc);
	}

	volumeTextureViews.clear();
	for (size_t i = 0; i < volumeTextures.size(); i++)
	{
		//volumeTextureViews.push_back(createTextureResource(i, i == 0));
		volumeTextureViews.push_back(createTextureView(i, true));
	}
}

void VolumeRenderer::createAnnotationResources()
{
	wgpu::TextureViewDescriptor tex_view_desc = {};
	tex_view_desc.format = wgpu::TextureFormat::RGBA8Unorm;
	tex_view_desc.dimension = wgpu::TextureViewDimension::e3D;
	tex_view_desc.baseMipLevel = 0;
	tex_view_desc.mipLevelCount = 1;
	tex_view_desc.baseArrayLayer = 0;
	tex_view_desc.arrayLayerCount = 1;
	tex_view_desc.aspect = wgpu::TextureAspect::All;

	annotationTexturePingView = annotationTexturePing.CreateView(&tex_view_desc);
	annotationTexturePongView = annotationTexturePong.CreateView(&tex_view_desc);
}

wgpu::TextureView VolumeRenderer::createTextureView(int source, bool rgba)
{
	// ====================== VOLUME VIEW ======================
	{
		wgpu::TextureViewDescriptor tex_view_desc = {};
		tex_view_desc.format = rgba ? wgpu::TextureFormat::RGBA8Unorm : wgpu::TextureFormat::R8Unorm;
		tex_view_desc.dimension = wgpu::TextureViewDimension::e3D;
		tex_view_desc.baseMipLevel = 0;
		tex_view_desc.mipLevelCount = 1;
		tex_view_desc.baseArrayLayer = 0;
		tex_view_desc.arrayLayerCount = 1;
		tex_view_desc.aspect = wgpu::TextureAspect::All;

		return volumeTextures[source].CreateView(&tex_view_desc);
	}
}
	
void VolumeRenderer::createEffectResources()
{
	std::cout << "Create effect resources..." << std::endl;
	// ====================== EFFECT RENDER TEXTURES ======================
	int numEffects = effectShaderModules.size();
	for (size_t i = 0; i < numEffects; i++)
	{
		wgpu::TextureDescriptor descriptor;
		descriptor.dimension = wgpu::TextureDimension::e2D;
		descriptor.size.width = width;
		descriptor.size.height = height;
		descriptor.size.depthOrArrayLayers = 1;
		descriptor.sampleCount = 1;
		//descriptor.format = wgpu::TextureFormat::RGBA8Unorm;
		descriptor.format = preferredSwapChainTextureFormat;
		descriptor.mipLevelCount = 1;
		descriptor.usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::RenderAttachment;

		wgpu::Texture texture = device.CreateTexture(&descriptor);

		wgpu::TextureViewDescriptor tex_view_desc = {};
		//tex_view_desc.format = wgpu::TextureFormat::RGBA8Unorm;
		tex_view_desc.label = "effects";
		tex_view_desc.format = preferredSwapChainTextureFormat;
		tex_view_desc.dimension = wgpu::TextureViewDimension::e2D;
		tex_view_desc.baseMipLevel = 0;
		tex_view_desc.mipLevelCount = 1;
		tex_view_desc.baseArrayLayer = 0;
		tex_view_desc.arrayLayerCount = 1;
		tex_view_desc.aspect = wgpu::TextureAspect::All;
		effectTextureViews.push_back(texture.CreateView(&tex_view_desc));	
	}

	for (size_t i = 0; i < numEffects; i++)
	{
		effectParamBuffers.push_back(utils::CreateBuffer(device, sizeof(EffectData), wgpu::BufferUsage::Uniform));
	}
}

wgpu::RenderPipeline VolumeRenderer::createRenderPipeline(std::string label, wgpu::ShaderModule vs, wgpu::ShaderModule fs, wgpu::PipelineLayout pipelineLayout,
	wgpu::VertexBufferLayout vertexBufferLayout, wgpu::TextureFormat textureFormat, bool enableDepthStencil)
{
	// ====================== PIPELINE CREATION ======================
	{
		utils::ComboRenderPipelineDescriptor descriptor;
		descriptor.label = label.c_str();
		descriptor.layout = pipelineLayout;

		// describe vertex state
		descriptor.vertex.module = vs;
		descriptor.vertex.bufferCount = 1;
		descriptor.vertex.buffers = &vertexBufferLayout;
		// describe fragment state
		descriptor.cFragment.module = fs;
		descriptor.cTargets[0].format = textureFormat;

		// Note: Using a stencil buffer will cause a bug in Dear imGui, so that the attachment states won't fit anymore
		// describe depth stencil state
		//wgpu::DepthStencilState* depthStencil = descriptor.EnableDepthStencil(wgpu::TextureFormat::Depth24PlusStencil8);
		if (enableDepthStencil)
		{
			
			wgpu::DepthStencilState* depthStencil = nullptr;
			if (enableStencil) {
				depthStencil = descriptor.EnableDepthStencil(wgpu::TextureFormat::Depth24PlusStencil8);
				std::cout << "create render pipiline with depthPlusStencil attachment" << std::endl;
			}
			else {
				depthStencil = descriptor.EnableDepthStencil(wgpu::TextureFormat::Depth32Float);	
				std::cout << "create render pipiline with depth attachment" << std::endl;
			}
			 	
			depthStencil->depthWriteEnabled = true;
			depthStencil->depthCompare = wgpu::CompareFunction::Less;
		}
		else {
			std::cout << "create render pipiline with without depthStencil attachment" << std::endl;
		}
		
		return device.CreateRenderPipeline(&descriptor);
	}
}

wgpu::RenderPipeline VolumeRenderer::setupVolumePipelineLayout()
{
	// ====================== BIND LAYOUT ======================

	// bind group layout (used by both the pipeline layout and uniform bind group)
	// the bind group layout and bind groups using the layout are treated as separate objects, 
	// allowing parameter values to be changed without changing the entire rendering pipeline
	// By using multiple bind group sets, we can swap out per - object parameters 
	// without conflicting with bind groups specifying global parameters during rendering.
	auto bindGroupLayout = utils::MakeBindGroupLayout(
		device, {
			{0, wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment, wgpu::BufferBindingType::Uniform},
			{1, wgpu::ShaderStage::Vertex, wgpu::BufferBindingType::Uniform},
			{2, wgpu::ShaderStage::Fragment, wgpu::SamplerBindingType::Filtering},
			{3, wgpu::ShaderStage::Fragment, wgpu::TextureSampleType::Float, wgpu::TextureViewDimension::e3D},
			{4, wgpu::ShaderStage::Fragment, wgpu::TextureSampleType::Float, wgpu::TextureViewDimension::e3D},
			{5, wgpu::ShaderStage::Fragment, wgpu::TextureSampleType::Float, wgpu::TextureViewDimension::e3D},
			{6, wgpu::ShaderStage::Fragment, wgpu::TextureSampleType::Float, wgpu::TextureViewDimension::e3D},
			{7, wgpu::ShaderStage::Fragment, wgpu::TextureSampleType::Float, wgpu::TextureViewDimension::e3D},
			{8, wgpu::ShaderStage::Fragment, wgpu::BufferBindingType::Uniform},
			{9, wgpu::ShaderStage::Fragment, wgpu::BufferBindingType::ReadOnlyStorage},
			{10, wgpu::ShaderStage::Fragment, wgpu::BufferBindingType::ReadOnlyStorage},
			{11, wgpu::ShaderStage::Fragment, wgpu::BufferBindingType::ReadOnlyStorage},
			{12, wgpu::ShaderStage::Fragment, wgpu::BufferBindingType::ReadOnlyStorage}
		});

	// pipeline layout (used by the render pipeline, released after its creation)
	wgpu::PipelineLayout pipelineLayout = utils::MakeBasicPipelineLayout(device, &bindGroupLayout);

	// ====================== BIND GROUP ======================

	// at a high level, bind groups follow a similar model to vertex buffers in WebGPU
	// Each bind group specifies an array of buffersand textures which it contains, 
	// and the parameter binding indices to map these too in the shader.
	volumeBindGroup = utils::MakeBindGroup(
		device, bindGroupLayout, {
			{0, cameraBuffer, 0, sizeof(CameraData)},
			{1, transformBuffer[0], 0, sizeof(glm::mat4)},
			{2, volumeTextureSampler},
			{3, volumeTextureViews[0]},
			{4, volumeTextureViews[1]},
			{5, annotationTexturePingView},
			{6, annotationTexturePongView},
			{7, volumeTextureViews[1]},  //TODO: ugly workaround
			{8, paramBuffer, 0, sizeof(ParamData)},
			{9, transferFunctionBufferColor, 0, sizeof(glm::vec4) * transferFunctionColor.size()},
			{10, transferFunctionBufferRamp1, 0, sizeof(float) * transferFunctionRampLow.size()},
			{11, transferFunctionBufferRamp2, 0, sizeof(float) * transferFunctionRampHigh.size()},
			{12, volumeRatiosBuffer, 0, sizeof(glm::vec3) * volumeTextures.size()}
		});


	// ====================== DEPTH STENCIL TEXTURE ======================
	{
		wgpu::TextureDescriptor descriptor;
		descriptor.dimension = wgpu::TextureDimension::e2D;
		descriptor.size.width = width;
		descriptor.size.height = height;
		descriptor.size.depthOrArrayLayers = 1;
		descriptor.sampleCount = 1;
		if (enableStencil) {
			descriptor.format = wgpu::TextureFormat::Depth24PlusStencil8;
		}
		else {
			descriptor.format = wgpu::TextureFormat::Depth32Float;
		}	
		descriptor.mipLevelCount = 1;
		descriptor.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding;
		depthStencilTexture = device.CreateTexture(&descriptor);
		depthStencilTextureView = depthStencilTexture.CreateView();
	}
	

	// ====================== VERTEX BUFFER LAYOUT ======================
	wgpu::VertexAttribute attributes[2];
	attributes[0].shaderLocation = 0;
	attributes[0].offset = 0;
	attributes[0].format = wgpu::VertexFormat::Float32x3;
	attributes[1].shaderLocation = 1;
	attributes[1].offset = 3 * sizeof(float);
	attributes[1].format = wgpu::VertexFormat::Float32x2;

	wgpu::VertexBufferLayout vertexBufferLayout;
	vertexBufferLayout.attributeCount = 2;
	vertexBufferLayout.attributes = attributes;
	vertexBufferLayout.arrayStride = 5 * sizeof(float);

	wgpu::RenderPipeline pipeline = createRenderPipeline("Volume Render Pipeline Descriptor", volumeShaderModules[0], volumeShaderModules[1], 
										pipelineLayout, vertexBufferLayout, preferredSwapChainTextureFormat, enableDepthAttachment);
	pipeline.SetLabel("Volume Render Pipeline");

	return pipeline;

	// cleanup
	//pipelineLayout.Release();

	//fsModule.Release();
	//vsModule.Release();
	//bindGroupLayout.Release();	
}

wgpu::RenderPipeline VolumeRenderer::setupEffectPipelineLayout(wgpu::ShaderModule vsModule, wgpu::ShaderModule fsModule, 
	wgpu::TextureView inputTextureView, wgpu::TextureView inputTexture2View, wgpu::Buffer paramBuffer)
{
	// ====================== BIND LAYOUT ======================

	// bind group layout (used by both the pipeline layout and uniform bind group)
	// the bind group layout and bind groups using the layout are treated as separate objects, 
	// allowing parameter values to be changed without changing the entire rendering pipeline
	// By using multiple bind group sets, we can swap out per - object parameters 
	// without conflicting with bind groups specifying global parameters during rendering.
	
	auto bindGroupLayout = utils::MakeBindGroupLayout(
		device, {
			{0, wgpu::ShaderStage::Fragment, wgpu::SamplerBindingType::Filtering},
			{1, wgpu::ShaderStage::Fragment, wgpu::TextureSampleType::Float, wgpu::TextureViewDimension::e2D},
			{2, wgpu::ShaderStage::Fragment, wgpu::TextureSampleType::Float, wgpu::TextureViewDimension::e2D},
			{3, wgpu::ShaderStage::Fragment, wgpu::BufferBindingType::Uniform},
		});

	// ====================== BIND GROUP ======================

	// at a high level, bind groups follow a similar model to vertex buffers in WebGPU
	// Each bind group specifies an array of buffersand textures which it contains, 
	// and the parameter binding indices to map these too in the shader.
	auto bindGroup = utils::MakeBindGroup(
		device, bindGroupLayout, {
			{0, effectTextureSampler},
			{1, inputTextureView},
			{2, inputTexture2View},
			{3, paramBuffer, 0, sizeof(EffectData) },
		});
	effectsBindGroup.push_back(bindGroup);

	wgpu::PipelineLayout pipelineLayout = utils::MakeBasicPipelineLayout(device, &bindGroupLayout);


	// ====================== VERTEX BUFFER LAYOUT ======================
	wgpu::VertexAttribute attributes[2];
	attributes[0].shaderLocation = 0;
	attributes[0].offset = 0;
	attributes[0].format = wgpu::VertexFormat::Float32x3;
	attributes[1].shaderLocation = 1;
	attributes[1].offset = 3 * sizeof(float);
	attributes[1].format = wgpu::VertexFormat::Float32x2;

	wgpu::VertexBufferLayout vertexBufferLayout;

	vertexBufferLayout.attributeCount = 2;
	vertexBufferLayout.attributes = attributes;
	vertexBufferLayout.arrayStride = 5 * sizeof(float);

	wgpu::RenderPipeline pipeline = createRenderPipeline("Effect Pipeline", vsModule, fsModule, pipelineLayout, vertexBufferLayout, preferredSwapChainTextureFormat);
	return pipeline;

	// cleanup
	//pipelineLayout.Release();

	//fsModule.Release();
	//vsModule.Release();
	//bindGroupLayout.Release();	
}

void VolumeRenderer::saveSession(std::string &file_name)
{
	json session;
	session["enable_volume_a"] = enableVolumeA;
	session["enable_volume_b"] = enableVolumeB;
	session["enable_volume_c"] = enableVolumeC;
	session["sample_rate"] = sampleRate;
	session["early_ray_termination"] = enableEarlyRayTermination;
	session["ray_jittering"] = enableJittering;
	session["ambient_occlusion"] = enableAmbientOcclusion;
	session["ao_radius"] = aoRadius;
	session["ao_samples"] = aoNumSamples;
	session["ao_strength"] = aoStrength;
	session["shadows"] = enableSoftShadows;
	session["shadow_quality"] = shadowQuality;
	session["shadow_strength"] = shadowStrength;
	session["shadow_radius"] = shadowRadius;
	session["bloom"] = enablePostProcessing;
	session["annotations"] = enableAnnotations;
	session["annotation_kernel_size"] = annotationKernelSize;
	session["clipping"] = enableClipping;
	session["clip_volume_a"] = clipMaskA;
	session["clip_volume_b"] = clipMaskB;
	session["clip_volume_c"] = clipMaskC;
	session["clip_offset"] = clippingPlaneOffset;
	session["clip_plane"] = clippingAxis;
	if (clippingViewAligned) {
		session["clip_plane"] = 3;
	}
	session["near_plane"] = nearPlane;
	session["far_plane"] = farPlane;
	json clearColorJson;
	clearColorJson["x"] = int(clear_color.x * 255.0f);
	clearColorJson["y"] = int(clear_color.y * 255.0f);
	clearColorJson["z"] = int(clear_color.z * 255.0f);

	session["clear_color"] = clearColorJson;

	std::ofstream file(file_name);
	file << std::setw(2) << session << std::endl;
	file.close();

}

void VolumeRenderer::loadSession(std::string file)
{
	std::ifstream sessionFile(file);
	if (sessionFile.is_open())
	{
		try
		{
			json session;
			sessionFile >> session;

			enableVolumeA = session["enable_volume_a"];
			enableVolumeB = session["enable_volume_b"];
			enableVolumeC = session["enable_volume_c"];
			sampleRate = session["sample_rate"];
			enableEarlyRayTermination = session["early_ray_termination"];
			enableJittering = session["ray_jittering"];
			enableAmbientOcclusion = session["ambient_occlusion"];
			aoRadius = session["ao_radius"];
			aoNumSamples = session["ao_samples"];
			aoStrength = session["ao_strength"];
			enableSoftShadows = session["shadows"];
			shadowQuality = session["shadow_quality"];
			shadowStrength = session["shadow_strength"];
			shadowRadius = session["shadow_radius"];
			enablePostProcessing = session["bloom"];
			enableAnnotations = session["annotations"];
			annotationKernelSize = session["annotation_kernel_size"];
			enableClipping = session["clipping"];
			clipMaskA = session["clip_volume_a"];
			clipMaskB = session["clip_volume_b"];
			clipMaskC = session["clip_volume_c"];
			clippingPlaneOffset = session["clip_offset"];
			clippingAxis = session["clip_plane"];
			clippingViewAligned = (clippingAxis == 3);
			nearPlane = session["near_plane"];
			farPlane = session["far_plane"];

			clear_color.x = session["clear_color"]["x"];
			clear_color.x /= 255.0f;
			clear_color.y = session["clear_color"]["y"];
			clear_color.y /= 255.0f;
			clear_color.z = session["clear_color"]["z"];
			clear_color.z /= 255.0f;
		}
		catch (json::exception& e)
		{
			std::cout << e.what() << '\n';
			return ;
		}
	}
	else
	{
		std::cout << "Unable to open session file: " << file << std::endl;
		return;
	}
}

void VolumeRenderer::setClippingPlaneModus(int modus)
{
	switch (modus)
	{
	case 0:
		enableClipping = false;
		enableAnnotations = enableClipping;
		break;
	case 1:
		clippingViewAligned = true;
		enableClipping = true;
		enableAnnotations = enableClipping;
		break;
	case 2:
		clippingAxis = 0;
		clippingViewAligned = false;
		enableClipping = true;
		enableAnnotations = enableClipping;
		break;
	case 3:
		clippingAxis = 1;
		clippingViewAligned = false;
		enableClipping = true;
		enableAnnotations = enableClipping;
		break;
	case 4:
		clippingAxis = 2;
		clippingViewAligned = false;
		enableClipping = true;
		enableAnnotations = enableClipping;
		break;
	default:
		break;
	}
}

void VolumeRenderer::setTransferFunction(int which, float lowRamp, float highRamp, glm::vec3 color)
{
	if (which < transferFunctionRampLow.size())
	{
		transferFunctionRampLow[which] = lowRamp;
		transferFunctionRampHigh[which] = highRamp;

		transferFunctionColor[which] = glm::vec4(color.x / 255.0f, color.y / 255.0f, color.z / 255.0f, 1.0);
	}
}

#ifndef __EMSCRIPTEN__
void VolumeRenderer::renderImGUI(const wgpu::TextureView& renderTarget)
{
	if (!showImGui) {
		return;
	}
	auto encoderImGui = device.CreateCommandEncoder();
	utils::ComboRenderPassDescriptor renderPassImGui({ renderTarget });
	renderPassImGui.cColorAttachments[0].loadOp = wgpu::LoadOp::Load;
	wgpu::RenderPassEncoder passImGui = encoderImGui.BeginRenderPass(&renderPassImGui);

	ImGui::Render();
	ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), passImGui.Get());
	
	passImGui.End();
	passImGui.Release();
	wgpu::CommandBuffer commandsImGui = encoderImGui.Finish();
	
	auto queue = device.GetQueue();
	queue.Submit(1, &commandsImGui);

	encoderImGui.Release();
	commandsImGui.Release();
}
#endif

void VolumeRenderer::renderPostprocessing(double timeElapsed, const wgpu::TextureView& renderTarget, wgpu::CommandEncoder* encoder)
{
	if (enablePostProcessing)
	{
		auto queue = device.GetQueue();
		for (size_t i = 0; i < effectPipelines.size(); i++)
		{
			EffectData effectData;
			effectData.param0 = glm::vec4(width, height, interactionMode, 0);
			effectData.param1 = glm::vec4(1, 0, 0, 1);
			effectData.param2 = glm::vec4(0, 1, 0, 1);
			effectData.param3 = glm::vec4(0, 0, 1, 1);
			effectData.param4 = glm::vec4(0, 0, 0, 1);

			queue.WriteBuffer(effectParamBuffers[i], 0, &effectData, sizeof(EffectData));
		}

		for (size_t i = 0; i < effectPipelines.size(); i++)
		{
			utils::ComboRenderPassDescriptor renderPassDescriptor({ renderTarget });
			renderPassDescriptor.label = "Effect Render Pass";

			if (i == effectPipelines.size() - 1) {
				renderPassDescriptor = utils::ComboRenderPassDescriptor({ renderTarget });
			}
			else {
				renderPassDescriptor = utils::ComboRenderPassDescriptor({ effectTextureViews[i + 1] });
			}

			wgpu::Color clearColor = { 0.0, 0.0, 0.0, 1.0 };
			//renderPassDescriptor.cColorAttachments[0].clearColor = clearColor;
			renderPassDescriptor.cColorAttachments[0].clearValue = clearColor;
			renderPassDescriptor.cDepthStencilAttachmentInfo.stencilStoreOp = wgpu::StoreOp::Undefined;
			renderPassDescriptor.cDepthStencilAttachmentInfo.stencilLoadOp = wgpu::LoadOp::Undefined;

			wgpu::RenderPassEncoder pass = encoder->BeginRenderPass(&renderPassDescriptor);
			if (initialized && enabled)
			{
				pass.SetPipeline(effectPipelines[i]);
				pass.SetBindGroup(0, effectsBindGroup[i]);
				pass.SetVertexBuffer(0, vertexBuffer);
				pass.SetIndexBuffer(indexBuffer, wgpu::IndexFormat::Uint32);
				pass.DrawIndexed(6);
			}

			pass.End();
			pass.Release();
		}
	}
}

void VolumeRenderer::renderVolumes(double timeElapsed, const wgpu::TextureView& renderTarget, wgpu::CommandEncoder *encoder)
{
	if (!initialized || !enabled) {
		return;
	}
	//std::cout << "renderVolumes()" << std::endl;
	//modelMatrix = glm::mat3(1.0);
	cameraData.view = camera->getViewMatrix() * modelMatrix;
	cameraData.viewInv = glm::inverse(cameraData.view);
	cameraData.aspectRatio = camera->getAspectRatio();
	cameraData.fov = camera->getFov();
	cameraData.proj = camera->getProjectionMatrix();

	auto queue = device.GetQueue();
	queue.WriteBuffer(cameraBuffer, 0, &cameraData, sizeof(CameraData));

	//std::cout << glm::to_string(cameraData.view) << std::endl;

	paramData.enableEarlyRayTermination = enableEarlyRayTermination;
	paramData.enableJittering = enableJittering;
	
	paramData.enableSoftShadows = enableSoftShadows;
	if (interactionMode) {
		paramData.interaction = 1.0f;
		//paramData.sampleRate = sampleRate / 2.0f;
		//paramData.enableAmbientOcclusion = false;
		//paramData.sampleRate = sampleRate / 2.0f;
		//paramData.aoNumSamples = 1;
		//paramData.shadowQuality = 0.5;
	}
	else 
	{
		paramData.interaction = 0.0f;
		//paramData.sampleRate = sampleRate;
		//paramData.enableAmbientOcclusion = enableAmbientOcclusion;
		//paramData.shadowQuality = shadowQuality;
		//paramData.aoNumSamples = aoNumSamples;
	}
	
	paramData.sampleRate = sampleRate;
	paramData.enableAmbientOcclusion = enableAmbientOcclusion;
	paramData.shadowQuality = shadowQuality;
	paramData.aoNumSamples = aoNumSamples;
	paramData.aoRadius = aoRadius;
	paramData.aoStrength = aoStrength;
	paramData.shadowStrength = shadowStrength;
	paramData.voxelSize = voxelSize;

	if (enableClipping) {
		// invert values since we use the vector as a bitmask
		paramData.clippingMask = glm::vec4(!clipMaskA, !clipMaskB, !clipMaskC, 1.0f);
	}
	else {
		paramData.clippingMask = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	}
	
	paramData.enableVolumeA = enableVolumeA;
	paramData.enableVolumeB = enableVolumeB;
	paramData.enableVolumeC = enableVolumeC;
	paramData.enableVolumeD = enableVolumeD;

	paramData.viewVector = glm::vec4(camera->getViewVector(), 0.0f);
	paramData.enableAnnotations = enableAnnotations;
	paramData.annotationVolume = annotationVolume;
	paramData.annotationPingPong = annotationHandler->isPingPong();
	paramData.clippingPlaneOrigin = clippingPlaneOrigin;
	paramData.clippingPlaneNormal = clippingPlaneNormal;
	paramData.clearColor = glm::vec4(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
	paramData.shadowRadius = shadowRadius;

	
	//if (paramData.slice > 1.0f) {
	//	paramData.slice = 0.0f;
	//}

	queue.WriteBuffer(paramBuffer, 0, &paramData, sizeof(ParamData));

	queue.WriteBuffer(transferFunctionBufferColor, 0, &transferFunctionColor[0], sizeof(glm::vec4) * numVolumes);

	queue.WriteBuffer(transferFunctionBufferRamp1, 0, &transferFunctionRampLow[0], sizeof(float) * numVolumes);
	queue.WriteBuffer(transferFunctionBufferRamp2, 0, &transferFunctionRampHigh[0], sizeof(float) * numVolumes);

	//utils::ComboRenderPassDescriptor renderPassDescriptor({ renderTarget }, depthStencilView);
	wgpu::RenderPassDescriptor renderPassDescriptor;
	wgpu::RenderPassColorAttachment colorAttachment;

	if (!effectPipelines.empty() && enablePostProcessing) {
		colorAttachment.view = effectTextureViews[0];
		//renderPassDescriptor = utils::ComboRenderPassDescriptor({ effectTextureViews[0] }, depthStencilView);
	}
	else
	{
		colorAttachment.view = renderTarget;
	}

	if (enableDepthAttachment)
	{
		wgpu::RenderPassDepthStencilAttachment depthStencilAttachment;
		//depthStencilAttachment.clearDepth = 1.0f;
		depthStencilAttachment.depthClearValue = 1.0f; 
		depthStencilAttachment.view = depthStencilTextureView;
		depthStencilAttachment.depthLoadOp = wgpu::LoadOp::Clear;
		depthStencilAttachment.depthStoreOp = wgpu::StoreOp::Store;
		if (enableStencil) 
		{
			//depthStencilAttachment.clearStencil = 1;
			depthStencilAttachment.stencilClearValue = 1;
			depthStencilAttachment.stencilLoadOp = wgpu::LoadOp::Clear;
			depthStencilAttachment.stencilStoreOp = wgpu::StoreOp::Discard;
		}
		else 
		{
			depthStencilAttachment.stencilLoadOp = wgpu::LoadOp::Undefined;
			depthStencilAttachment.stencilStoreOp = wgpu::StoreOp::Undefined;
		}
		renderPassDescriptor.depthStencilAttachment = &depthStencilAttachment;
	}

	wgpu::Color clearColor = { 0.0, 0.0, 0.0, 1.0 };
	//renderPassDescriptor.cColorAttachments[0].clearValue = clearColor;

	colorAttachment.loadOp = wgpu::LoadOp::Clear;
	colorAttachment.storeOp = wgpu::StoreOp::Store;
	//olorAttachment.clearColor = clearColor;
	colorAttachment.clearValue = clearColor;
	renderPassDescriptor.colorAttachments = &colorAttachment;
	renderPassDescriptor.colorAttachmentCount = 1;
	renderPassDescriptor.label = "Volume Render Pass";

	auto pass = encoder->BeginRenderPass(&renderPassDescriptor);
	pass.SetPipeline(volumePipeline);
	pass.SetBindGroup(0, volumeBindGroup);
	pass.SetVertexBuffer(0, vertexBuffer);
	pass.SetIndexBuffer(indexBuffer, wgpu::IndexFormat::Uint32);
	pass.DrawIndexed(6);

	pass.End();
	pass.Release();
}

void VolumeRenderer::preRender(double timeElapsed)
{
	// only go through slices when not in annotation mode
	if (!Input::getKey(Hotkeys::getHotkey("annotation_mode")))
	{
		float distance = 0.0f;
		if (Hotkeys::getHotkey("slices_mouse") == GLFW_MOUSE_BUTTON_MIDDLE)
		{
			distance = Input::mouseScrollDelta().second * 0.007f;
		}
		else
		{
			if (Input::getMouseButton(Hotkeys::getHotkey("slices_mouse")))
			{
				auto mousePosTmp = Input::mousePosition();
				double deltaY = mousePos.second - mousePosTmp.second;
				distance = deltaY * 0.001;
			}
		}
		clippingPlaneOffset += distance;
		clippingPlaneOffset = std::max(std::min(clippingPlaneOffset, 1.0f), -1.0f);
	}

	mousePos = Input::mousePosition();

	glm::vec3 clippingPlaneUp;
	if (clippingViewAligned)
	{
		clippingPlaneNormal = glm::inverse(modelMatrix) * glm::vec4(camera->getViewVector(), 0.0f);
		glm::vec3 upVec = glm::inverse(modelMatrix) * glm::vec4(camera->getUpVec(), 0.0f);
		auto rightVec = glm::cross(glm::vec3(clippingPlaneNormal), upVec);
		if (glm::dot(glm::vec3(clippingPlaneNormal), rightVec) < 0.01f)
		{
			auto rightVec = glm::cross(glm::vec3(clippingPlaneNormal), glm::vec3(1, 0, 0));
		}
		clippingPlaneUp = glm::normalize(glm::cross(glm::vec3(clippingPlaneNormal), rightVec));
	}
	else
	{
		if (clippingAxis == 0)
		{
			clippingPlaneNormal = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
			clippingPlaneUp = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
		}
		if (clippingAxis == 1)
		{
			clippingPlaneNormal = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
			clippingPlaneUp = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
		}
		if (clippingAxis == 2)
		{
			clippingPlaneNormal = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
			clippingPlaneUp = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
		}
	}
	if (initialized) {
		clippingPlaneOrigin = glm::vec4(clippingPlaneOffset * volumeRatios[0] * glm::vec3(clippingPlaneNormal), 1.0f);
	}

	annotationHandler->clippingPlaneNormal = clippingPlaneNormal;
	annotationHandler->clippingPlaneOrigin = clippingPlaneOrigin;
	annotationHandler->setKernelSize(glm::vec3(annotationKernelSize));
	annotationHandler->annotationVolume = annotationVolume;

	clippingPlaneRenderer->enableVolumeA = enableVolumeA;
	clippingPlaneRenderer->enableVolumeB = enableVolumeB;
	clippingPlaneRenderer->enableVolumeC = enableVolumeC;
	clippingPlaneRenderer->enableVolumeD = enableVolumeD;

	clippingPlaneRenderer->annotationVolume = annotationVolume;

	clippingPlaneRenderer->setEnabled(enableClipping && enableDepthAttachment);
	clippingPlaneRenderer->clippingPlaneOrigin = clippingPlaneOrigin;
	clippingPlaneRenderer->clippingPlaneUp = clippingPlaneUp;
	clippingPlaneRenderer->clippingPlaneNormal = clippingPlaneNormal;
	if (enableFullscreenAnnotations && enableClipping) 
	{
		clippingPlaneRenderer->enableFullscreen();
		annotationHandler->volumeModelMatrix = glm::mat4(1.0f);
		clippingPlaneRenderer->volumeModelMatrix = glm::mat4(1.0f);
		annotationHandler->setCamera(clippingPlaneRenderer->camera_fullscreen);
		enabled = false;
	}
	else
	{
		annotationHandler->volumeModelMatrix = modelMatrix;
		clippingPlaneRenderer->volumeModelMatrix = modelMatrix;
		clippingPlaneRenderer->disableFullscreen();
		annotationHandler->setCamera(camera);
		enabled = true;
	}
	if (enableFullscreenAnnotations && !enableClipping) {
		enableFullscreenAnnotations = false;
	}

	camera->setNear(nearPlane);
	camera->setFar(farPlane);
}

void VolumeRenderer::render(double timeElapsed, const wgpu::TextureView &renderTarget)
{
	auto encoder = device.CreateCommandEncoder();
	
	preRender(timeElapsed);

	// ======================= VOLUME RENDER PASS =============================
	renderVolumes(timeElapsed, renderTarget, &encoder);
	
	// ======================= EFFECT RENDER PASS =============================
	renderPostprocessing(timeElapsed, renderTarget, &encoder);
	
	// submit commands to GPU
	wgpu::CommandBuffer commands = encoder.Finish();
	auto queue = device.GetQueue();
	queue.Submit(1, &commands);
	
	encoder.Release();
	commands.Release();	

	// ======================= ImGUI RENDER PASS =============================
#ifndef __EMSCRIPTEN__
	composeImGui();
	renderImGUI(renderTarget);
#endif
}