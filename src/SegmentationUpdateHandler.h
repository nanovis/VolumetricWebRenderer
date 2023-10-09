#pragma once

#include <webgpu/webgpu_cpp.h>
#include "VolumeLoader.h"

class SegmentationUpdateHandler
{
public:
	SegmentationUpdateHandler(wgpu::Device device);

	virtual ~SegmentationUpdateHandler();

	void init();

	void process(int which, wgpu::Texture segmentationTexture, wgpu::Texture volumeTexture, wgpu::TextureView volumeTextureView, VolumeDescriptor &volumeDescriptor);

	void copyTextureToTexture(wgpu::Texture sourceTexture, wgpu::Texture destinationTexture);

private:
	
	void createResources(VolumeDescriptor &volumeDescriptor, wgpu::Texture segmentationTexture);

	wgpu::ComputePipeline setupPipelineLayout(wgpu::TextureView volumeTextureView, wgpu::TextureView newSegmentationTextureView);

	bool initialized = false;

	VolumeDescriptor volumeDescriptor;

	wgpu::Device device;

	wgpu::BindGroup bindGroup;
	wgpu::Buffer paramBuffer;

	wgpu::Texture tmpVolumeTexture = nullptr;
	wgpu::TextureView tmpVolumeTextureView = nullptr;

	wgpu::TextureView segmentationTextureView;

	wgpu::ComputePipeline pipeline;
	wgpu::ShaderModule shaderModule = nullptr;

};

