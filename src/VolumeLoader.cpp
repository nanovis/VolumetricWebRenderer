#include "VolumeLoader.h"

#include "PagedVolume.h"

#include <vector>
#include <iostream>
#include <fstream>  
#include <algorithm>
#include <math.h>

// define is used so that json throws exceptions
#define __EXCEPTIONS
#include <json/json.hpp>
using json = nlohmann::json;

VolumeLoader::VolumeLoader()
{

}

VolumeLoader::~VolumeLoader()
{
}

bool VolumeLoader::loadAndUploadVolumes(wgpu::Device device)
{
	volumeTextures.clear();
	
	for (int i = 0; i < volumeDescriptors.size(); i++)
	{
		VolumeDescriptor descriptor = volumeDescriptors[i];
		int numChannels = 4;
		long long size = (long long)descriptor.sizeX * (long long)descriptor.sizeY * (long long)descriptor.sizeZ * (long long)numChannels;
		bool rgb = true;
		//allocateMemory
		if (i == 0) 
		{
			std::cout << "going to alloc bytes: " << size * sizeof(unsigned short) << std::endl;
			data = std::vector<unsigned char>(size, 0);
		}

		if (i == 4) 
		{
			std::cout << "going to alloc bytes: " << size * sizeof(unsigned short) << std::endl;
			data = std::vector<unsigned char>(size, 0);
		}

		load(descriptor, i < 4 ? i : 0, rgb);

		if (i == 3)
		{
			//for first 4 volumes, pack them into a single RGBA texture
			volumeTextures.push_back(createTexture(device, descriptor.sizeX, descriptor.sizeY, descriptor.sizeZ, wgpu::TextureFormat::RGBA8Unorm, true));
			uploadData(volumeTextures.back(), device, rgb, descriptor.sizeX, descriptor.sizeY, descriptor.sizeZ);
		}
		else if (i > 3)
		{
			volumeTextures.push_back(createTexture(device, descriptor.sizeX, descriptor.sizeY, descriptor.sizeZ, wgpu::TextureFormat::RGBA8Unorm, false));
			uploadData(volumeTextures.back(), device, rgb, descriptor.sizeX, descriptor.sizeY, descriptor.sizeZ);
		}
	}

	return true;
}

bool VolumeLoader::loadAndUploadVolumesBySlice(wgpu::Device device)
{
	int numChannels = 4;

	// we assume all volumes have the same size
	VolumeDescriptor descriptor = volumeDescriptors[0];
	int size = descriptor.sizeX * descriptor.sizeY * numChannels;
	int numSlices = descriptor.sizeZ;

	// create textures
	volumeTextures.clear();
	volumeTextures.push_back(createTexture(device, descriptor.sizeX, descriptor.sizeY, descriptor.sizeZ, wgpu::TextureFormat::RGBA8Unorm, true));
	volumeTextures.push_back(createTexture(device, descriptor.sizeX, descriptor.sizeY, descriptor.sizeZ, wgpu::TextureFormat::RGBA8Unorm, false));

	std::cout << "loading Volumes: " << std::endl;;
	for (size_t i = 0; i < volumeDescriptors.size(); i++)
	{
		std::cout << volumeDescriptors[i].fileName << std::endl;
	}

	// load each slide separately
	for (size_t z = 0; z < numSlices; z++)
	{
		std::cout << "loading slice: " << std::to_string(z) << std::endl;
		for (int i = 0; i < volumeDescriptors.size(); i++)
		{
			int currentTexture = i / 4.0f;
			VolumeDescriptor descriptor = volumeDescriptors[i];

			if (i == 0 || i == 4)
			{
				//std::cout << "going to allocate bytes: " << size * sizeof(unsigned short) << std::endl;
				data = std::vector<unsigned char>(size, 0);
			}

			loadSliceFromRaw(descriptor, z, i < 4 ? i : 0, numChannels);
			uploadDataSlice(volumeTextures[currentTexture], device, true, descriptor.sizeX, descriptor.sizeY, z);
		}
	}
	std::cout << "volume loaded." << std::endl;

	return true;
}

