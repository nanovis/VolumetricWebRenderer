#include "AnnotationHandler.h"

#include "Input.h"
#include "ShaderUtil.h"
#include "Helper.h"

#include "dawn/utils/WGPUHelpers.h"
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/intersect.hpp>
#include <iostream>
#include <fstream>
#include <filesystem>

struct AnnotationParam
{
	float param0;
	float param1;
	float param2;
	float param3;
	glm::uvec4 volumeSize;
	glm::vec4 annotationVolume;
	glm::vec4 vertex;
	glm::vec4 pingPong;
	glm::vec4 kernelSize;
	glm::vec4 clippingPlaneOrigin;
	glm::vec4 clippingPlaneNormal;

} annotationParam;

AnnotationHandler::AnnotationHandler(wgpu::Device device, Camera *camera, int width, int height)
{
	this->camera = camera;
	this->width = width;
	this->height = height;
	this->device = device;

	kernelSize = glm::vec3(25);
}

void AnnotationHandler::reload()
{
	if (!initialized) {
		return;
	}
	init();
}

void AnnotationHandler::init()
{
	annotationParamBuffer = utils::CreateBuffer(device, sizeof(AnnotationParam), wgpu::BufferUsage::Uniform);

	std::string shaderPath("work/shader/");
	std::string annotateFile(shaderPath + "annotate.compute");
	annotationShaderModule = ShaderUtil::loadAndCompileShader(device, annotateFile);

	std::cout << "setting up the annotation pipeline layout..." << std::endl;
	annotationPipelinePing = setupAnnotationPipelineLayout(true);
	annotationPipelinePong = setupAnnotationPipelineLayout(false);

	initialized = true;

	//annotations.clear();
	//strokes.clear();
}

void AnnotationHandler::setTexturePing(wgpu::Texture texture)
{
	this->annotationTexturePing = texture;

	wgpu::TextureViewDescriptor tex_view_desc = {};
	tex_view_desc.format = wgpu::TextureFormat::RGBA8Unorm;
	tex_view_desc.dimension = wgpu::TextureViewDimension::e3D;
	tex_view_desc.baseMipLevel = 0;
	tex_view_desc.mipLevelCount = 1;
	tex_view_desc.baseArrayLayer = 0;
	tex_view_desc.arrayLayerCount = 1;
	tex_view_desc.aspect = wgpu::TextureAspect::All;

	annotationTexturePingView = annotationTexturePing.CreateView(&tex_view_desc);
}

void AnnotationHandler::setTexturePong(wgpu::Texture texture)
{
	this->annotationTexturePong = texture;

	wgpu::TextureViewDescriptor tex_view_desc = {};
	tex_view_desc.format = wgpu::TextureFormat::RGBA8Unorm;
	tex_view_desc.dimension = wgpu::TextureViewDimension::e3D;
	tex_view_desc.baseMipLevel = 0;
	tex_view_desc.mipLevelCount = 1;
	tex_view_desc.baseArrayLayer = 0;
	tex_view_desc.arrayLayerCount = 1;
	tex_view_desc.aspect = wgpu::TextureAspect::All;

	annotationTexturePongView = annotationTexturePong.CreateView(&tex_view_desc);
}

wgpu::ComputePipeline AnnotationHandler::setupAnnotationPipelineLayout(bool pingPong)
{
	auto bgl = utils::MakeBindGroupLayout(
		device, {
			{0, wgpu::ShaderStage::Compute, wgpu::BufferBindingType::Uniform},
			{1, wgpu::ShaderStage::Compute, wgpu::StorageTextureAccess::WriteOnly, wgpu::TextureFormat::RGBA8Unorm, wgpu::TextureViewDimension::e3D},
			{2, wgpu::ShaderStage::Compute, wgpu::TextureSampleType::Float, wgpu::TextureViewDimension::e3D},
		});

	annotationBindGroup[pingPong ? 1 : 0] = utils::MakeBindGroup(
		device, bgl,
		{
			{0, annotationParamBuffer, 0, sizeof(AnnotationParam)},
			{1, pingPong ? annotationTexturePongView : annotationTexturePingView},
			{2, pingPong ? annotationTexturePingView : annotationTexturePongView},
		});

	wgpu::PipelineLayout pipelineLayout = utils::MakeBasicPipelineLayout(device, &bgl);

	wgpu::ComputePipelineDescriptor csDesc;
	csDesc.layout = pipelineLayout;
	csDesc.compute.module = annotationShaderModule;
	csDesc.compute.entryPoint = "main";

	return device.CreateComputePipeline(&csDesc);
}

void AnnotationHandler::resize(int width, int height)
{
	this->width = width;
	this->height = height;
}

