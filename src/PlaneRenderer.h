#pragma once

#include <webgpu/webgpu_cpp.h>
#include <array>
#include <glm/glm.hpp>

class Camera;

class PlaneRenderer
{
public:
	PlaneRenderer(wgpu::Device &device, wgpu::TextureFormat preferredSwapChainTextureFormat, Camera *camera);

	virtual ~PlaneRenderer();

	void render(const wgpu::TextureView &renderTarget);

	void resize(int width, int height);

	void init();

	void setVolumeTexture(wgpu::TextureView &texture);

	void enableFullscreen();

	void disableFullscreen();

	inline void setEnabled(bool enableClipping) { this->enabled = enableClipping; }

	glm::vec4 clippingPlaneOrigin;
	glm::vec4 clippingPlaneNormal;
	glm::vec3 clippingPlaneUp;
	glm::vec4 volumeSize;
	glm::vec3 volumeRatio;
	float paddingRatioX;
		
	uint64_t transferFunctionBufferRamp1Size;
	uint64_t transferFunctionBufferRamp2Size;
	uint64_t transferFunctionBufferColorSize;
	uint64_t volumeRatiosBufferSize;

	wgpu::Buffer transferFunctionBufferRamp1;
	wgpu::Buffer transferFunctionBufferRamp2;
	wgpu::Buffer transferFunctionBufferColor;
	wgpu::Buffer volumeRatiosBuffer;
	wgpu::TextureView depthTextureView;

	wgpu::TextureView annotationTextureView;

	Camera* camera_fullscreen = nullptr;

	glm::mat4 volumeModelMatrix = glm::mat4(1.0f);

	bool enableStencil = false;

	bool enableVolumeA = true;
	bool enableVolumeB = true;
	bool enableVolumeC = true;
	bool enableVolumeD = true;

	bool enableRawData = true;

	int annotationVolume = 0;


private:

	void preRender();

	bool enabled = false;
	float fullscreen_animation;
	bool fullscreen = false;
	wgpu::BlendState* blend;
	Camera *camera = nullptr;;
	std::array<wgpu::ShaderModule, 2> shaderModules;
	wgpu::Device device;
	wgpu::Buffer vertexBuffer;
	wgpu::Buffer indexBuffer;
	wgpu::Buffer paramBuffer;
	wgpu::Buffer cameraBuffer;
	wgpu::BindGroup bindGroup;
	wgpu::TextureFormat preferredSwapChainTextureFormat;
	wgpu::RenderPipeline pipeline;
	wgpu::TextureView volumeTextureView;
	
	wgpu::Sampler volumeTextureSampler;
	
	int width, height;

	bool isVolumeSet = false;
};