bool VolumeLoader::openVolumes(std::string &file)
{
	std::string path;

	const size_t last_slash_idx = file.rfind('/');
	//const size_t last_backslash_idx = file.rfind('\\');

	if (std::string::npos != last_slash_idx)
	{
		path = file.substr(0, last_slash_idx);
		path.append("/");
	}

	std::ifstream configFile(file);
	if (configFile.is_open())
	{
		volumeDescriptors.clear();

		try
		{
			json config;
			configFile >> config;

			for (size_t i = 0; i < config["files"].size(); i++)
			{
				std::string fileName(config["files"][i]);
				std::ifstream volumeFile(path + fileName);
				if (volumeFile.is_open())
				{
					std::cout << "Load file: " << path + fileName << std::endl;
					json volume;
					volumeFile >> volume;

					std::string volumeFileName = volume["file"];

					int sizeX = volume["size"]["x"];
					int sizeY = volume["size"]["y"];
					int sizeZ = volume["size"]["z"];

					// in WebGPU the number of bytes between the beginning of each row of the texture must be a multiple of 256 bytes
					// therefore we might have to insert extra padding between rows if they're not a multiple of 256
					int numChannels = 4;
					int sizeOfTexel = sizeof(unsigned char) * numChannels; // we are using RGBA8Unorm textures
					int padding = (256 / sizeOfTexel) - sizeX % (256 / sizeOfTexel);
					padding = 0;
					int sizeXPadded = sizeX + padding;

					double voxelSizeX = volume["ratio"]["x"];
					double voxelSizeY = volume["ratio"]["y"];
					double voxelSizeZ = volume["ratio"]["z"];

					float voxelSize = 1.0f;
					if (!volume["voxelSize"].is_null())
					{
						voxelSize = std::max(volume["voxelSize"]["x"], std::max(volume["voxelSize"]["y"], volume["voxelSize"]["z"]));
						std::cout << "VOXEL SIZE" << voxelSize;
					} 

					int bytesPerVoxel = volume["bytesPerVoxel"];
					int usedBits = volume["usedBits"];
					int skipBytes = volume["skipBytes"];

					bool isLittleEndian = volume["isLittleEndian"];
					bool isSigned = volume["isSigned"];
					int addValue = volume["addValue"];
					
					TransferFunction tf;
					if (!volume["transferFunction"].is_null()) \
					{
						std::string tfFile = volume["transferFunction"].get<std::string>();
						tf = loadTransferFunction(path + tfFile);
					}
					else
					{
						tf.color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
						tf.rampLow = 0.1f;
						tf.rampHigh = 0.9f;
					}

					VolumeDescriptor volumeDescriptor;
					volumeDescriptor.path = path;
					volumeDescriptor.fileName = volumeFileName;
					volumeDescriptor.transferFunction = tf;
					volumeDescriptor.bytesPerVoxel = bytesPerVoxel;
					volumeDescriptor.usedBits = usedBits;
					volumeDescriptor.skipBytes = skipBytes;
					volumeDescriptor.littleEndian = isLittleEndian;
					volumeDescriptor.isSigned = isSigned;
					volumeDescriptor.addValue = addValue;
					volumeDescriptor.voxelSize = voxelSize;
					volumeDescriptor.sizeX = sizeX;
					volumeDescriptor.sizeY = sizeY;
					volumeDescriptor.sizeZ = sizeZ;
					volumeDescriptor.voxelSizeX = voxelSizeX;
					volumeDescriptor.voxelSizeY = voxelSizeY;
					volumeDescriptor.voxelSizeZ = voxelSizeZ;

					volumeDescriptors.push_back(volumeDescriptor);	
				}
				else
				{
					std::cout << "Unable to open volume file: " << path + fileName << std::endl;
					return false;
				}
			}
		}
		catch (json::exception& e)
		{
			std::cout << e.what() << '\n';
			return false;
		}
	}
	else
	{
		std::cout << "Unable to open file: " << file << std::endl;
		return false;
	}
	return true;
}

