#include "ShaderUtil.h"

#include "dawn/utils/WGPUHelpers.h"

#include <array>
#include <fstream>
#include <streambuf>
#include <iostream>

std::array<wgpu::ShaderModule, 2> ShaderUtil::loadAndCompileShader(wgpu::Device &device, std::string& vsFile, std::string& fsFile)
{
	std::cout << "load shader module: " << vsFile << ", " << fsFile << std::endl;
	bool ok;
	std::string vsVolume = loadShader(vsFile, ok);
	if (!ok)
	{
		std::cout << "error while loading shader." << vsVolume;
	}

	std::string fsVolume = loadShader(fsFile, ok);
	if (!ok)
	{
		std::cout << "error while loading shader." << fsVolume;
	}

	std::array<wgpu::ShaderModule, 2> shaderModules;

	// compile shaders
	shaderModules[0] = utils::CreateShaderModule(device, vsVolume.c_str());
	shaderModules[1] = utils::CreateShaderModule(device, fsVolume.c_str());

	return shaderModules;
}

wgpu::ShaderModule ShaderUtil::loadAndCompileShader(wgpu::Device &device, std::string& computeFile)
{
	std::cout << "load shader module: " << computeFile << std::endl;
	bool ok;
	std::string compute = loadShader(computeFile, ok);
	if (!ok)
	{
		std::cout << "error while loading shader." << compute;
	}

	std::array<wgpu::ShaderModule, 2> shaderModules;

	// compile shaders
	wgpu::ShaderModule shaderModule = utils::CreateShaderModule(device, compute.c_str());

	return shaderModule;
}

std::string ShaderUtil::loadShader(std::string file, bool& ok)
{
	ok = true;
	std::ifstream ifile(file);
	if (ifile.fail()) {
		ok = false;
		return std::string();
	}
	std::string str;

	ifile.seekg(0, std::ios::end);
	str.reserve(ifile.tellg());
	ifile.seekg(0, std::ios::beg);

	str.assign((std::istreambuf_iterator<char>(ifile)),
		std::istreambuf_iterator<char>());

	return str;
}