#include "PlaneRenderer.h"

#include "Helper.h"
#include "ShaderUtil.h"
#include "dawn/utils/WGPUHelpers.h"
#include "dawn/utils/ComboRenderPipelineDescriptor.h"
#include "Camera.h"

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <iostream>

#define M_PI 3.14159265358979323846

struct ParamDataPlane
{
	glm::vec4 param0;
	glm::vec4 enableVolumes;
	glm::vec4 clippingPlaneOrigin;
	glm::vec4 clippingPlaneNormal;
	glm::vec4 volumeSize;
	glm::vec4 screenSize;
} paramDataPlane;

struct CameraDataPlane
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 viewInv;
	glm::mat4 proj;
	float aspectRatio;
	float fov;
} cameraDataPlane;

PlaneRenderer::PlaneRenderer(wgpu::Device &device, wgpu::TextureFormat preferredSwapChainTextureFormat, Camera *camera)
{
	this->camera = camera;
	this->device = device;
	this->preferredSwapChainTextureFormat = preferredSwapChainTextureFormat;


	camera_fullscreen = new Camera(glm::vec3(0, 0, 1.0f), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
}

PlaneRenderer::~PlaneRenderer()
{
}

void PlaneRenderer::resize(int width, int height)
{
	this->width = width;
	this->height = height;
}

void PlaneRenderer::preRender()
{
	auto queue = device.GetQueue();
	
	glm::mat4 model(1.0f);
	model = glm::translate(model, glm::vec3(clippingPlaneOrigin));

	glm::vec3 from = glm::normalize(glm::vec3(0.0f, 0.0f, 1.0f));
	glm::vec3 to = clippingPlaneNormal;
	if (abs(dot(from, to)) < 0.9999f)
	{
		glm::vec3 v = glm::cross(from, to);
		v = glm::normalize(v);
		float angle = acos(glm::dot(to, from) / (glm::length(to) * glm::length(from)));
		model = glm::rotate(model, angle, v);
	}

	// if fullscreen is enabled, we interpolate from the original to the fullscreen camera
	if (fullscreen)
	{
		// set fullscreen camera parameters
		camera_fullscreen->setAspectRatio(camera->getAspectRatio());
		camera_fullscreen->setUpVector(clippingPlaneUp);
		float object_size = abs(volumeRatio.x * clippingPlaneUp.x) + abs(volumeRatio.y * clippingPlaneUp.y) + abs(volumeRatio.z * clippingPlaneUp.z);
		float object_size_half = object_size / 2.0f;
		float fov_y = camera_fullscreen->getFov() / camera_fullscreen->getAspectRatio();
		float fov_y_half = glm::radians(fov_y * 0.5f);
		float camera_distance = object_size_half / glm::tan(fov_y_half);
		camera_fullscreen->setPosition(clippingPlaneOrigin + clippingPlaneNormal * -camera_distance);
		camera_fullscreen->setViewCenter(glm::vec3(clippingPlaneOrigin));

		// decompose original camera matrix
		glm::mat4 transformation = camera->getViewMatrix() * volumeModelMatrix;
		glm::vec3 scale;
		glm::quat rotation;
		glm::vec3 translation;
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(transformation, scale, rotation, translation, skew, perspective);

		// decompose fullscreen camera matrix
		glm::mat4 fullscreen_transformation = camera_fullscreen->getViewMatrix();
		glm::vec3 fullscreen_scale;
		glm::quat fullscreen_rotation;
		glm::vec3 fullscreen_translation;
		glm::vec3 fullscreen_skew;
		glm::vec4 fullscreen_perspective;
		glm::decompose(fullscreen_transformation, fullscreen_scale, fullscreen_rotation, fullscreen_translation, fullscreen_skew, fullscreen_perspective);

		// interpolate camera parameters
		auto fov = (1.0f - fullscreen_animation) * camera->getFov() + fullscreen_animation * camera_fullscreen->getFov();
		auto new_rotation = glm::slerp(rotation, fullscreen_rotation, fullscreen_animation);
		auto new_translation = (1.0f - fullscreen_animation) * translation + fullscreen_animation * fullscreen_translation;

		glm::mat4 rot = glm::toMat4(new_rotation);
		glm::mat4 trans = glm::translate(glm::mat4(1.0f), new_translation);
		glm::mat4 new_view = trans * rot;

		cameraDataPlane.model = model;
		cameraDataPlane.view = new_view;
		cameraDataPlane.viewInv = glm::inverse(cameraDataPlane.view);
		cameraDataPlane.aspectRatio = camera_fullscreen->getAspectRatio();
		cameraDataPlane.fov = fov;
		cameraDataPlane.proj = camera_fullscreen->getProjectionMatrix();

		// the animation speed
		fullscreen_animation += 0.05f;
		if (fullscreen_animation > 1.0f) {
			fullscreen_animation = 1.0f;
		}
	}
	else
	{
		cameraDataPlane.model = model;
		cameraDataPlane.view = camera->getViewMatrix() * volumeModelMatrix;
		cameraDataPlane.viewInv = glm::inverse(cameraDataPlane.view);
		cameraDataPlane.aspectRatio = camera->getAspectRatio();
		cameraDataPlane.fov = camera->getFov();
		cameraDataPlane.proj = camera->getProjectionMatrix();
	}


	queue.WriteBuffer(cameraBuffer, 0, &cameraDataPlane, sizeof(CameraDataPlane));

	paramDataPlane.param0 = glm::vec4(fullscreen_animation, paddingRatioX, annotationVolume, enableRawData);
	paramDataPlane.enableVolumes = glm::vec4(enableVolumeA, enableVolumeB, enableVolumeC, enableVolumeD);
	paramDataPlane.clippingPlaneOrigin = clippingPlaneOrigin;
	paramDataPlane.clippingPlaneNormal = clippingPlaneNormal;
	paramDataPlane.volumeSize = volumeSize;
	paramDataPlane.screenSize = glm::vec4(width, height, 1, 1);
	queue.WriteBuffer(paramBuffer, 0, &paramDataPlane, sizeof(ParamDataPlane));
}

void PlaneRenderer::render(const wgpu::TextureView &renderTarget)
{
	if (!isVolumeSet || !enabled) {
		return;
	}
	preRender();
		
	auto queue = device.GetQueue();
	wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
	utils::ComboRenderPassDescriptor renderPassDescriptor({ renderTarget });
	renderPassDescriptor.label = "Plane Render Pass";
	wgpu::Color clearColor = { 1.0, 0.0, 0.0, 0.0 };

	// load attachment, do not clear it
	renderPassDescriptor.cColorAttachments[0].loadOp = wgpu::LoadOp::Load;

	if (enableStencil) 
	{
		//renderPassDescriptor.cDepthStencilAttachmentInfo.clearStencil = 1;
		renderPassDescriptor.cDepthStencilAttachmentInfo.stencilClearValue = 1;
		renderPassDescriptor.cDepthStencilAttachmentInfo.stencilLoadOp = wgpu::LoadOp::Clear;
		renderPassDescriptor.cDepthStencilAttachmentInfo.stencilStoreOp = wgpu::StoreOp::Discard;
	}
	else 
	{
		renderPassDescriptor.cDepthStencilAttachmentInfo.stencilStoreOp = wgpu::StoreOp::Undefined;
		renderPassDescriptor.cDepthStencilAttachmentInfo.stencilLoadOp = wgpu::LoadOp::Undefined;
	}

	//renderPass.cColorAttachments[0].clearColor = clearColor;
	wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDescriptor);
	pass.SetPipeline(pipeline);
	pass.SetBindGroup(0, bindGroup);
	pass.SetVertexBuffer(0, vertexBuffer);
	pass.SetIndexBuffer(indexBuffer, wgpu::IndexFormat::Uint32);
	pass.DrawIndexed(6);
	
	pass.End();
	pass.Release();

	wgpu::CommandBuffer commands = encoder.Finish();
	queue.Submit(1, &commands);

	encoder.Release();
	commands.Release();
}