TransferFunction VolumeLoader::loadTransferFunction(std::string filename)
{
	std::string tfPath = filename;
	std::cout << "loading transfer function: " << tfPath << "..." << std::endl;
	std::ifstream configFile(tfPath);

	if (configFile.is_open())
	{
		try
		{
			json tfConfig = json::parse(configFile);
			TransferFunction tf{};

			tf.rampLow = tfConfig["rampLow"].get<float>();
			tf.rampHigh = tfConfig["rampHigh"].get<float>();
			tf.color.x = tfConfig["color"]["x"].get<float>() / 255.0f;
			tf.color.y = tfConfig["color"]["y"].get<float>() / 255.0f;
			tf.color.z = tfConfig["color"]["z"].get<float>() / 255.0f;
			tf.color.w = 1.0f;

			//transferFunctions.insert(std::pair<std::string, TransferFunction>("filename", tf));

			std::cout << "TF loaded." << std::endl;
			return tf;
		}
		catch (json::exception& e)
		{
			std::cout << e.what() << '\n';
			TransferFunction tf{};
			return tf;
		}
	}
	else
	{
		std::cout << "Unable to open file: " << filename << std::endl;
	}
}

void VolumeLoader::load(VolumeDescriptor &volume_descriptor, int destination, bool rgb)
{
	int numChannels = 4;
	if (!rgb) {
		numChannels = 1;
	}
	loadFromRaw(volume_descriptor, destination, numChannels);

	//std::cout << "loading " << destination << " left the data sizes like so: " << data[0].size() <<
	//																	 " | " << data[1].size() <<
	//																	 " | " << data[2].size() <<
	//																	 " | " << data[3].size() << std::endl << std::endl;
	//createPagedVolume();
}

wgpu::Texture VolumeLoader::createTexture(wgpu::Device device, int sizeX, int sizeY, int sizeZ, wgpu::TextureFormat format, bool storageBinding)
{
	wgpu::TextureDescriptor descriptor;
	descriptor.dimension = wgpu::TextureDimension::e3D;
	descriptor.size.width = sizeX;
	descriptor.size.height = sizeY;
	descriptor.size.depthOrArrayLayers = sizeZ;
	descriptor.sampleCount = 1;
	descriptor.format = format;
	descriptor.mipLevelCount = 1;
	if (storageBinding) {
		descriptor.usage = wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::StorageBinding;
	}
	else {
		descriptor.usage = wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::TextureBinding;
	}
	std::cout << "Create texture of size : " << sizeX << ", " << sizeY << ", " << sizeZ << std::endl;

	auto texture = device.CreateTexture(&descriptor);

	return texture;
}

void VolumeLoader::uploadDataSlice(wgpu::Texture volumeTexture, wgpu::Device device, bool rgba, int sizeX, int sizeY, int slice)
{
	int packChannels = 4;

	uint32_t bytesPerRow = sizeX * sizeof(unsigned char);
	uint64_t dataSize = data.size() * sizeof(unsigned char);

	if (rgba) {
		bytesPerRow *= packChannels;
	}

	wgpu::TextureDataLayout layout;
	layout.rowsPerImage = sizeY;
	layout.bytesPerRow = bytesPerRow;

	wgpu::Extent3D copySize = { (uint32_t)sizeX, (uint32_t)sizeY, (uint32_t)1 };

	wgpu::ImageCopyTexture copyTexture;
	copyTexture.texture = volumeTexture;
	copyTexture.mipLevel = 0;
	copyTexture.origin = { 0, 0, (unsigned int)slice };

	auto queue = device.GetQueue();
	queue.WriteTexture(&copyTexture, &data[0], dataSize, &layout, &copySize);
}

