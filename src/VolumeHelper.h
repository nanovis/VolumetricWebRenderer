#pragma once

#include <glm/glm.hpp>
#include <string>

struct TransferFunction
{
	glm::vec4 color;
	float rampLow;
	float rampHigh;
};

struct VolumeDescriptor
{
	std::string path;
	std::string fileName;
	TransferFunction transferFunction;
	int bytesPerVoxel, usedBits, skipBytes;
	bool littleEndian;
	bool isSigned;
	int addValue;
	float voxelSize = 1.0f;
	int sizeX, sizeY, sizeZ;
	double voxelSizeX, voxelSizeY, voxelSizeZ;
};
