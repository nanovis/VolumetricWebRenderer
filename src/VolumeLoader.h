#pragma once

#include "PagedVolume.h"
#include "VolumeHelper.h"
#include <glm/glm.hpp>
#include <webgpu/webgpu_cpp.h>
#include <string>
#include <vector>

class VolumeLoader
{
public:

	/**
	 * Constructs a new Volume Loader object.
	 */
	VolumeLoader();
	/**
	 * Destructs the Volume Loader object.
	 */
	~VolumeLoader();

	/**
	 * Opens volume data. 
	 * @param file The config file.
	 */
	bool openVolumes(std::string &file);

	/**
	 * Loads and uploads volume data to the GPU.
	 * @param device The webgpu device.
	 */
	bool loadAndUploadVolumes(wgpu::Device device);

	/**
	 * Loads and streams volume data to the GPU slice by slice.
	 * @param device The webgpu device.
	 */
	bool loadAndUploadVolumesBySlice(wgpu::Device device);

	/**
	 * Create a webgpu texture according the the input parameters.
	 * @param device The webgpu device.
	 */
	wgpu::Texture createTexture(wgpu::Device device, int sizeX, int sizeY, int sizeZ, wgpu::TextureFormat format, bool storageBinding = false);

	std::vector<VolumeDescriptor> volumeDescriptors;
	std::vector<wgpu::Texture> volumeTextures;

private:

	void load(VolumeDescriptor& volume_descriptor, int destination, bool rgb);

	void uploadData(wgpu::Texture volumeTexture, wgpu::Device device, bool rgba, int sizeX, int sizeY, int sizeZ);

	void uploadDataBuffer(wgpu::Texture volumeTexture, wgpu::Device device, bool rgba, int sizeX, int sizeY, int sizeZ);

	void uploadDataSlice(wgpu::Texture volumeTexture, wgpu::Device device, bool rgba, int sizeX, int sizeY, int slice);

	void uploadDataSliceBuffer(wgpu::Texture volumeTexture, wgpu::Device device, bool rgba, int sizeX, int sizeY, int slice);

	TransferFunction loadTransferFunction(std::string filename);
	
	void loadFromRaw(VolumeDescriptor &volume_descriptor, int destination, int numChannels);
	
	void loadSliceFromRaw(VolumeDescriptor& volume_descriptor, int slice, int destination, int numChannels);

	unsigned int highestValue;
	
	std::vector<unsigned char> data;
};