void PlaneRenderer::enableFullscreen()
{
	if (!fullscreen)
	{
		fullscreen = true;
		fullscreen_animation = 0.0f;
	}
}

void PlaneRenderer::disableFullscreen()
{
	if (fullscreen)
	{
		fullscreen = false;
		fullscreen_animation = 0.0f;
	}
}

void PlaneRenderer::init()
{
	std::cout << "initialize PlaneRenderer... [width: " << width << ", height: " << height << "]" << std::endl;
	float const vertData[] =
	{
		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f, // 
		 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, // 
		-1.0f,  1.0f, 0.0f, 1.0f, 0.0f, //
		1.0f,  1.0f, 0.0f, 1.0f, 1.0f, // 
	};
	uint32_t const indexData[] = { 0, 1, 2, 1, 3, 2 };
	vertexBuffer = utils::CreateBufferFromData(device, vertData, sizeof(vertData), wgpu::BufferUsage::Vertex);
	indexBuffer = utils::CreateBufferFromData(device, indexData, sizeof(indexData), wgpu::BufferUsage::Index);

	paramBuffer = utils::CreateBuffer(device, sizeof(ParamDataPlane), wgpu::BufferUsage::Uniform);
	cameraBuffer = utils::CreateBufferFromData(device, &cameraDataPlane, sizeof(CameraDataPlane), wgpu::BufferUsage::Uniform);

	std::string shaderPath("work/shader/");

	std::string vsFile(shaderPath + "plane.vs");
	std::string fsFile(shaderPath + "plane.fs");

	shaderModules = ShaderUtil::loadAndCompileShader(device, vsFile, fsFile);

	auto bgl = utils::MakeBindGroupLayout(
		device, {
			{0, wgpu::ShaderStage::Vertex, wgpu::BufferBindingType::Uniform},
			{1, wgpu::ShaderStage::Fragment, wgpu::BufferBindingType::Uniform},
			{2, wgpu::ShaderStage::Fragment, wgpu::SamplerBindingType::Filtering},
			{3, wgpu::ShaderStage::Fragment, wgpu::TextureSampleType::Float, wgpu::TextureViewDimension::e3D},
			{4, wgpu::ShaderStage::Fragment, wgpu::BufferBindingType::ReadOnlyStorage},
			{5, wgpu::ShaderStage::Fragment, wgpu::BufferBindingType::ReadOnlyStorage},
			{6, wgpu::ShaderStage::Fragment, wgpu::BufferBindingType::ReadOnlyStorage},
			{7, wgpu::ShaderStage::Fragment, wgpu::BufferBindingType::ReadOnlyStorage},
			{8, wgpu::ShaderStage::Fragment, wgpu::TextureSampleType::Depth, wgpu::TextureViewDimension::e2D},
			{9, wgpu::ShaderStage::Fragment, wgpu::TextureSampleType::Float, wgpu::TextureViewDimension::e3D},
		});

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

	wgpu::PipelineLayout pipelineLayout = utils::MakeBasicPipelineLayout(device, &bgl);

	bindGroup = utils::MakeBindGroup(
		device, bgl, {
			{0, cameraBuffer, 0, sizeof(CameraDataPlane)},
			{1, paramBuffer, 0, sizeof(ParamDataPlane)},
			{2, volumeTextureSampler},
			{3, volumeTextureView},
			{4, transferFunctionBufferColor, 0, transferFunctionBufferColorSize},
			{5, transferFunctionBufferRamp1, 0, transferFunctionBufferRamp1Size},
			{6, transferFunctionBufferRamp2, 0, transferFunctionBufferRamp2Size},
			{7, volumeRatiosBuffer, 0, volumeRatiosBufferSize},
			{8, depthTextureView},
			{9, annotationTextureView},
		});

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

	utils::ComboRenderPipelineDescriptor descriptor;
	descriptor.layout = pipelineLayout;

	// describe vertex state
	descriptor.vertex.module = shaderModules[0];
	descriptor.vertex.bufferCount = 1;
	descriptor.vertex.buffers = &vertexBufferLayout;
	// describe fragment state
	descriptor.cFragment.module = shaderModules[1];
	blend = new wgpu::BlendState();
	blend->alpha.operation = wgpu::BlendOperation::Add;
	blend->alpha.srcFactor = wgpu::BlendFactor::One;
	blend->alpha.dstFactor = wgpu::BlendFactor::One;

	blend->color.operation = wgpu::BlendOperation::Add;
	blend->color.srcFactor = wgpu::BlendFactor::SrcAlpha;
	blend->color.dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha;

	descriptor.cTargets[0].blend = blend;
	descriptor.cTargets[0].writeMask = wgpu::ColorWriteMask::All;
	descriptor.cTargets[0].format = preferredSwapChainTextureFormat;

	pipeline = device.CreateRenderPipeline(&descriptor);
}

void PlaneRenderer::setVolumeTexture(wgpu::TextureView &view)
{
	volumeTextureView = view;

	isVolumeSet = true;
}