void VolumeLoader::uploadDataSliceBuffer(wgpu::Texture volumeTexture, wgpu::Device device, bool rgba, int sizeX, int sizeY, int slice)
{
	int packChannels = 4;

	uint64_t bufferSize = data.size() * sizeof(unsigned char);
	wgpu::BufferDescriptor bufferDescriptor;
	bufferDescriptor.size = bufferSize;
	bufferDescriptor.usage = wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst;

	wgpu::Buffer staging_buffer = device.CreateBuffer(&bufferDescriptor);

	auto queue = device.GetQueue();
	queue.WriteBuffer(staging_buffer, 0, &data[0], bufferSize);


	// bytes per row needs to be a multiple of 256
	uint32_t bytesPerRow = sizeX * sizeof(unsigned char);

	if (rgba)
	{
		bytesPerRow *= packChannels;
	}

	// Todo: check if sizeZ needs to be incorporated into the 
	wgpu::ImageCopyBuffer bufferCopyView;
	bufferCopyView.buffer = staging_buffer;
	bufferCopyView.layout.offset = 0;
	bufferCopyView.layout.bytesPerRow = bytesPerRow;
	bufferCopyView.layout.rowsPerImage = sizeY;

	wgpu::ImageCopyTexture textureCopyView;
	textureCopyView.texture = volumeTexture;
	textureCopyView.mipLevel = 0;
	textureCopyView.origin = { 0, 0, (unsigned int)slice};
	// Todo: do we need this define?
#if !defined(__EMSCRIPTEN__)
	textureCopyView.aspect = wgpu::TextureAspect::All;
#endif

	wgpu::Extent3D copySize = { (uint32_t)sizeX, (uint32_t)sizeY, (uint32_t) 1 };
	wgpu::CommandEncoderDescriptor enc_desc;
	auto encoder = device.CreateCommandEncoder(&enc_desc);
	encoder.CopyBufferToTexture(&bufferCopyView, &textureCopyView, &copySize);

	wgpu::CommandBufferDescriptor cmd_buf_desc;
	wgpu::CommandBuffer copy = encoder.Finish(&cmd_buf_desc);
	queue.Submit(1, &copy);

	encoder.Release();
	staging_buffer.Release();
}

void VolumeLoader::uploadData(wgpu::Texture volumeTexture, wgpu::Device device, bool rgba, int sizeX, int sizeY, int sizeZ)
{
	int packChannels = 4;
	
	// bytes per row needs to be a multiple of 256
	uint32_t bytesPerRow = sizeX * sizeof(unsigned char);

	uint64_t dataSize = data.size() * sizeof(unsigned char);

	if (rgba) {
		bytesPerRow *= packChannels;
	}

	wgpu::TextureDataLayout layout;
	layout.rowsPerImage = sizeY;
	layout.bytesPerRow = bytesPerRow;

	wgpu::Extent3D copySize = { (uint32_t)sizeX, (uint32_t)sizeY, (uint32_t)sizeZ };

	wgpu::ImageCopyTexture copyTexture;
	copyTexture.texture = volumeTexture;
	copyTexture.mipLevel = 0;
	copyTexture.origin = { 0, 0, 0 };

	auto queue = device.GetQueue();
	queue.WriteTexture(&copyTexture, &data[0], dataSize, &layout, &copySize);
}

void VolumeLoader::uploadDataBuffer(wgpu::Texture volumeTexture, wgpu::Device device, bool rgba, int sizeX, int sizeY, int sizeZ)
{
	int packChannels = 4;

	uint64_t bufferSize = data.size() * sizeof(unsigned char);
	wgpu::BufferDescriptor bufferDescriptor;
	bufferDescriptor.size = bufferSize;
	bufferDescriptor.usage = wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst;

	wgpu::Buffer staging_buffer = device.CreateBuffer(&bufferDescriptor);

	auto queue = device.GetQueue();
	queue.WriteBuffer(staging_buffer, 0, &data[0], bufferSize);


	// bytes per row needs to be a multiple of 256
	uint32_t bytesPerRow = sizeX * sizeof(unsigned char);

	if (rgba)
	{
		bytesPerRow *= packChannels;
	}

	// Todo: check if sizeZ needs to be incorporated into the 
	wgpu::ImageCopyBuffer bufferCopyView;
	bufferCopyView.buffer = staging_buffer;
	bufferCopyView.layout.offset = 0;
	bufferCopyView.layout.bytesPerRow = bytesPerRow;
	bufferCopyView.layout.rowsPerImage = sizeY;

	wgpu::ImageCopyTexture textureCopyView;
	textureCopyView.texture = volumeTexture;
	textureCopyView.mipLevel = 0;
	textureCopyView.origin = { 0, 0, 0 };
	// Todo: do we need this define?
#if !defined(__EMSCRIPTEN__)
	textureCopyView.aspect = wgpu::TextureAspect::All;
#endif

	wgpu::Extent3D copySize = { (uint32_t)sizeX, (uint32_t)sizeY, (uint32_t)sizeZ };
	wgpu::CommandEncoderDescriptor enc_desc;
	auto encoder = device.CreateCommandEncoder(&enc_desc);
	encoder.CopyBufferToTexture(&bufferCopyView, &textureCopyView, &copySize);

	wgpu::CommandBufferDescriptor cmd_buf_desc;
	wgpu::CommandBuffer copy = encoder.Finish(&cmd_buf_desc);
	queue.Submit(1, &copy);

	encoder.Release();
	staging_buffer.Release();
}