void AnnotationHandler::processAnnotations()
{
	if (!initialized) {
		return;
	}
	
	// map mouse position to [0..1]
	float posX = Input::mousePosition().first / (float)width;
	float posY = 1.0f - (Input::mousePosition().second / (float)height);

	float epsilon = 0.002f;
	if (abs(posX - prev_pos_x) + abs(posY - prev_pos_y) < epsilon) {
		return;
	}
	prev_pos_x = posX;
	prev_pos_y = posY;

	// map from [0..1] to [-1..1]
	posX = (posX * 2.0f) - 1.0f;
	posY = (posY * 2.0f) - 1.0f;

	float fov = (camera->getFov() / 360.0f) * (2.0f * 3.141592654f);
	float fl = 1.0f / tan(fov / 2.0f);

	glm::mat4 viewInv = glm::inverse(camera->getViewMatrix() * volumeModelMatrix);

	// ray
	glm::vec4 tmp = glm::vec4(posX / (1.0f / camera->getAspectRatio()), posY, 0.0f - fl, 1.0f);
	glm::vec3 ve(viewInv * tmp);
	glm::vec3 rayOrig = glm::vec3(viewInv * glm::vec4(0.0, 0.0, 0.0, 1.0));
	glm::vec3 rayDir = glm::normalize(ve - rayOrig);

	float intersectionDistance = 0;

	bool intersects = glm::intersectRayPlane(rayOrig, rayDir, glm::vec3(clippingPlaneOrigin), glm::vec3(clippingPlaneNormal), intersectionDistance);

	if (!intersects) {
		return;
	}

	glm::vec3 intersection = rayOrig + intersectionDistance * rayDir;
	glm::vec3 vertex = (intersection / volumeRatios[0]) * glm::vec3(0.5) + glm::vec3(0.5);
	
	applyAnnotations(vertex, false);
	pingPong = !pingPong;
	// copy annotation to other texture 
	applyAnnotations(vertex, true);
}

void AnnotationHandler::applyAnnotations(glm::vec3 vertex, bool copy)
{
	if (vertex.x >= 0.0 && vertex.x <= 1.0 &&
		vertex.y >= 0.0 && vertex.y <= 1.0 &&
		vertex.z >= 0.0 && vertex.z <= 1.0)
	{
		auto queue = device.GetQueue();

		annotationParam.param0 = Input::mousePosition().first / (float)width;
		annotationParam.param1 = Input::mousePosition().second / (float)height;
		annotationParam.param2 = addAnnotation ? 1.0 : 0.0f;
		annotationParam.volumeSize = volumeSize;
		annotationParam.annotationVolume = glm::vec4(annotationVolume, 0, 0, 0);
		annotationParam.vertex = glm::vec4(vertex.x, vertex.y, vertex.z, 1.0f);
		annotationParam.pingPong = glm::vec4(float(copy), 0.0f, 0.0f, 0.0f);
		annotationParam.kernelSize = glm::ivec4(kernelSize.x, kernelSize.y, kernelSize.z, 0.0f);
		annotationParam.clippingPlaneOrigin = clippingPlaneOrigin;
		annotationParam.clippingPlaneNormal = clippingPlaneNormal;

		queue.WriteBuffer(annotationParamBuffer, 0, &annotationParam, sizeof(AnnotationParam));

		wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
		wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
		pass.SetPipeline(pingPong ? annotationPipelinePing : annotationPipelinePong);
		pass.SetBindGroup(0, annotationBindGroup[pingPong ? 1 : 0]);
		unsigned numBlocksX = ceil(float(int(kernelSize.x) * 2 + 1) / 4.0f);
		unsigned numBlocksY = ceil(float(int(kernelSize.y) * 2 + 1) / 4.0f);
		unsigned numBlocksZ = ceil(float(int(kernelSize.z) * 2 + 1) / 4.0f);


		pass.DispatchWorkgroups(numBlocksX, numBlocksY, numBlocksZ);
		pass.End();

		wgpu::CommandBuffer commands = encoder.Finish();

		queue.Submit(1, &commands);

		encoder.Release();

		// serialization
		if (newStroke) 
		{
			Stroke stroke;
			stroke.volume_name = volumeDescriptors[annotationVolume].fileName;
			stroke.kernelSize = kernelSize;
			stroke.add = addAnnotation;

			strokes.push_back(stroke);
			newStroke = false;
		}
		strokes.back().positions.push_back(vertex);
	}
}

void AnnotationHandler::serialize(std::string file_name)
{
	json jStrokes;
	Stroke stroke;
	stroke.add = true;
	stroke.kernelSize = glm::vec3(666);
	stroke.positions.push_back(glm::vec3(1));
	stroke.volume_name = "asd";
		
	
	for (size_t i = 0; i < strokes.size(); i++)
	{
		Stroke stroke = strokes[i];
		json jStroke;
		jStroke["volume_name"] = stroke.volume_name;

		json jKernelSize;
		jKernelSize["x"] = stroke.kernelSize.x;
		jKernelSize["y"] = stroke.kernelSize.y;
		jKernelSize["z"] = stroke.kernelSize.z;

		jStroke["add"] = stroke.add;

		jStroke["kernel_size"] = jKernelSize;

		json jPositions;
		for (size_t j = 0; j < stroke.positions.size(); j++)
		{
			json jPosition;
			glm::vec3 position = stroke.positions[j];
			jPosition["x"] = position.x;
			jPosition["y"] = position.y;
			jPosition["z"] = position.z;
			jPositions.push_back(jPosition);
		}
		jStroke["positions"] = jPositions;
		jStrokes.push_back(jStroke);
	}
#ifndef __EMSCRIPTEN__
	std::filesystem::create_directories("work/annotations/");
#endif
	
	std::string annotation_path("work/annotations/");
	std::ofstream file(annotation_path + file_name);
	file << std::setw(2) << jStrokes << std::endl;
	file.close();
}