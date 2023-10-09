#include "SegmentationUpdateHandler.h"

#include "dawn/utils/WGPUHelpers.h"
#include "ShaderUtil.h"
#include "Helper.h"

#include <glm/gtx/string_cast.hpp>
#include <iostream>

struct SegmentationParam
{
	glm::vec4 param0;
} segmentationParam;

SegmentationUpdateHandler::SegmentationUpdateHandler(wgpu::Device device)
{
	this->device = device;
}

SegmentationUpdateHandler::~SegmentationUpdateHandler()
{

}

void SegmentationUpdateHandler::init()
{
	std::string shaderPath("work/shader/");
	std::string shaderFile(shaderPath + "maskUpdate.compute");
	shaderModule = ShaderUtil::loadAndCompileShader(device, shaderFile); 

	paramBuffer = utils::CreateBuffer(device, sizeof(SegmentationParam), wgpu::BufferUsage::Uniform);
	
	initialized = true;
}

void SegmentationUpdateHandler::copyTextureToTexture(wgpu::Texture sourceTexture, wgpu::Texture destinationTexture)
{
	wgpu::ImageCopyTexture sourceCopyTexture;
	sourceCopyTexture.texture = sourceTexture;
	sourceCopyTexture.mipLevel = 0;
	sourceCopyTexture.origin = { 0, 0, 0 };

	wgpu::ImageCopyTexture destinationCopyTexture;
	destinationCopyTexture.texture = destinationTexture;
	destinationCopyTexture.mipLevel = 0;
	destinationCopyTexture.origin = { 0, 0, 0 };

	wgpu::Extent3D copySize = { (uint32_t)volumeDescriptor.sizeX, (uint32_t)volumeDescriptor.sizeY, (uint32_t)volumeDescriptor.sizeZ };

	// copy volume texture
	wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
	encoder.CopyTextureToTexture(&sourceCopyTexture, &destinationCopyTexture, &copySize);
	
	wgpu::CommandBuffer commands = encoder.Finish();
	auto queue = device.GetQueue();
	queue.Submit(1, &commands);
	
	encoder.Release();
}

void SegmentationUpdateHandler::process(int which, wgpu::Texture segmentationTexture, wgpu::Texture volumeTexture, wgpu::TextureView volumeTextureView, VolumeDescriptor &volumeDescriptor)
{
	if (!initialized) {
		return;
	}
	this->volumeDescriptor = volumeDescriptor;
	createResources(volumeDescriptor, segmentationTexture);
	
	copyTextureToTexture(volumeTexture, tmpVolumeTexture);
	
	pipeline = setupPipelineLayout(volumeTextureView, segmentationTextureView);

	
	wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
	
	segmentationParam.param0 = glm::vec4(volumeDescriptor.sizeX, volumeDescriptor.sizeY, volumeDescriptor.sizeZ, which);
	
	auto queue = device.GetQueue();
	queue.WriteBuffer(paramBuffer, 0, &segmentationParam, sizeof(SegmentationParam));
		
	wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
	pass.SetPipeline(pipeline);
	pass.SetBindGroup(0, bindGroup);
	unsigned numBlocksX = ceil(float(volumeDescriptor.sizeX) / 4.0f);
	unsigned numBlocksY = ceil(float(volumeDescriptor.sizeY) / 4.0f);
	unsigned numBlocksZ = ceil(float(volumeDescriptor.sizeZ) / 4.0f);
	pass.DispatchWorkgroups(numBlocksX, numBlocksY, numBlocksZ);
	pass.End();

	wgpu::CommandBuffer commands = encoder.Finish();

	queue.Submit(1, &commands);

	encoder.Release();
	tmpVolumeTexture.Release();
	tmpVolumeTextureView.Release();
}

void SegmentationUpdateHandler::createResources(VolumeDescriptor &volumeDescriptor, wgpu::Texture segmentationTexture)
{
	VolumeLoader volumeLoader;
	tmpVolumeTexture = volumeLoader.createTexture(device, volumeDescriptor.sizeX, volumeDescriptor.sizeY, volumeDescriptor.sizeZ, wgpu::TextureFormat::RGBA8Unorm, true);

	wgpu::TextureViewDescriptor tex_view_desc = {};
	tex_view_desc.format = wgpu::TextureFormat::RGBA8Unorm;
	tex_view_desc.dimension = wgpu::TextureViewDimension::e3D;
	tex_view_desc.baseMipLevel = 0;
	tex_view_desc.mipLevelCount = 1;
	tex_view_desc.baseArrayLayer = 0;
	tex_view_desc.arrayLayerCount = 1;
	tex_view_desc.aspect = wgpu::TextureAspect::All;

	tmpVolumeTextureView = tmpVolumeTexture.CreateView(&tex_view_desc);

	segmentationTextureView = segmentationTexture.CreateView(&tex_view_desc);
}

wgpu::ComputePipeline SegmentationUpdateHandler::setupPipelineLayout(wgpu::TextureView volumeTextureView, wgpu::TextureView newSegmentationTextureView)
{
	auto bgl = utils::MakeBindGroupLayout(
		device, {
			{0, wgpu::ShaderStage::Compute, wgpu::BufferBindingType::Uniform},
			{1, wgpu::ShaderStage::Compute, wgpu::TextureSampleType::Float, wgpu::TextureViewDimension::e3D},
			{2, wgpu::ShaderStage::Compute, wgpu::TextureSampleType::Float, wgpu::TextureViewDimension::e3D},
			{3, wgpu::ShaderStage::Compute, wgpu::StorageTextureAccess::WriteOnly, wgpu::TextureFormat::RGBA8Unorm, wgpu::TextureViewDimension::e3D},
			
		});

	bindGroup = utils::MakeBindGroup(
		device, bgl,
		{
			{0, paramBuffer, 0, sizeof(SegmentationParam)},
			{1, tmpVolumeTextureView},
			{2, newSegmentationTextureView},
			{3, volumeTextureView},
		});

	wgpu::PipelineLayout pipelineLayout = utils::MakeBasicPipelineLayout(device, &bgl);

	wgpu::ComputePipelineDescriptor csDesc;
	csDesc.layout = pipelineLayout;
	csDesc.compute.module = shaderModule;
	csDesc.compute.entryPoint = "main";

	return device.CreateComputePipeline(&csDesc);
}