void VolumeLoader::loadSliceFromRaw(VolumeDescriptor &volume_descriptor, int slice, int destination, int numChannels)
{
	if (destination >= numChannels) {
		std::cout << "Error: VolumeLoader::loadFromRaw : destination >= number of channels";
		return;
	}

	auto file_name = volume_descriptor.path + volume_descriptor.fileName;

	std::ifstream file(file_name, std::ios::in | std::ios::binary);
	if (!file.is_open())
	{
		std::cout << "Unable to open file: " << file_name << std::endl;
		return;
	}

	//file.seekg(0, file.end);
	//std::streamsize size = file.tellg();
	//file.seekg(0, file.beg);

	//std::cout << "---------------------destination " << destination << " is being loaded..." << std::endl;

	int sizeOfSlice = volume_descriptor.sizeX * volume_descriptor.sizeY * volume_descriptor.bytesPerVoxel;
	//int sizeOfSlicePadded = volume_descriptor.sizeXPadded * volume_descriptor.sizeY * volume_descriptor.bytesPerVoxel;

	// initalize slice with zeros
	std::vector<char> rawData(sizeOfSlice, 0);

	file.seekg(slice * sizeOfSlice, file.beg);

	if (!file.read(rawData.data(), sizeOfSlice))
	{
		std::cout << "Error reading file: " << file_name << std::endl;
	}

	unsigned int highestBit = 0;
	unsigned int highestValue = 0;

	int counter = 0;
	int xPos = 0;
	int yPos = 0;

	for (int i = 0; i < sizeOfSlice; i += volume_descriptor.bytesPerVoxel)
	{
		unsigned int value = 0;
		unsigned char rawValue;

		for (int j = 0; j < volume_descriptor.bytesPerVoxel; j++)
		{
			rawValue = (unsigned char)(rawData[i + j]);

			if (volume_descriptor.littleEndian)
			{
				value += (rawValue << (8 * j));
			}
			else
			{
				value = (value << 8) + rawValue;
			}
		}

		if (volume_descriptor.isSigned)
		{
			signed int tmp = (signed int)value;
			tmp += volume_descriptor.addValue;
			value = (unsigned int)tmp;
		}

		if (volume_descriptor.usedBits == 0)
		{
			//find highest set bit
			unsigned int bit;
			for (unsigned int j = volume_descriptor.bytesPerVoxel * 8 - 1; j > 0; j--)
			{
				if (j <= highestBit) {
					break;
				}

				bit = 1 << (j);

				if ((value & bit) > 0) {
					highestBit = j;
				}
			}
		}

		if (value > highestValue) {
			highestValue = value;
		}

		int paddedCounter = yPos * volume_descriptor.sizeX * numChannels + xPos * numChannels + destination;
		data[paddedCounter] = value;
	

		xPos++;
		if (xPos >= volume_descriptor.sizeX) {
			xPos = 0;
			yPos++;
		}
		counter++;
	}

	// debug output
	//std::cout << " raw volume data size: " << volume_descriptor.sizeX * volume_descriptor.sizeY * volume_descriptor.sizeZ << " unsigned shorts" << std::endl;
	//std::cout << "highest bit: " << highestBit << std::endl;;
	//std::cout << "highest value: " << highestValue << std::endl;;

	return;
}

void VolumeLoader::loadFromRaw(VolumeDescriptor &volume_descriptor, int destination, int numChannels)
{
	if (destination >= numChannels) {
		std::cout << "Error: VolumeLoader::loadFromRaw : destination >= number of channels";
		return;
	}

	auto file_name = volume_descriptor.path + volume_descriptor.fileName;

	std::cout << "loading Volume" << file_name << std::endl;

	std::ifstream file(file_name, std::ios::in | std::ios::binary);
	if (!file.is_open())
	{
		std::cout << "Unable to open file: " << file_name << std::endl;
		return;
	}

	file.seekg(0, file.end);
	std::streamsize size = file.tellg();
	file.seekg(0, file.beg);

	std::cout << "---------------------destination " << destination << " is being loaded..." << std::endl;

	// initalize with zeros
	std::vector<char> rawData(size, 0);

	if (!file.read(rawData.data(), size))
	{
		std::cout << "Error reading file" << file_name << std::endl;
	}

	file.close();

	if (volume_descriptor.sizeX * volume_descriptor.sizeY * volume_descriptor.sizeZ * volume_descriptor.bytesPerVoxel > rawData.size() - volume_descriptor.skipBytes)
	{
		std::cout << "Error in file size: " << file_name << std::endl;
		std::cout << volume_descriptor.sizeX << "*" << volume_descriptor.sizeY << "*" << volume_descriptor.sizeZ << "*" << 
			volume_descriptor.bytesPerVoxel << " = " << (volume_descriptor.sizeX * volume_descriptor.sizeY * volume_descriptor.sizeZ * volume_descriptor.bytesPerVoxel) << std::endl;
		std::cout << rawData.size() << " - " << volume_descriptor.skipBytes << " = " << (rawData.size() - volume_descriptor.skipBytes) << std::endl;
		return;
	}

	unsigned int highestBit = 0;
	unsigned int highestValue = 0;

	int counter = 0;
	int xPos = 0;
	int yPos = 0;
	int zPos = 0;

	for (int i = volume_descriptor.skipBytes; i < std::min(volume_descriptor.sizeX * volume_descriptor.sizeY * volume_descriptor.sizeZ * volume_descriptor.bytesPerVoxel, (int)rawData.size()); i += volume_descriptor.bytesPerVoxel)
	{
		unsigned int value = 0;
		unsigned char rawValue;

		for (int j = 0; j < volume_descriptor.bytesPerVoxel; j++)
		{
			rawValue = (unsigned char)(rawData[i + j]);

			if (volume_descriptor.littleEndian)
			{
				value += (rawValue << (8 * j));
			}
			else
			{
				value = (value << 8) + rawValue;
			}
		}

		if (volume_descriptor.isSigned)
		{
			signed int tmp = (signed int)value;
			tmp += volume_descriptor.addValue;
			value = (unsigned int)tmp;
		}

		if (volume_descriptor.usedBits == 0)
		{
			//find highest set bit
			unsigned int bit;
			for (unsigned int j = volume_descriptor.bytesPerVoxel * 8 - 1; j > 0; j--)
			{
				if (j <= highestBit) {
					break;
				}

				bit = 1 << (j);

				if ((value & bit) > 0) {
					highestBit = j;
				}
			}
		}
		
		long long paddedCounter = (long long)zPos * volume_descriptor.sizeY * (long long)volume_descriptor.sizeX * (long long)numChannels + (long long)yPos * (long long)volume_descriptor.sizeX * (long long)numChannels + (long long)xPos * (long long)numChannels + (long long)destination;
		data[paddedCounter] = value;

		xPos++;
		if (xPos >= volume_descriptor.sizeX) {
			xPos = 0;
			yPos++;
			if (yPos >= volume_descriptor.sizeY) {
				yPos = 0;
				zPos++;
			}
		}

		counter++;
	}

	if (volume_descriptor.usedBits == 0)
	{
		highestBit++;
	}
	else
	{
		highestBit = volume_descriptor.usedBits;
		highestValue = (int)pow(2.0, (double)volume_descriptor.usedBits);
	}

	// debug output
	//std::cout << " raw volume data size: " << volume_descriptor.sizeX * volume_descriptor.sizeY * volume_descriptor.sizeZ << " unsigned shorts" << std::endl;
	//std::cout << "highest bit: " << highestBit << std::endl;;
	//std::cout << "highest value: " << highestValue << std::endl;;

	return;
}
